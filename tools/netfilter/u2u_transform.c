
/* ================================================================
   $Id: u2u_transform.c,v 1.19 2011/06/10 03:46:20 oneway Exp oneway $
   1. Decapsulates incoming UDP to TCP
   2. Encapsulates outgoing TCP to UDP 
   
   In 1, incoming UDP has payload padding and encryption, except 
   if the payload is PSH/ACK, no padding, only encryption. 
   In 2, no padding/encryption yet. 
   
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
   
   Billx: to compile: install related rpm, then
   gcc -g  -O2 -o u2u_transform u2u_transform.c sj-enc.c -lnetfilter_queue -lssl
   gcc -DDEBUG  -g  -O2 -o u2u_transform u2u_transform.c sj-enc.c -lnetfilter_queue -lssl
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
#include "aes.h"

#include <openssl/rand.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

#define __FAVOR_BSD
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include "u2u_transform.h"


static unsigned int sslrand()
{
    unsigned int randint;
    RAND_bytes((unsigned char*) &randint, sizeof(int));
    return randint;
}

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

void fill_rand(unsigned char * buf, int rand_len)
{
    RAND_bytes(buf, rand_len);
    return;
}

static int get_pad_len(int tcp_len)
{
    if ( tcp_len >= 200 ) {
        return sslrand() % 100 + 1;
    }
    else {
        return sslrand() % 80 + 15;
    }
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

int get_tcp_hdr_len(char *buf)
{
    struct tcphdr *tcph = (struct tcphdr *)buf;
    int tcpHdrLen = tcph->th_off * 4;
    if (tcpHdrLen > MAX_TCP_HDR_LEN || tcpHdrLen < MIN_TCP_HDR_LEN) {
        return -1;
    }
    return tcpHdrLen ;
}


/* sends out a UDP via regular socket,  so hopefully 
   conntrack can fix it. input newpkt is a TCP segment w/ ip header
*/
unsigned int send_udp(unsigned char *newpkt, unsigned int pay_len)
{
    struct sockaddr_in dst_addr;   // source and dst 
    struct ip *iph = (struct ip *) newpkt;
    int ip_hdr_len = 4 * iph->ip_hl;
    int ip_len = ntohs(iph->ip_len);
    struct tcphdr *tcph = (struct tcphdr *) ( newpkt + ip_hdr_len ) ;
    struct tcp_option_mss *mss; 
    memset(&dst_addr, 0, sizeof(dst_addr)); 
    dst_addr.sin_family = AF_INET; 
    dst_addr.sin_addr.s_addr = iph->ip_dst.s_addr;
    dst_addr.sin_port = tcph->th_dport; 
    tcph->th_sport = htons(usport); 
    
    if ( ip_len > pay_len ) {
        // Corrupted packet. Do nothing.
        return pay_len;
    }

    int tcp_len = ip_len - ip_hdr_len;
    if ( tcp_len < MIN_TCP_HDR_LEN ) {
        // corrupted packet. Do nothing.
        return pay_len;
    }
    
#ifdef DEBUG
    print_ip_pkt((unsigned char *)tcph, tcp_len); 
#endif

    // change MSS values to avoid ip frag in returning packet
    if (tcph->th_flags == 0x12) {  // has MSS field
        mss = (struct tcp_option_mss *) ( (char *) tcph + sizeof(struct tcphdr));
#ifdef DEBUG
        printf("Outgoing Original MSS=%d\n", ntohs(mss->mss) );
        print_ip_pkt((unsigned char *)tcph, tcp_len); 
#endif
        if ( ntohs(mss->mss) > 1340 ) mss->mss = htons(1340);
    }
    
    int pad_len = get_pad_len(tcp_len);
    tcp_len = add_padding((unsigned char *)tcph, tcp_len, pad_len);

    // needs to cover tcp hdr 60B and padding  byte
    int encl = get_enc_len(tcp_len);
    int after_enc_len = (*EncryptPacket)( (char *)(tcph), &encl, (char *) encbuf );
    if (after_enc_len  < 0 ) {
        return pay_len;
    }
    
#ifdef DEBUG
    printf("Outgoing UDP: tcp header pad=%d in: %d, out: %d\n", pad_len, tcp_len, tcp_len - encl + after_enc_len); 
    if (after_enc_len >= 0) print_ip_pkt(encbuf, after_enc_len); 
#endif
    // append the rest of payload unencrypted. Safe if no more data left.
    memcpy( encbuf+after_enc_len,  (char *)(newpkt + ip_hdr_len + encl), 
            tcp_len - encl);   
    return sendto(sock, encbuf, tcp_len-encl+after_enc_len, 0, 
                  (struct sockaddr *) &dst_addr, 
                  sizeof(dst_addr) );
}


