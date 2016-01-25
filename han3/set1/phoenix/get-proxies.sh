#!/bin/bash
# script to update DNS server zone file and reload server. 
# need to be root to do it. 
path=/home/oneway/phoenix
maxn=10
xy_list=$path/proxies.txt 
domain=clipurl.org

# script to generate a shuffled list
rseq=$path/rseq.sh

zonefile=/var/named/chroot/var/named/clipurl.org

# get updated proxy list
wget -O $xy_list http://69.93.255.138:8082/firep_ssl.txt

id=1
nodes=( $(tail +2 $xy_list) ) 
for idx in `$rseq 0 $((${#nodes[@]} - 1))`; do 
   node=${nodes[$idx]}
   ip=`echo $node | cut -d ':' -f 1`
   port=`echo $node | cut -d ':' -f 2`
   # port to MX preference number mapping: 80->10, 3128->20, 8080->30
   if [ $port -eq 80 ]; then
      pref=10
   elif [ $port -eq 3128 ]; then 
      pref=20
   elif [ $port -eq 8080 ]; then 
      pref=30
   else 
      break
   fi

    echo "$domain.            MX       $pref $ip" >> $zonefile
    let id=id+1 
   if [ $id -gt $maxn ]; then
    break
   fi
done
# stuff proxy servers in


