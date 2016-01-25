
/* ================================================================
  $Id: u2u_transform_raw.c,v 1.6 2011/10/01 04:31:13 oneway Exp oneway $

  Built on u2u_transform.c (v 1.19), to  
  1. Decapsulates incoming UDP to TCP
  2. Encapsulates outgoing TCP to UDP 

In 1, incoming UDP has payload padding and encryption, except 
if the payload is PSH/ACK, no padding, only encryption. 
In 2, no padding/encryption yet. 

Difference from u2u_transform: 

1. Using raw socket, instead of UDP socket, to send outgoing UDP encap'd TCP, so 
   it has better control of IP and UDP header fields. Now ip.id is set as random, 
   instead of 0 by the kernel when using UDP socket. 
2. If "-s 0" or not given, will use random port number. 

Iptables rules used: 

port=4000
/sbin/iptables -F
/sbin/iptables -I INPUT -p udp --dport $port -j NFQUEUE --queue-num 3
/sbin/iptables -I OUTPUT -p tcp -s 192.168.1.80 --sport $port -j NFQUEUE --queue-num 3

# Need to turn off TSO:
# list offload options:
[root@vadtw4 ~]# /usr/sbin/ethtool -k eth0

# turn off tso
[root@vadtw4 ~]# /usr/sbin/ethtool -K eth0 tso off

====================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/types.h>
#include <linux/netfilter.h>		/* for NF_ACCEPT */
#include <arpa/inet.h>
#include <sys/socket.h>

#include <openssl/rand.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define __FAVOR_BSD
#include <netinet/udp.h>
#include <netinet/tcp.h>

// out is a static buffer. returns new length of data in out
extern int DecryptPacket0(char* in, int in_len, char* out);
extern int EncryptPacket0(char* in, int in_len, char* out);
extern int DecryptPacket1(char* in, int in_len, char* out);
extern int EncryptPacket1(char* in, int in_len, char* out);
extern int DecryptPacket2(char* in, int in_len, char* out);
extern int EncryptPacket2(char* in, int in_len, char* out);
extern int DecryptPacket3(char* in, int in_len, char* out);
extern int EncryptPacket3(char* in, int in_len, char* out);

int (*EncryptPacket)(char* in, int in_len, char* out);
int (*DecryptPacket)(char* in, int in_len, char* out);

/* global variables */
int stateful = 0;    // 1: transforms psh/ack after seeing ack in 
                     // 0: transforms psh/ack anyway 

int usport = 0;   // default userver src port 

int sock;   // for socket sending of UDP

unsigned char newpkt[4096];   // this is for transforms that expand 
                              // the size of the original packet. 
                              // Need a new buffer for that. 
unsigned char encbuf[4096];   // buffer for enc/dec functions 

unsigned int sslrand()
{
    unsigned int randint;
    RAND_bytes((unsigned char*) &randint, sizeof(int));
    return randint;
}

struct tcp_option_mss {
    uint8_t kind; /* 2 */
    uint8_t len; /* 4 */
    uint16_t mss;
} __attribute__((packed));

struct pseudo_header {   //needed for checksum calculation
        unsigned long source_address;
        unsigned long dest_address;
        unsigned char placeholder;
        unsigned char protocol;
        unsigned short tcp_length;
        //char tcp[28];
        struct tcphdr tcp;
};   

struct upseudo_header {   //needed for checksum calculation
        unsigned long source_address;
        unsigned long dest_address;
        unsigned char placeholder;
        unsigned char protocol;
        unsigned short udp_length;
        //char tcp[28];
        struct udphdr udp;
};

//General Networking Functions
 unsigned short in_checksum( unsigned short *ptr, int nbytes) {
        register long sum;
        unsigned short oddbyte;
        register short answer;

        sum=0;
        while(nbytes>1) {
                sum+=*ptr++;
                nbytes-=2;
        }
        if(nbytes==1) {
                oddbyte=0;
                *((u_char*)&oddbyte)=*(u_char*)ptr;
                sum+=oddbyte;
        }

        sum = (sum>>16)+(sum & 0xffff);
        sum = sum + (sum>>16);
        answer=(short)~sum;

        return(answer);
}

