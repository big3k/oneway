
/* ================================================================
  $Id: tcp_transform.c,v 1.8 2011/05/23 16:30:28 oneway Exp oneway $   
  Transform a TCP packet per "TCP Transformer Design V1.0 (12/10/2010)."
  First setp up a queue, e.g. 
 /sbin/iptables -I OUTPUT -p tcp --dport 9999 -j QUEUE
 To compile: gcc -o tcp_transform tcp_transform.c -lssl -lnetfilter_queue
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

#include <openssl/rand.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define __FAVOR_BSD
#include <netinet/udp.h>
#include <netinet/tcp.h>

/* global variables */
int stateful = 0;    // 1: transforms psh/ack after seeing ack in 
                     // 0: transforms psh/ack anyway 

struct pseudo_header {   //needed for checksum calculation
        unsigned long source_address;
        unsigned long dest_address;
        unsigned char placeholder;
        unsigned char protocol;
        unsigned short tcp_length;
        //char tcp[28];
        struct tcphdr tcp;
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
    int i; 

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

unsigned int transform_udp(unsigned char *data, unsigned int ip_len) {
  unsigned char seudo[4096];
  unsigned char rndc[512];
  unsigned char sip[256];
  unsigned char dip[256];
  int i, rndl, tcpl;
  // craft typical win tcp options
  unsigned char opt[] = "\x02\x04\x04\xec\x01\x01\x04\x02";
  struct pseudo_header phdr;
  struct ip *iph = (struct ip *) data;
  struct udphdr *udph = (struct udphdr *) ( data + sizeof(struct ip) ) ;
  struct tcphdr *tcph = (struct tcphdr *) ( data + sizeof(struct ip) + 
                                                   sizeof(struct udphdr) );
  time_t rawt;
  char *tstr;
  unsigned key; 

  /* decrypting payload */ 
  tcpl = ip_len - sizeof(struct ip) - sizeof(struct udphdr); 
  key = ( ( ip_len * ip_len ) % 127 ) + 1; 
  tstr = (char *) tcph; 
  for (i=0; i<tcpl; i++) tstr[i] = tstr[i] ^ key;  
  /* Debug print */
      printf("\nTCP in UDP ack= %u\n", ntohl(tcph->th_ack)) ;
      printf("\ntcph->th_off= %u\n", tcph->th_off) ;
      printf("\ntcph->th_x2= %u\n", tcph->th_x2) ;
      printf("\niph->ip_len= %u\n", ntohs(iph->ip_len)) ;
  /*  end of debug  */
#ifdef DEBUG
      time ( &rawt );
      tstr = ctime( &rawt );
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP in UDP: %s:%d -> %s:%d\n",
             sip, ntohs(tcph->th_sport),
             dip, ntohs(tcph->th_dport) );
      printf("TCP in UDP flag in: %02x \n", tcph->th_flags); 
#endif
  // Get rid of UDP encap, ignore any TCP payload
  tcpl = tcph->th_off * 4;    // header length only
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


unsigned int transform_tcp(unsigned char *data, unsigned int ip_len) { 
  unsigned char seudo[4096];
  unsigned char rndc[512];
  unsigned char sip[256]; 
  unsigned char dip[256]; 
  int i;
  unsigned int rndl; 
  // craft typical win tcp options
  unsigned char opt[] = "\x02\x04\x04\xec\x01\x01\x04\x02"; 
  struct pseudo_header phdr; 
  static unsigned long lastseq;   // save the seq # of the incoming ack 
  struct ip *iph = (struct ip *) data;
  struct tcphdr *tcph = (struct tcphdr *) ( data + sizeof(struct ip) );
  time_t rawt;
  char *tstr; 

  /* Debug print 
      printf("\nTCP ack= %u\n", ntohl(tcph->th_ack)) ; 
      printf("\ntcph->th_off= %u\n", tcph->th_off) ; 
      printf("\ntcph->th_x2= %u\n", tcph->th_x2) ; 
      printf("\niph->ip_len= %u\n", ntohs(iph->ip_len)) ; 
  */
  // Transform 1: incoming ACK to SYN 
  strcpy( sip, inet_ntoa(iph->ip_src) ); 
  strcpy( dip, inet_ntoa(iph->ip_dst) ); 

  if (tcph->th_flags == 0x10 ) {
      
#ifdef DEBUG
      time ( &rawt ); 
      tstr = ctime( &rawt );
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP: %s:%d -> %s:%d\n", 
             sip, ntohs(tcph->th_sport), 
             dip, ntohs(tcph->th_dport) );  
      printf("TCP flag in: %02x TCP flag out: %02x\n", tcph->th_flags, TH_SYN); 
#endif
      tcph->th_flags = 0x02; 
      tcph->th_ack = htonl(0);
      lastseq = ntohl(tcph->th_seq);    //save it for matching the return SYN/ACK 
      tcph->th_off = 0x7;   // added 4x2 bytes of TCP option
      tcph->th_x2 = 0;
      // append tcp option 
      memcpy( (unsigned char *)tcph+sizeof(struct tcphdr), opt, 8);  
      ip_len =  sizeof(struct ip) +  sizeof(struct tcphdr) + 8; 
      iph->ip_len = htons(ip_len);   // new packet length
      iph->ip_sum = 0; 
      iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip)); 
       
  // Transform 2: when stateful, outgoing SYN/ACK to PSH/ACK, 
  //              if the incoming one is ACK
  //             otherwise 
  // also set ip_id to random number. 
  } else if (   ( tcph->th_flags == 0x12 && ntohl(tcph->th_ack) -1 == lastseq 
                                         && stateful == 1 )
             || ( tcph->th_flags == 0x12 && stateful == 0 ) ) { 
#ifdef DEBUG
      time ( &rawt );
      tstr = ctime( &rawt );
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP: %s:%d -> %s:%d\n", 
             sip, ntohs(tcph->th_sport), 
             dip, ntohs(tcph->th_dport) );  
      printf("TCP flag in: %02x TCP flag out: %02x\n", tcph->th_flags, 
               TH_ACK|TH_PUSH); 
