#!/bin/bash

sip1=63.209.222.223
sip2=63.209.222.224
sip3=63.209.222.225
sip4=63.209.222.226

while read line; do 

echo testing $line
/home/oneway/libnet/sample/win-pa -u -s $sip1 -o 9999 -d $line -p 1863
/home/oneway/libnet/sample/win-pa -u -s $sip2 -o 9999 -d $line -p 1863
/home/oneway/libnet/sample/win-pa -u -s $sip3 -o 9999 -d $line -p 1863
/home/oneway/libnet/sample/win-pa -u -s $sip4 -o 9999 -d $line -p 1863

done < <(cat /home/oneway/phoenix/u-nodes.txt) 