void print_ip_pkt(unsigned char *data, unsigned int ip_len) { 
    unsigned int i; 

    printf("payload_len=%d \n", ip_len);
    // display packet data
    printf("%43s", " ");
    for (i = 0; i < ip_len; i++) {
        printf("%02x ", data[i]);
        switch ( (i+15)% 16 ) {
           case 0: printf("\n");  break;
           case 8: printf(" ");  break;
        }
    }
   fputc('\n', stdout);
   fputc('\n', stdout);
}

void fill_rand(unsigned char * buf, int rand_len)
{
    RAND_bytes(buf, rand_len);
    return;
}

static int add_padding(unsigned char * buf , int in_buf_len, int pad_len)
{
    int out_buf_len = in_buf_len;
    memmove(buf + 1, buf, in_buf_len);

    // not including itself
    *buf = pad_len;
    ++ out_buf_len;
    
    // append random bytes to the end
    fill_rand(buf + out_buf_len, pad_len);
    out_buf_len += pad_len;

    return out_buf_len;
}

static int rm_padding(unsigned char * buf, int buf_len)
{
    int pad_len = *buf;

    if ( (buf_len - pad_len -1) < 20 ) {
        return buf_len;
    }

    memmove(buf, buf+1, buf_len-pad_len -1);
    return buf_len - pad_len - 1;
}

