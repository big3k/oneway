#!/bin/sh
pip=202.108.37.228
qip=202.108.37.228
sip=202.108
cip=203.108
dev=eth0

for i in `seq 0 25` 
do
#echo $sip.$i.0 
./cping -s $sip.$i.0 -c $cip.$i.0 -p $pip -q $qip -i $dev -n 256 -C c.txt -S s.txt
done



