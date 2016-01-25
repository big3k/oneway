#!/bin/bash
# script to update DNS server zone file and reload server. 
# need to be root to do it. 
# 11/9/2006: simplify: assuming all nodes are using numeric IPs
path=/home/oneway/phoenix
# get all the nodes in the all-nodes.txt file 
$path/get-unodes.sh
# send UDP 18563 packets to nodes and get results fron han3
$path/run-win-pa.sh
node_list=$path/good-unodes.txt
scp oneway@han3:good-unodes.txt $node_list 
# script to generate a shuffled list
rseq=$path/rseq.sh
# script to stuff proxy to mx
proxy=$path/get-proxies.sh
# script to stuff proxy to mx
ip_enc=$path/ip-enc.sh
# original version of netcat. GNU version's -w option does not work 
##temp nc=$path/nc
nc=true 
# max nodes put in DNS
maxn=5

zonefile=/var/named/chroot/var/named/clipurl.org

ttl=423
port=1863
cat > $zonefile <<EOF
\$TTL $ttl
@       IN      SOA     ns1.clipurl.org. admin.clipurl.org. (
                        200602151       ; serial, todays date + todays serial #
                        8H              ; refresh, seconds
                        2H              ; retry, seconds
                        4W              ; expire, seconds
                        1D )            ; minimum, seconds
                        NS      ns1
                        NS      ns2

localhost       A       127.0.0.1
ns1             A       65.59.220.28
ns2             A       63.209.222.223

svr		A	188.240.217.185
		A	190.196.35.238

EOF

id=1
nodes=( $(cat $node_list) ) 
for idx in `$rseq 0 $((${#nodes[@]} - 1))`; do 
   ip=${nodes[$idx]}
   $nc -w 3 -z $ip $port 
   if [ $? -eq 0 ]; then
    echo $ip
    echo "www            A        $ip" >> $zonefile
    wget -O /dev/null http://dynupdate.no-ip.com/dns?username=leoy@epochtimes.com\&password=KvTtbU\&hostname=freedom.no-ip.net@SRV$id\&ip=$ip
    let id=id+1 
   fi
   if [ $id -gt $maxn ]; then
    break
   fi
done

id=1
for idx in `$rseq 0 $((${#nodes[@]} - 1))`; do
   ip=${nodes[$idx]}
   $nc -w 3 -z $ip $port
   if [ $? -eq 0 ]; then
    echo $ip
    echo "uuu            A        $ip" >> $zonefile
#    wget -O /dev/null http://dynupdate.no-ip.com/dns?username=leoy@epochtimes.com\&password=KvTtbU\&hostname=freedom.no-ip.net@SRV$id\&ip=$ip
    let id=id+1
   fi
   if [ $id -gt $maxn ]; then
    break
   fi
done

# add new domains with encrypted IPs for FPX nodes

for host in a b c d e f g h; do 
  maxn=$(( $RANDOM % 3 + 3 ))     ## 3, 4, 5
  id=1
  for idx in `$rseq 0 $((${#nodes[@]} - 1))`; do
   node=${nodes[$idx]}
   $nc -w 3 -z $node $port
   if [ $? -eq 0 ]; then
    ip=`$ip_enc $node`   # encrypt IP 
    echo $host :  $ip
    echo "$host            A        $ip" >> $zonefile
    wget -O /dev/null http://dynupdate.no-ip.com/dns?username=leoy@epochtimes.com\&password=KvTtbU\&hostname=$host.homesecuritymac.com@SRV$id\&ip=$ip
    let id=id+1
   fi
   if [ $id -gt $maxn ]; then
    break
   fi
  done
done   # for host

# stuff proxy list now
#  $proxy

if [ $id -gt $maxn ]; then
 /etc/init.d/named reload
fi