/* open a TCP/IP raw socket for sending data */
/*  returns socket id if successful, -1 otherwise */
 int open_rawsock() {
    int on=1; /* IP_HDRINCL on or off. 0: let kernel make header */
    int rawsocket;

    if ( (rawsocket = socket(AF_INET, SOCK_RAW, IPPROTO_IPIP)) < 0){
        perror("main:open_rawsock()");
        return(-1);
    }

    //* we build the ip header
    if( setsockopt(rawsocket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
        perror("main:setsockopt()");
        return(-1);
    }

    return rawsocket;
}

/* insert UDP header to a TCP segment
   sends out a UDP via a raw socket,  so hopefully
   conntrack can fix it. input newpkt is a TCP segment w/ ip header
*/
int send_udp_raw(unsigned char *data, unsigned int pay_len) {
  unsigned char seudo[4096];
  struct upseudo_header uphdr;
   unsigned short ip_hdr_len, ip_len, newl, tcpl, encl; 
   unsigned short after_enc_len, tcp_len, new_tcp_len, pad_len;
   struct sockaddr_in dst_addr;   // source and dst
   struct ip *iph0 = (struct ip *) data;
   // stuffing new packet
   struct ip *iph = (struct ip *) newpkt;
   struct udphdr *udph = (struct udphdr *) ( newpkt + sizeof(struct ip) ) ;
   struct tcphdr *tcph = (struct tcphdr *) ( newpkt + sizeof(struct ip) 
                                                    + sizeof(struct udphdr) ) ;
   struct tcp_option_mss *mss;

   ip_hdr_len = 4 * iph0->ip_hl;
   ip_len = ntohs(iph0->ip_len);
   if ( ip_len > pay_len ) {
       // Corrupted packet. Do nothing.
       return pay_len;      
   }

  tcpl = ip_len - sizeof(struct ip);
  memcpy( (char *)iph, data, ip_hdr_len );
  memcpy( (char *)tcph, data + ip_hdr_len, tcpl);

#ifdef DEBUG
 // see the interesting post: 
 // http://ttocp.blogspot.com/2011/05/weird-behavior-of-ip-header-and.html
     printf("Intercepted outgoing TCP: %s:%d -> ", 
           inet_ntoa(iph->ip_src), ntohs(tcph->th_sport));
     printf("%s:%d\n", 
           inet_ntoa(iph->ip_dst), ntohs(tcph->th_dport) );
     printf("IP header len: %d  ip_len: %d  pay_len: %d tcpl: %d\n", 
           ip_hdr_len, ip_len, pay_len, tcpl); 
#endif

  udph->uh_sport = tcph->th_sport;
  udph->uh_dport = tcph->th_dport;
  udph->uh_sum  = 0;
  if (usport == 0 ) { 
    tcph->th_sport = sslrand(); 
  } else {
    tcph->th_sport = htons(usport);
  }

  memset(&dst_addr, 0, sizeof(dst_addr));
  dst_addr.sin_family = AF_INET;
  dst_addr.sin_addr.s_addr = iph->ip_dst.s_addr;
  dst_addr.sin_port = tcph->th_dport;

  // change MSS values to avoid ip frag in returning packet
  if (tcph->th_flags == 0x12) {  // has MSS field
     mss = (struct tcp_option_mss *) ( (char *) tcph + sizeof(struct tcphdr));
#ifdef DEBUG
     printf("Outgoing Original MSS=%d\n", ntohs(mss->mss) );
     printf("Outgoing destination: %s:%d\n", inet_ntoa(dst_addr.sin_addr), 
                                             ntohs(tcph->th_dport) );
#endif
     if ( ntohs(mss->mss) > 1340 ) mss->mss = htons(1340);
  }

  tcp_len = ip_len - ip_hdr_len;
  if ( tcp_len < 20 ) {
        // corrupted packet. Do nothing.
        return pay_len;
  }
#ifdef DEBUG
   printf("New packet IP header: \n"); 
   print_ip_pkt((unsigned char *)iph, ip_hdr_len);
#endif
  if ( tcp_len >= 200 ) {
      pad_len = sslrand() % 100 + 1;
  }
  else {
      pad_len = sslrand() % 80 + 10;
  }
  tcp_len = add_padding((unsigned char *)tcph, tcp_len, pad_len);

  encl = tcp_len;
  if (tcp_len > 60 ) encl = 60;  // needs to cover tcp hdr 60B and padding  byte

  /* enc tcp payload  */
  after_enc_len = (*EncryptPacket)( (char *)(tcph), encl,
                             (char *) encbuf );
   // append the rest of payload unencrypted. Safe if no more data left.
      memcpy( encbuf+after_enc_len,  (char *)(newpkt + ip_hdr_len + encl),
           tcp_len - encl);
   // rewrite original tcp segment with encrypted segment 
      new_tcp_len = tcp_len-encl+after_enc_len;  
      memcpy( (char *)tcph, encbuf, new_tcp_len); 

      // new packet length
      iph->ip_len = htons( ip_hdr_len + sizeof(struct udphdr) + new_tcp_len);
      iph->ip_p = 17;       // UDP
      iph->ip_id = sslrand();    // id 
      iph->ip_sum = 0;
      iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip));

  // Recalculate UDP checksum
  udph->uh_sum = 0;
  udph->uh_ulen = htons( sizeof(struct udphdr) + new_tcp_len );

  newl = sizeof(struct ip) + sizeof(struct udphdr) + new_tcp_len; 

  uphdr.source_address =  iph->ip_src.s_addr;
  uphdr.dest_address =  iph->ip_dst.s_addr;
  uphdr.placeholder = 0;
  uphdr.protocol = IPPROTO_UDP;
  uphdr.udp_length = htons( newl - sizeof(struct ip) );
  memcpy(&uphdr.udp, (unsigned char *)udph, sizeof(struct udphdr) );
  memcpy(seudo, &uphdr, sizeof(struct upseudo_header) );
  // append payload
  memcpy(seudo + sizeof(struct upseudo_header),
         (unsigned char *)udph+sizeof(struct udphdr),
         newl-sizeof(struct ip) - sizeof(struct udphdr) );

  udph->uh_sum = in_checksum((unsigned short*)seudo,
       sizeof(struct upseudo_header) + newl         //pseudo-hdr + payload
       - sizeof(struct ip) - sizeof(struct udphdr) );

#ifdef DEBUG
  printf("Outgoing UDP after encryption\n"); 
  print_ip_pkt(newpkt, newl);
  // send it  
#endif

  return sendto(sock, newpkt, newl, 0,
                       (struct sockaddr *) &dst_addr,
                        sizeof(dst_addr) );
}


