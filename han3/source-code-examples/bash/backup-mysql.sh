#!/bin/bash
now=`date +%m%d%y`
old=`date -d "now-20day" +%m%d%y`
old2=`date -d "now-21day" +%m%d%y`

cd /var/lib/mysql 
tar cvf /home/oneway/johnfax-db-backup/$now.tar mysql ttadatabase
rm -rf /home/oneway/johnfax-db-backup/$old.tar
rm -rf /home/oneway/johnfax-db-backup/${old2}.tar