/* remove UDP encapsulation */
/* no padding for PSH/ACK segments */
unsigned int transform_u2t(unsigned char *data, unsigned int pay_len)
{
    unsigned char seudo[4096];
    struct pseudo_header phdr;
    struct ip *iph = (struct ip *) data;
    int ip_hdr_len = 4 * iph->ip_hl;
    struct udphdr *udph = (struct udphdr *) ( data + 4 * iph->ip_hl ) ;
    int udp_len = ntohs(udph->uh_ulen);
    int udp_hdr_len = sizeof(struct udphdr);
    struct tcphdr *tcph = (struct tcphdr *) ( (unsigned char *)udph + udp_hdr_len);
    // printf("got ip_hl=%d udp udp_len=%d udp_hdr_len=%d\n", iph->ip_hl, udp_len, udp_hdr_len);
    struct tcp_option_mss *mss; 
    
    int ip_len = ntohs(iph->ip_len);
    if ( ip_len > pay_len || udp_len > (pay_len - ip_hdr_len) ) {
        // Corrupted packet. Let os handle it.
        return pay_len;
    }

    int tcpl = udp_len - udp_hdr_len; 
    if (tcpl < get_enc_tcp_min_len()) {
        return pay_len;  // not TCP payload, do nothing. 
    }
    
    // only the first few bytes or less are ciphertext
    int encl = get_dec_len(tcpl);
    int after_decrypt_len =  (*DecryptPacket)( (char *) tcph, &encl, (char *)encbuf );
    if (after_decrypt_len  < 0 ) {
        return pay_len;
    }
    int leftover_len = tcpl - encl;
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
    time_t rawt;
    time ( &rawt );
    char *tstr = ctime( &rawt );
    int i = 0;
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


/* remove UDP encapsulation to a TCP segment */
unsigned int transform_t2u(unsigned char *data, unsigned int ip_len)
{ 
    unsigned char seudo[4096];
    struct upseudo_header uphdr; 
    
    // point to the new data structure, newpkt. global var
    struct ip *iph = (struct ip *) newpkt;
    struct udphdr *udph = (struct udphdr *) ( newpkt + sizeof(struct ip) ) ;
    struct tcphdr *tcph = (struct tcphdr *) ( newpkt + sizeof(struct ip) +
                                              sizeof(struct udphdr) );
    int tcpl = ip_len - sizeof(struct ip); 
    int newl = ip_len + sizeof(struct udphdr); 
    memcpy( (char *)iph, data, sizeof(struct ip) );
    memcpy( (char *)tcph, data + sizeof(struct ip), tcpl); 
    udph->uh_sport = tcph->th_sport; 
    udph->uh_dport = tcph->th_dport; 
    udph->uh_ulen = htons( sizeof(struct udphdr) + tcpl ); 
    udph->uh_sum  = 0;
    
    // padding and encryption 
    
#ifdef DEBUG
    unsigned char sip[256]; 
    unsigned char dip[256]; 
    time_t rawt;
    time ( &rawt ); 
    char *tstr = ctime( &rawt );
    int i = 0;
    for (i=4; i<strlen(tstr)-5; i++) putchar(tstr[i]);
    printf("TCP: %s:%d -> %s:%d\n", 
           sip, ntohs(tcph->th_sport), 
           dip, ntohs(tcph->th_dport) );  
    printf("TCP flag in: %02x TCP flag out: %02x\n", tcph->th_flags, TH_SYN); 
#endif
    
    iph->ip_len = htons(newl);   // new packet length
    iph->ip_p = 17;       // UDP
    iph->ip_sum = 0; 
    iph->ip_sum = in_checksum((unsigned short*)iph, sizeof(struct ip)); 
    
    // Recalculate UDP checksum
    udph->uh_sum = 0;
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
    
    return newl; 
}

/* returns packet id */
static u_int32_t display_pkt (struct nfq_data *tb)
{
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(tb);
    if (ph) {
        id = ntohl(ph->packet_id);
#ifdef DEBUG
        printf("hw_protocol=0x%04x hook=%u id=%u ",
               ntohs(ph->hw_protocol), ph->hook, id);
#endif
    }
    
#ifdef DEBUG
    struct nfqnl_msg_packet_hw *hwph = nfq_get_packet_hw(tb);
    if (hwph) {
        int i, hlen = ntohs(hwph->hw_addrlen);
        
        printf("hw_src_addr=");
        for (i = 0; i < hlen-1; i++)
            printf("%02x:", hwph->hw_addr[i]);
        printf("%02x ", hwph->hw_addr[hlen-1]);
    }
    
    u_int32_t mark = nfq_get_nfmark(tb);
    if (mark)
        printf("mark=%u ", mark);
    
    u_int32_t ifi = nfq_get_indev(tb);
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

#ifdef DEBUG
    unsigned char *data;
    int ret = nfq_get_payload(tb, &data);
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
    
#ifdef DEBUG
    printf("entering callback\n");
    printf("Transformed packet\n");
    print_ip_pkt(payload, pay_len); 
#endif
    
    iph = (struct ip *) payload; 
    //-if ( iph->ip_p == 4 ) { // UDP encap'g TCP 
    //-  iph2 = (struct ip *) ( payload + encl ); 
    //-  if ( iph2->ip_p == 17 ) { // UDP encap'g TCP 
    if ( iph->ip_p == 17 ) { // UDP encap'g TCP 
#ifdef DEBUG
        //- printf("ipip enc UDP recieved, encl=%d\n", encl);
        printf("udp enc tcp receivved\n"); 
#endif
        //- pay_len = transform_u2t( (payload + encl), pay_len - encl ) 
        //-            + encl;  // return new len
        pay_len = transform_u2t( payload , pay_len);  
        return nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, payload);
        //-} else { 
        //-  return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
        //- }
    } else {  // TCP 
        //  pay_len = transform_t2u(payload, pay_len);  // data in newpkt now 
        // send it back to kernel, so hopefully onntrack will take care of it
        if ( send_udp(payload, pay_len) < 0 )   
            fprintf(stderr, "socket sending udp failed ..."); 
        
        //return nfq_set_verdict(qh, id, NF_ACCEPT, pay_len, newpkt);
        return nfq_set_verdict(qh, id, NF_DROP, 0, NULL); 
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in src_addr;   // source and dst
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle;
    int c, fd, qid=3;
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
    case 4: 
        EncryptPacket = &EncryptPacket4; 
        DecryptPacket = &DecryptPacket4; 
        break; 
    case 5: 
        EncryptPacket = &EncryptPacket5; 
        DecryptPacket = &DecryptPacket5; 
        break; 
    default: 
        printf("enc_id %d out of range <0|1|2|3|4|5> !\n", enc_id); 
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
    
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "failed to create socket");
        exit(1); 
    }
    
    if ( bind(sock, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0 ){ 
        fprintf(stderr, "failed to bind  socket");
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