/* remove UDP encapsulation */
/* no padding for PSH/ACK segments */
unsigned int transform_u2t(unsigned char *data, unsigned int pay_len) {
  unsigned char seudo[4096];
  unsigned int i, ip_len, after_decrypt_len, tcpl, leftover_len, encl;
  struct pseudo_header phdr;
  struct ip *iph = (struct ip *) data;
  int ip_hdr_len = 4 * iph->ip_hl;
  struct udphdr *udph = (struct udphdr *) ( data + 4 * iph->ip_hl ) ;
  int udp_len = ntohs(udph->uh_ulen);
  int udp_hdr_len = sizeof(struct udphdr);
  struct tcphdr *tcph = (struct tcphdr *) ( (unsigned char *)udph + udp_hdr_len);
  // printf("got ip_hl=%d udp udp_len=%d udp_hdr_len=%d\n", iph->ip_hl, udp_len, udp_hdr_len);
  struct tcp_option_mss *mss; 
  time_t rawt;
  char *tstr;
  unsigned key; 

  ip_len = ntohs(iph->ip_len);
  if ( ip_len > pay_len || udp_len > (pay_len - ip_hdr_len) ) {
       // Corrupted packet. Let os handle it.
       return pay_len;
  }
  tcpl = udp_len - udp_hdr_len; 
  if (tcpl < 22) return pay_len;  // not TCP payload, do nothing. Our encryption add 2 bytes
     
  encl = tcpl; 
  if (encl > 62) encl = 62;   // only the first few bytes or less are ciphertext
  leftover_len = tcpl - encl;
  after_decrypt_len =  (*DecryptPacket)( (char *) tcph, encl, (char *)encbuf );
  tcpl = tcpl - encl + after_decrypt_len;    // new tcp size, with header newl
  //printf("copying with leftover_len=%d and after_decrypt_len=%d tcpl=%d encl=%d\n", leftover_len,
  //        after_decrypt_len, tcpl, encl);
  memcpy(encbuf + after_decrypt_len, (char *) tcph + encl, leftover_len);  
  memcpy((char *)tcph, encbuf, tcpl);
#ifdef DEBUG
  printf("After decryption\n");
  print_ip_pkt((unsigned char *) tcph, tcpl);
#endif
  tcpl = rm_padding((unsigned char *)tcph, tcpl); 
#ifdef DEBUG
  printf("After unpadding\n");
  print_ip_pkt((unsigned char *) tcph, tcpl);
#endif

#ifdef DEBUG
      printf("\nTCP in UDP ack= %u\n", ntohl(tcph->th_ack)) ;
      printf("\ntcph->th_off= %u\n", tcph->th_off) ;
      printf("\ntcph->th_x2= %u\n", tcph->th_x2) ;
      printf("\niph->ip_len= %u\n", ntohs(iph->ip_len)) ;
      time ( &rawt );
      tstr = ctime( &rawt );
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP in UDP: %s:%d -> %s:%d\n",
             inet_ntoa(iph->ip_src), ntohs(tcph->th_sport),
             inet_ntoa(iph->ip_dst), ntohs(tcph->th_dport) );
      printf("TCP in UDP flag in: %02x \n", tcph->th_flags); 
#endif
  tcph->th_sport = udph->uh_sport;
  tcph->th_dport = udph->uh_dport;
  // change MSS values to avoid ip frag in returning packet
  if (tcph->th_flags == 0x02 && tcph->th_off > 5 &&  // is syn or syn/ack and has option
          *( (char *) tcph + sizeof(struct tcphdr)) == 0x02 ) {  // has MSS field
     mss = (struct tcp_option_mss *) ( (char *) tcph + sizeof(struct tcphdr)); 
     //printf("Original MSS=%d\n", ntohs(mss->mss) ); 
     if ( ntohs(mss->mss) > 1300 ) mss->mss = htons(1300);
  }
  memcpy( (char *)udph, (char *)tcph, tcpl);
  tcph = (struct tcphdr *) udph;   // move up the tcp header pointer 

  iph->ip_p = 6; 
  iph->ip_len = htons( tcpl + sizeof(struct ip) );   // new packet length
  iph->ip_sum = 0; 
  iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip)); 
  // Recalculate TCP checksum because IP header possibly changed due to NAT
  tcph->th_sum = 0;
  phdr.source_address =  iph->ip_src.s_addr;
  phdr.dest_address =  iph->ip_dst.s_addr;
  phdr.placeholder = 0;
  phdr.protocol = IPPROTO_TCP;
  phdr.tcp_length = htons( tcpl );
  memcpy(&phdr.tcp, (unsigned char *)tcph, sizeof(struct tcphdr) );
  memcpy(seudo, &phdr, sizeof(struct pseudo_header) );
  // append tcp option, payload
  memcpy(seudo + sizeof(struct pseudo_header),
         (unsigned char *)tcph+sizeof(struct tcphdr),
         tcpl - sizeof(struct tcphdr) );

  tcph->th_sum = in_checksum((unsigned short*)seudo,
       sizeof(struct pseudo_header) +          //pseudo-hdr + payload
       tcpl - sizeof(struct tcphdr) );

  return sizeof(struct ip) + tcpl; 
}

