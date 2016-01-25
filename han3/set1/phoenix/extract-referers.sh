#!/bin/bash

path=/var/log/squid
tmpf=/tmp/tmp.referer 
echo > $tmpf
#for reff in referer.log referer.log.0 referer.log.1 referer.log.2 referer.log.3
for reff in referer.log
 do
  awk -F 'http://' '{print $2, $3}' $path/$reff |awk '{split($1, surl, "/"); split($2, durl, "/"); print surl[1], durl[1];}' | awk '{if ($1!=$2) print $1, $0}' >> $tmpf 
done

# take a breath 
dotf=/var/log/squid/`hostname`-ref-`date +%y%m%d%H`.dot
echo digraph G > $dotf
echo '{' >> $dotf
echo 'ranksep=1;' >> $dotf
echo 'rankdir=LR;' >> $dotf
echo 'nodesep=0.1;' >> $dotf
echo 'node[fontsize=9, height=0.3, width=1.5, fixedsize=true];' >> $dotf

sort $tmpf |uniq  |perl -pe 's/^.*?\.//' |awk '{OFS=""; if (index($3, $1)==0) print "\"", $2, "\" [URL=\"http:\/\/", $2, "\"];\n", "\"", $3, "\" [URL=\"http:\/\/", $3, "\"];\n", "\"", $2, "\"",  "->", "\"", $3, "\";" }' >> $dotf
echo '}' >> $dotf



