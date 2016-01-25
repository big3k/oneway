#!/bin/bash
# script to update active server list 
# email alert if server is down 
path=/home/oneway/phoenix
server_list=$path/servers.txt
# original version of netcat. GNU version's -w option does not work 
nc=$path/nc
tmpf=$path/tmp-server.txt
port=1863
webpath=/usr/local/apache2/htdocs/fphenix
good=$path/running-servers
bad1=$path/down-servers
bad2=$path/down-lastchk

servers=( $(cat $server_list) ) 
echo -n > $bad1
touch $bad2
for idx in `seq 0 $((${#servers[@]} - 1))`; do 
   server=`echo ${servers[$idx]} |cut -d: -f1`
   uml=`echo ${servers[$idx]} |cut -d: -f2`
   $nc -w 6 -z $server $port 
   if [ $? -eq 0 ]; then
    echo $uml >> $good
    echo "UDPIP=$server" >> $tmpf
   else
    echo Server $server is down! >> $bad1
   fi
done

cat $tmpf >> $good
rm $tmpf
mv -f $good $webpath
cmp -s $bad1 $bad2
if [ $? -ne 0 -a -s $bad1 ]; then 
  mail -v -s "Linux info" yudong@hsb.gsfc.nasa.gov < $bad1
  cp -f $bad1 $bad2
fi
