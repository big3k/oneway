#!/usr/bin/perl
@output=`ls -U /data1/pool/done/ |/bin/sort -n`; 
$i = 1; 
$fname = 0; 
$last = pop @output;
chomp $last; 
print "Content-type: text/html\n\n";
print "<html><body><h3> Done </h3> <pre>\n";
while ($i < $last) {
  for ($j=$i; $j<$i+4; $j++) {
   if (-e "/data1/pool/done/$j") {
    print "$j\t";
   } else {
    print " \t";
   } # end if
  } # end for

   print "\n";
   $i+=4; 
  
}
print "</pre></body></html>\n";
 