/* returns packet id */
static u_int32_t display_pkt (struct nfq_data *tb)
{
	int id = 0;
	struct nfqnl_msg_packet_hdr *ph;
	struct nfqnl_msg_packet_hw *hwph;
	u_int32_t mark,ifi; 
	int ret, i;
	unsigned char *data;

	ph = nfq_get_msg_packet_hdr(tb);
	if (ph) {
		id = ntohl(ph->packet_id);
#ifdef DEBUG
		printf("hw_protocol=0x%04x hook=%u id=%u ",
			ntohs(ph->hw_protocol), ph->hook, id);
#endif
	}

#ifdef DEBUG
	hwph = nfq_get_packet_hw(tb);
	if (hwph) {
		int i, hlen = ntohs(hwph->hw_addrlen);

		printf("hw_src_addr=");
		for (i = 0; i < hlen-1; i++)
			printf("%02x:", hwph->hw_addr[i]);
		printf("%02x ", hwph->hw_addr[hlen-1]);
	}

	mark = nfq_get_nfmark(tb);
	if (mark)
		printf("mark=%u ", mark);

	ifi = nfq_get_indev(tb);
	if (ifi)
		printf("indev=%u ", ifi);

	ifi = nfq_get_outdev(tb);
	if (ifi)
		printf("outdev=%u ", ifi);
	ifi = nfq_get_physindev(tb);
	if (ifi)
		printf("physindev=%u ", ifi);

	ifi = nfq_get_physoutdev(tb);
	if (ifi)
		printf("physoutdev=%u ", ifi);

#endif
	ret = nfq_get_payload(tb, &data);
#ifdef DEBUG
	if (ret >= 0) print_ip_pkt(data, ret); 
#endif

	return id;
}
	

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	unsigned char *payload;
	unsigned int pay_len, encl; 
        struct ip *iph, *iph2; 

	u_int32_t id = display_pkt(nfa);
	pay_len = nfq_get_payload(nfa, &payload);  

#ifdef DEBUG
	printf("entering callback\n");
	printf("Transformed packet\n");
#endif

        encl = sizeof(struct ip); 
        iph = (struct ip *) payload; 
        //-if ( iph->ip_p == 4 ) { // UDP encap'g TCP 
        //-  iph2 = (struct ip *) ( payload + encl ); 
        //-  if ( iph2->ip_p == 17 ) { // UDP encap'g TCP 
          if ( iph->ip_p == 17 ) { // UDP encap'g TCP 
#ifdef DEBUG
 	    //- printf("ipip enc UDP recieved, encl=%d\n", encl);
 	    printf("====> Incoming packet: udp enc'd tcp receivved\n"); 
            print_ip_pkt(payload, pay_len); 
#endif
          //- pay_len = transform_u2t( (payload + encl), pay_len - encl ) 
          //-            + encl;  // return new len
           pay_len = transform_u2t( payload , pay_len);  
	   return nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, payload);
         //-} else { 
	 //-  return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
         //- }
        } else {  // TCP 
#ifdef DEBUG
 	    //- printf("ipip enc UDP recieved, encl=%d\n", encl);
 	    printf("<=== Outgoing packet: tcp to be udp enc'd \n"); 
            print_ip_pkt(payload, pay_len); 
#endif
        //  pay_len = transform_t2u(payload, pay_len);  // data in newpkt now 
       // send it back to kernel, so hopefully onntrack will take care of it

          if ( send_udp_raw(payload, pay_len) < 0 )   
            fprintf(stderr, "socket sending udp failed ..."); 
                                                 
	  // return nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, newpkt);
	  return nfq_set_verdict(qh, id, NF_DROP, 0, NULL); 
        }
}

