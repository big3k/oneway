#!/usr/bin/perl
$output=`ls -U -x -w 30 /data1/pool/crashes/`; 
$num =`ls /data1/pool/crashes/ | wc -l `;

print "Content-type: text/html\n\n";
print "<html><body><h3>Crashes: $num </h3> <pre>\n";
print $output;
print "</pre></body></html>\n";
 


