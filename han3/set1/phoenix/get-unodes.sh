#!/bin/bash
# script to get IP of unoses 
cd /home/oneway/phoenix
wget -O tmp_unodes.txt  https://unodes.edoors.info/unode/vpn/unode_list_vpn.txt
#backup url wget -O tmp_unodes.txt  https://cs123.dyndns.org/unode/vpn/unode_list_vpn.txt
cat tmp_unodes.txt | grep -v '^#' |cut -d '/' -f 1 > u-nodes.txt
# if no tw nodes needed
#cat tmp_unodes.txt | grep -v '^#' |grep -v -i hinet |cut -d '/' -f 1 > u-nodes.txt
#cat nodes.txt u-nodes.txt > all-nodes.txt
cp -f u-nodes.txt /home/oneway/mc
