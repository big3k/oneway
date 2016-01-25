#!/usr/bin/perl
$output=`ls -U -x -w 40 /data1/pool/munching/`; 
$num =`ls /data1/pool/munching/ | wc -l `;

print "Content-type: text/html\n\n";
print "<html><body><h3> Assignments: $num </h3> <pre>\n";
print $output;
print "</pre></body></html>\n";
 


