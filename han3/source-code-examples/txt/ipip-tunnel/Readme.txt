
9/17/2006

* Add/del alias IP to an interface
/sbin/ip addr add 63.209.222.71/24 dev eth0
/sbin/ip addr delete 63.209.222.71/24 dev eth0


* Set up tunneling between big3k and johnfax: 

unode:  big3k: 70.16.31.83
userver:  johnfax: 66.98.154.149


(tried to use phx2 as userver, but it seems router firewall is 
blocking ipip packets).

On big3k: 

[root@plain root]# modprobe ipip

/sbin/ip tunnel add tun-unode mode ipip remote 66.98.154.149 dev eth0
/sbin/ifconfig tun-unode 192.168.3.100
# route 192.168.2.100 traffic via tunnel
/sbin/ip route add  192.168.2.100 dev tun-unode
# see packets on big3k:
/usr/sbin/tcpdump -i eth0 -n "host 66.98.154.149"
# while 
[root@plain root]# nc -v -v -n 192.168.2.100 25
# and get: 
11:00:24.905950 70.16.31.83 > 66.98.154.149: 192.168.3.100.54621 >
192.168.2.100.smtp: S 3955613213:3955613213(0) win 5760 <mss
1440,sackOK,timestamp 264084505 0,nop,wscale 0> (DF) (ipip-proto-4)

# and when sniffing on the tun interface, 
/usr/sbin/tcpdump -i tun-unode -n
# we get: 
11:17:12.108372 192.168.3.100.56759 > 192.168.2.100.smtp: S
732255603:732255603(0) win 5760 <mss 1440,sackOK,timestamp 264185225
0,nop,wscale 0> (DF)



On johnfax: 
# set up interface. (can we omit "remote ip"?)
/sbin/ip tunnel add tun-userver mode ipip remote 70.16.31.83 dev eth0
# add IP
ifconfig tun-userver 192.168.2.100

# Now ping from unode: 
[root@plain root]# ping 192.168.2.100

# we get on johnfax: 
[root@plesk7 oneway]# /usr/sbin/tcpdump -i tun-userver -n
tcpdump: listening on tun-userver
00:49:42.044105 192.168.3.100 > 192.168.2.100: icmp: echo request (DF)
00:49:43.055399 192.168.3.100 > 192.168.2.100: icmp: echo request (DF)
00:49:44.055374 192.168.3.100 > 192.168.2.100: icmp: echo request (DF)

Note: on johnfax, if the tunnel "remote 70.16.31.83" ip does not 
match the client's IP, the ipip tunnel won't receive the packets, 
but eth0 still receives the encapsulated IP packet. 


# *** Now try to route node traffic to an arbitrary ip on the physical 
# interface on u-server: 
# add a fictionous IP to the physical interface: 
 /sbin/ip addr add 63.209.222.71 dev eth0

# on unode, let traffic to server go through tunnel: 
 /sbin/route add -host 63.209.222.71 gw 192.168.3.100 dev tun-unode

#Now on server, fire up tcpdump on eth0, 
#And on client, do: 

[root@localhost oneway]# nc -v -v -n 63.209.222.71 110

# Now the u-server tcpdump shows: 

tcpdump: listening on eth0
12:40:57.595200 63.209.222.71.pop3 > 192.168.3.100.39388: S
2770304021:2770304021(0) ack 103880839 win 5792 <mss 1460,sackOK,timestamp
1170149458 248555951,nop,wscale 0> (DF)

#The unode has IP 192.168.1.100 on the physical interface eth0, so we try:

[root@localhost oneway]# nc -v -v -s 192.168.1.100 -n 63.209.222.71 110

#Server got: 
tcpdump: listening on eth0
12:47:06.037453 63.209.222.71.pop3 > 192.168.1.100.39398: S
3153353550:3153353550(0) ack 490002048 win 5792 <mss 1460,sackOK,timestamp
1170186302 248744669,nop,wscale 0> (DF)

# So the routing on the server side works, and the IP packet was delivered
from tunnel to eth0 unwrapped!


# Now trying to route traffic destined to port 9999 on unode to userver.

# on unode (big3k): 

iptables -A PREROUTING -i eth0 -t mangle -p tcp --dport 9999 -j MARK --set-mark 2

# to add log
/sbin/iptables -A PREROUTING -i eth0 -t mangle -p tcp --dport 9999 -j LOG --log-level debug

echo 202 unode.out >> /etc/iproute2/rt_tables
ip rule add fwmark 2 table unode.out
ip route add default via 192.168.3.100 dev tun-unode table unode.out

# note: maybe it shoud be "via 192.168.2.100", the other end of the
# ppp link

#(to delete: ip route del table unode.out)

## diagnostic commands:
iptables -t mangle -L
ip rule ls
ip route list table unode.out
ip addr
ip route


# but traffic to 9999 was not routed to the tunnel inteface. 
# Bill said that is because the fwmark rule can not procede
# rule 0, which is local ip forwarding. 

[root@localhost etc]# ip rule show
0:      from all lookup local
32765:  from all fwmark 0x2 lookup unode.out
32766:  from all lookup main
32767:  from all lookup 253


### further reading

http://www.linuxvirtualserver.org/VS-IPTunneling.html

http://lists.netfilter.org/pipermail/netfilter/2000-November/006089.html :
Testing here reveals that the route filtering and mark don't play well
together.  Try:

# for f in /proc/sys/net/ipv4/conf/*/rp_filter; do echo 0 > $f; done
# echo 1 > /proc/sys/net/ipv4/route/flush

Then run your tests again.  If that's the problem, just disable route
filtering on the interface where the replies to the marked packets
come in.