#endif
      tcph->th_flags = TH_ACK; 
      tcph->th_off = 0x5;   // cut off TCP option
      tcph->th_x2 = 0;
      RAND_bytes((unsigned char *)&(tcph->th_win), sizeof(u_int16_t));
      tcph->th_win &= 0xF000; // make sure it is at least 0xF000
      RAND_bytes((unsigned char *)&rndl, sizeof(int));
      rndl = 0;
	  //rndl = rndl%256 + 32; 
      //for (i=0; i<rndl; i++) rndc[i] = rand()%256;
      RAND_bytes((unsigned char *)rndc, rndl);
      memcpy( (unsigned char *)tcph+sizeof(struct tcphdr), rndc, rndl);  
      ip_len =  sizeof(struct ip) +  sizeof(struct tcphdr) + rndl;
      iph->ip_len = htons(ip_len);   // new packet length
      iph->ip_id = rand(); 
      iph->ip_sum = 0;
      iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip));

  } else { 
#ifdef DEBUG
      time ( &rawt );
      tstr = ctime( &rawt );
      for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
      printf("TCP: %s:%d -> %s:%d\n", 
             sip, ntohs(tcph->th_sport), 
             dip, ntohs(tcph->th_dport) );  
      printf("No transform. TCP flag in: %02x TCP flag out: %02x\n", tcph->th_flags, 
               tcph->th_flags); 
#endif
      return ip_len;  // do nothing
  }

  // Recalculate tcp checksum
  tcph->th_sum = 0;
  phdr.source_address =  iph->ip_src.s_addr; 
  phdr.dest_address =  iph->ip_dst.s_addr; 
  phdr.placeholder = 0;
  phdr.protocol = IPPROTO_TCP;
  phdr.tcp_length = htons( ip_len - sizeof(struct ip) );
  memcpy(&phdr.tcp, (unsigned char *)tcph, sizeof(struct tcphdr) );
  memcpy(seudo, &phdr, sizeof(struct pseudo_header) ); 
  // append tcp option, payload
  memcpy(seudo + sizeof(struct pseudo_header), 
         (unsigned char *)tcph+sizeof(struct tcphdr),
         ip_len-sizeof(struct ip) - sizeof(struct tcphdr) ); 

  tcph->th_sum = in_checksum((unsigned short*)seudo,
       sizeof(struct pseudo_header) + ip_len         //pseudo-hdr + payload
       - sizeof(struct ip) - sizeof(struct tcphdr) );

  return ip_len; 
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
	unsigned int pay_len; 
        struct ip *iph; 

	u_int32_t id = display_pkt(nfa);
	pay_len = nfq_get_payload(nfa, &payload);  
        iph = (struct ip *) payload; 
        if ( iph->ip_p == 17 ) { // UDP encap'g TCP SYN 
          pay_len = transform_udp(payload, pay_len);  // return new len
        } else {  // TCP 
          pay_len = transform_tcp(payload, pay_len);  // return new len
        }
#ifdef DEBUG
	printf("entering callback\n");
	printf("Transformed packet\n");
        print_ip_pkt(payload, pay_len); 
#endif
	return nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, payload);
}

int main(int argc, char **argv)
{
	struct nfq_handle *h;
	struct nfq_q_handle *qh;
	struct nfnl_handle *nh;
	int fd;
	int rv;
	char buf[4096] __attribute__ ((aligned));

        if(argc != 2) {
          printf("Usage: %s <-s|-u> \n", argv[0]);
          printf("       -s: transform psh/ack after seeing ack in\n"); 
          printf("       -u: transform psh/ack anyway (default) \n"); 
          exit(-1);
        }

       stateful = 0;  
       if ( strncmp (argv[1], "-s", 2) == 0 ) {
          stateful = 1; 
#ifdef DEBUG
	  printf("Stateful tranform started ...\n");
#endif
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

	printf("binding this socket to queue '0'\n");
	qh = nfq_create_queue(h,  0, &cb, NULL);
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
		printf("pkt received\n");
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
