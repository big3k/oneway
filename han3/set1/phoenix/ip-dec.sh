#!/bin/bash
# function to decrypt an IP address 

f1=`echo $1 | cut -d . -f 1`
f2=`echo $1 | cut -d . -f 2`
f3=`echo $1 | cut -d . -f 3`
f4=`echo $1 | cut -d . -f 4`

k1=68    # ascii of 'D'
k2=83    # 'S'
k3=72    # 'H'
k4=70	 # 'F'

let "ip1=$f3^$k1"
let "ip2=$f4^$k2"
let "ip3=$f1^$k3"
let "ip4=$f2^$k4"

echo $ip1.$ip2.$ip3.$ip4

