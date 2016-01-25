
# node 65.49.2.133 will forward traffic to vadtw4
# on server vadtw4, run

#/sbin/iptables -F
#/sbin/iptables -I INPUT -p udp --dport 4000 -j NFQUEUE --queue-num 3
#/sbin/iptables -I OUTPUT -p tcp -s 192.168.1.81 --sport 4000 -j NFQUEUE --queue-num 3

# then start 

# ./u2u_transform-debug -q 3 -s 444 -u 192.168.1.81 -d 4000 -e 0

# on big3k (client), run the following to send packet to node

cat syn.pad.dat | nc -w 1 -n -n -u 65.49.2.133 444
