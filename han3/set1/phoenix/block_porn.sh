#!/bin/bash
# script to put ips in iptables' block_porn chain on bw7 and phx2. 
# IP list is obtained from http://hfhctl.edoors.com/prn.txt

ips=`wget -O - http://hfhctl.edoors.com/prn.txt`

if [ $? -ne 0 ]; then
  echo "Extraction of IP list failed. Did nothing ..."; 
  exit 1 
fi

/sbin/iptables -F block_porn

for ip in $ips; do
 /sbin/iptables -A block_porn -d $ip -j DROP
done



