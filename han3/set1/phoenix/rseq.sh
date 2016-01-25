#!/bin/bash
# usage: rseq n1 n2
# rseq: simple script to generate a randomly shuffled sequence 
# between n1 and n2.

numbers=( $(seq $1 $2) ) 
max=$(( ${#numbers[@]} ))

while [ $max -gt 0 ]; do 
  rand=$(( $RANDOM % $max ))
  max=$(( $max - 1 ))
  if [ "$rand" -eq "$max" ]
  then
    continue
  fi
  tmp=${numbers[$rand]}
  numbers[$rand]=${numbers[$max]}
  numbers[$max]=$tmp
done; 

for idx in $(seq 0 $((${#numbers[@]} - 1))); do
  echo ${numbers[$idx]}
done

