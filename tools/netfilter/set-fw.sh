
port=80

/sbin/iptables -F

#===== The following rule applies to transformer design v1.0 (12/10/2010) 

/sbin/iptables -I INPUT -p tcp  -m conntrack --dport $port  --ctstate \! ESTABLISHED --tcp-flags ALL ACK -j QUEUE

#=== it seems not work
#== /sbin/iptables -I OUTPUT -p tcp  -m conntrack --sport $port --ctstate \! ESTABLISHED --tcp-flags ALL SYN,ACK -j QUEUE

#== No session tracking, works 
/sbin/iptables -I OUTPUT -p tcp --sport $port --tcp-flags ALL SYN,ACK -j QUEUE


# The following rule applies to tcp_transform v1.4 
# (client changes SYN to PSH/ACK)
#/sbin/iptables -I INPUT -p tcp  -m conntrack --dport 12345  --ctstate \! ESTABLISHED --tcp-flags ALL ACK,PSH -j QUEUE

# The following rule applies to tcp_transform v1.6 
# (server changes SYN/ACK to RST/ACK)
#/sbin/iptables -I OUTPUT -p tcp  --sport 12345  --tcp-flags ALL ACK,SYN -j QUEUE

exit

