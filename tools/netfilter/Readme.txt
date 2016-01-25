
6/4/2011

# list offload options:
[root@vadtw4 ~]# /usr/sbin/ethtool -k eth0

# turn off tso
[root@vadtw4 ~]# /usr/sbin/ethtool -K eth0 tso off


11/24/2010

1. Install libnfnetlink: 
=============================
 wget http://netfilter.org/projects/libnfnetlink/files/libnfnetlink-1.0.0.tar.bz2
 bunzip2 libnfnetlink-1.0.0.tar.bz2
 tar xvf libnfnetlink-1.0.0.tar
 cd libnfnetlink-1.0.0/
 ./configure
 make
 # su to root, and run "make install"


2. Install libnetfilter_queue: 
=============================

 wget http://netfilter.org/projects/libnetfilter_queue/files/libnetfilter_queue-1.0.0.tar.bz2
 bunzip2 libnetfilter_queue-1.0.0.tar.bz2
 tar xvf libnetfilter_queue-1.0.0.tar
 cd libnetfilter_queue-1.0.0/

 export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/home/oneway/netfilter/libnfnetlink-1.0.0
./configure
 make
 #  su to root and run "make install"

 # compile test program: 
 cd utils 
 make nfqnl_test

3. Testing 
=========================================================
[root@new-host oneway]# /sbin/iptables -I INPUT -p tcp --dport 21 -j QUEUE
[root@new-host oneway]# /sbin/iptables -I INPUT -p tcp --dport 80 -j QUEUE

# fire up the program, then use a browser to point to http://192.168.1.2
# from a PC on the LAN (port 80 is open to the LAN).

[root@new-host netfilter]# ./nfqnl_test
opening library handle
unbinding existing nf_queue handler for AF_INET (if any)
binding nfnetlink_queue as nf_queue handler for AF_INET
binding this socket to queue '0'
setting copy_packet mode
pkt received
hw_protocol=0x0008 hook=1 id=1 hw_src_addr=00:19:b9:4b:bc:13 indev=2 payload_len=48
entering callback
pkt received
hw_protocol=0x0008 hook=1 id=2 hw_src_addr=00:19:b9:4b:bc:13 indev=2 payload_len=40
entering callback
pkt received
hw_protocol=0x0008 hook=1 id=3 hw_src_addr=00:19:b9:4b:bc:13 indev=2 payload_len=428
entering callback
pkt received
hw_protocol=0x0008 hook=1 id=4 hw_src_addr=00:19:b9:4b:bc:13 indev=2 payload_len=40
entering callback


11/27/2010
========================================================================
tcp_transform.c is done. Testing. 

1. libnetfilter_queue can not be compiled on han3 ( Kernel has to be >=
2.6.14).

****** Trying to intercept lone (PUSH, ACK) packets ************
2. On vadtw4: 

# --- set up queue --------------- 
/sbin/iptables -I INPUT -p tcp  -m conntrack --dport 8443  --ctstate \! ESTABLISHED --tcp-flags ALL ACK,PSH -j QUEUE

# --- fire up tcp_transform, and on han3, send packets out : 

[root@han3 sample]# ./win-pa -s 72.52.66.12 -o 11234 -d 38.105.86.204 -p 8443
-P -A -O1260 -v

# --- tcp_transfor's output : 
[root@vadtw4 netfilter]# ./tcp_transform
opening library handle
unbinding existing nf_queue handler for AF_INET (if any)
binding nfnetlink_queue as nf_queue handler for AF_INET
binding this socket to queue '0'
setting copy_packet mode
pkt received
hw_protocol=0x0008 hook=1 id=1 hw_src_addr=00:0b:5f:cc:f6:80 indev=2
payload_len=40
                                           45 00
00 28 00 00 40 00 e2 06  91 5a 48 34 42 0c 26 69
56 cc 2b e2 20 fb 22 7a  05 c4 00 00 04 d2 50 18
15 fc 18 6e 00 00
entering callback


11/28/2010
========================================================================
 Now testing if the kernel receives the transformed packets correctly, 
and responds. 

1. Run a TCP server listening on port 8443: 

[oneway@vadtw4 ~]$ nc -v -v -l 38.105.86.204 8443

2. Set up queue: 

/sbin/iptables -F

/sbin/iptables -I INPUT -p tcp  -m conntrack --dport 8443  --ctstate \!
ESTABLISHED --tcp-flags ALL ACK,PSH -j QUEUE

3. Fire up transformer: 

[root@vadtw4 netfilter]# ./tcp_transform
opening library handle
unbinding existing nf_queue handler for AF_INET (if any)
binding nfnetlink_queue as nf_queue handler for AF_INET
binding this socket to queue '0'
setting copy_packet mode
pkt received
hw_protocol=0x0008 hook=1 id=1 hw_src_addr=00:0b:5f:cc:f6:80 indev=2
payload_len=40

TCP flag in: 18 TCP flag out: 02
                                           45 00
00 28 00 00 40 00 e2 06  57 43 ad 54 17 03 26 69
56 cc 3b 13 20 fb 60 65  e4 8a 00 00 00 00 50 02
3e 71 8e e6 00 00
entering callback

4. Send out a (PUSH, ACK) segment from another machine: 

[root@han3 sample]# ./win-pa -s 173.84.23.3 -d 38.105.86.204 -p 8443 -P -A -v
-O1260 -o 4321

5. Here is what we got from tcpdump: 

[root@vadtw4 ~]# /usr/sbin/tcpdump -i eth0 -nn "port 8443"
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on eth0, link-type EN10MB (Ethernet), capture size 96 bytes
09:59:33.438932 IP 173.84.23.3.4321 > 38.105.86.204.8443: P ack 1663380823 win
41085
09:59:33.440685 IP 38.105.86.204.8443 > 173.84.23.3.4321: S
1189875695:1189875695(0) ack 1513141761 win 5840 <mss 1460>
09:59:37.400836 IP 38.105.86.204.8443 > 173.84.23.3.4321: S
1189875695:1189875695(0) ack 1513141761 win 5840 <mss 1460>
09:59:43.399913 IP 38.105.86.204.8443 > 173.84.23.3.4321: S
1189875695:1189875695(0) ack 1513141761 win 5840 <mss 1460>


6. Conclusion: The incoming (PUSH, ACK) segment was correctly transformed 
into SYN and passed it to kernel, and the application is responding to 
the "SYN." ,


