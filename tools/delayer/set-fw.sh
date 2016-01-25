
port=12345

/sbin/iptables -F

/sbin/iptables -I OUTPUT -m length -p tcp --length 200:4000 --sport $port --tcp-flags ALL ACK -j QUEUE
#/sbin/iptables -I OUTPUT -p tcp  --sport $port --tcp-flags ALL ACK -j QUEUE


# The following rule applies to tcp_transform v1.4 
# (client changes SYN to PSH/ACK)
#/sbin/iptables -I INPUT -p tcp  -m conntrack --dport 12345  --ctstate \! ESTABLISHED --tcp-flags ALL ACK,PSH -j QUEUE

# The following rule applies to tcp_transform v1.6 
# (server changes SYN/ACK to RST/ACK)
#/sbin/iptables -I OUTPUT -p tcp  --sport 12345  --tcp-flags ALL ACK,SYN -j QUEUE

exit

