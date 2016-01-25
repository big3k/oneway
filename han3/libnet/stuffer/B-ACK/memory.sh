#!/bin/sh
dip=202.108.37.228
dprt=80
ttl=18
for sprt in 2000 3000 4000 5000 6000 7000 8000 9000 10000 11000 12000
do
echo "=== $sprt "
date
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl -E tbkw.txt
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl -E tbkw.txt
done

for sprt in 2004 3004 4004 5004 6004 7004 8004 9004 10004 11004 12004
do
echo "===ACK $sprt "
date
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl 
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl 
sleep 300
done