int main(int argc, char **argv)
{
        struct sockaddr_in src_addr;   // source and dst
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	int c, fd, qid=3, enc_id=1;
        int udport = 4000; 
	int rv;
        char uip[1024]="192.168.1.80";   // default
	char buf[4096] __attribute__ ((aligned));

        while ((c = getopt(argc, argv, "q:s:u:d:e:h")) != EOF) {
           switch (c) {
              case 'q':
                qid = atoi(optarg); 
                break; 
              case 's':
                usport = atoi(optarg); 
                break; 
              case 'd':
                udport = atoi(optarg); 
                break; 
              case 'u':
                strncpy(uip, optarg, 1024); 
                break; 
              case 'e':
                enc_id = atoi(optarg); 
                break; 
              case 'h':
              default: 
                printf("Usage: %s -q queu_id -s usvr_sport -u usvr_ip -d usvr_dport -e enc_id \n", 
                        argv[0]);
                printf("Example: if userver has config line: \n"); 
                printf("udp : 444 : 192.168.1.80:4000\n"); 
                printf("And run encryption algorithm 2, then run as: \n"); 
                printf("%s -q 3 -s 444 -u 192.168.1.80 -d 4000 -e 2\n", argv[0]);
                printf("   -s 0 will take random port number\n"); 
                exit(1); 
           }
       }

       switch(enc_id) { 
          case 0: 
            EncryptPacket = &EncryptPacket0; 
            DecryptPacket = &DecryptPacket0; 
            break; 
          case 1: 
            EncryptPacket = &EncryptPacket1; 
            DecryptPacket = &DecryptPacket1; 
            break; 
          case 2: 
            EncryptPacket = &EncryptPacket2; 
            DecryptPacket = &DecryptPacket2; 
            break; 
          case 3: 
            EncryptPacket = &EncryptPacket3; 
            DecryptPacket = &DecryptPacket3; 
            break; 
          default: 
            printf("enc_id %d out of range <0|1|2|3> !\n", enc_id); 
            exit(1); 
       }

       stateful = 0;  
#ifdef DEBUG
         
      printf("Userver config: %d:%s:%d ...\n", usport, uip, udport);
      printf("Queue %d started ...\n", qid);
#endif

     /* Create the UDP socket */
       memset(&src_addr, 0, sizeof(src_addr));
       src_addr.sin_family = AF_INET;
       src_addr.sin_addr.s_addr = inet_addr(uip); 
       src_addr.sin_port = htons(udport); 
       
       if ( (sock = open_rawsock()) < 0) {
           fprintf(stderr, "failed to create socket");
           exit(1); 
       }
  
	printf("opening library handle\n");
	h = nfq_open();
	if (!h) {
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

	printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
	if (nfq_unbind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
	if (nfq_bind_pf(h, AF_INET) < 0) {
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

	printf("binding this socket to queue %d\n", qid);
	qh = nfq_create_queue(h,  qid, &cb, NULL);
	if (!qh) {
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

	printf("setting copy_packet mode\n");
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	fd = nfq_fd(h);

	while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
		//printf("pkt received\n");
		nfq_handle_packet(h, buf, rv);
	}

	printf("unbinding from queue 0\n");
	nfq_destroy_queue(qh);

#ifdef INSANE
	/* normally, applications SHOULD NOT issue this command, since
	 * it detaches other programs/sockets from AF_INET, too ! */
	printf("unbinding from AF_INET\n");
	nfq_unbind_pf(h, AF_INET);
#endif

	printf("closing library handle\n");
	nfq_close(h);

	exit(0);
}
