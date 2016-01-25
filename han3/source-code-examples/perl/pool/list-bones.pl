#!/usr/bin/perl
$output=`ls -U -x -w 20 /data1/pool/bones/`; 
$num =`ls /data1/pool/bones/ | wc -l `; 
print "Content-type: text/html\n\n";
print "<html><body><h3> Tasks: $num </h3> <pre>\n";
print $output;
print "</pre></body></html>\n";
 


