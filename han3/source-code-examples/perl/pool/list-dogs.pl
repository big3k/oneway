#!/usr/bin/perl
$output=`ls -U -x -w 40 /data1/pool/dogs/`; 
print "Content-type: text/html\n\n";
print "<html><body><h3> Requests from compute nodes </h3> <pre>\n";
print $output;
print "</pre></body></html>\n";
 


