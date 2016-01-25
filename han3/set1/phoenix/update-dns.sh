#!/bin/bash
# script to update DNS server zone file and reload server. 
# need to be root to do it. 
path=/home/oneway/phoenix
# get all the nodes in the all-nodes.txt file 
$path/get-unodes.sh
node_list=$path/all-nodes.txt
# script to generate a shuffled list
rseq=$path/rseq.sh
# script to stuff proxy to mx
proxy=$path/get-proxies.sh
# original version of netcat. GNU version's -w option does not work 
nc=$path/nc
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
   node=${nodes[$idx]}
   ip=`/usr/bin/host $node | cut -d ' ' -f 4`
   # if $node is already an IP address, use it
   if [ "$ip" = "pointer" -o "$ip" = "found:" ];  then
     ip=$node
   fi 
   $nc -w 3 -z $ip $port 
   if [ $? -eq 0 ]; then
    echo $node
    echo "www1            A        $ip" >> $zonefile
#    wget -O /dev/null http://dynupdate.no-ip.com/dns?username=leoyue@gmail.com\&password=KvTtbU\&hostname=freedom.no-ip.net@SRV$id\&ip=$ip
    let id=id+1 
   fi
   if [ $id -gt $maxn ]; then
    break
   fi
done

id=1
for idx in `$rseq 0 $((${#nodes[@]} - 1))`; do
   node=${nodes[$idx]}
   ip=`/usr/bin/host $node | cut -d ' ' -f 4`
   # if $node is already an IP address, use it
   if [ "$ip" = "pointer" -o "$ip" = "found:" ];  then
     ip=$node
   fi
   $nc -w 3 -z $ip $port
   if [ $? -eq 0 ]; then
    echo $node
    echo "www2            A        $ip" >> $zonefile
#    wget -O /dev/null http://dynupdate.no-ip.com/dns?username=leoyue@gmail.com\&password=KvTtbU\&hostname=freedom.no-ip.net@SRV$id\&ip=$ip
    let id=id+1
   fi
   if [ $id -gt $maxn ]; then
    break
   fi
done

   echo "www            CNAME   www1" >> $zonefile
   echo "uuu            CNAME   www2" >> $zonefile
# stuff proxy list now
$proxy

if [ $id -gt $maxn ]; then
 /etc/init.d/named reload
fi

