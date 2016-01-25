#!/usr/bin/perl
# Signal: put a file on /tmp/, to signal the start of the farmer and dogs  
print "Content-type: text/html\n\n";
print "<html><body><h3> Job started ... </h3> <pre>\n";
print "</pre></body></html>\n";

`touch /tmp/FDstart`; 
 


