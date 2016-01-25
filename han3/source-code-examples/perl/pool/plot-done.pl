#!/usr/bin/perl
# $Id: plot-done.pl,v 1.3 2004/02/27 23:24:45 bbuser Exp bbuser $
# This program plots the 1km tasks in the queue "bones". 

$key="done"; 
$items = "/data1/pool/$key"; 
$mapfile = "/usr/local/apache/cgi-bin/pool/$key-map.1gd1i"; 
$ctlfile = "/usr/local/apache/cgi-bin/pool/$key-map.ctl"; 
$blockfile = "/usr/local/apache/cgi-bin/pool/blocks.ctl"; 
$imgfile = "/usr/local/apache/htdocs/pool/$key-map.gif"; 
$imglink = "/pool/$key-map.gif"; 

$title{'bones'} = "Remaining jobs"; 
$title{'munching'} = "Current assignments"; 
$title{'done'} = "Finished jobs"; 
$title{'crashes'} = "Crashed jobs"; 

$total = 2500;  ### 50 by 50

@bones = `ls -U $items`; 

foreach $line (@bones) {
  chomp $line; 
   if ($line =~ /\-/) {   # if it has a host name in it
    ($host, $tmp) = split /\-/, $line; 
    $line = $tmp;
   }
  ($ic, $ir) = split /_/, $line; 
  $index = ($ir -1) * 50 + $ic; 
  # fill with ones 
  $map[$index] = 100; 
} # end foreach 
$map[$total] = 1;  # fill a data point to eliminate the msg "uniform data..." on plot. 

open(MAP, ">$mapfile") or die "Can not open map file: $!\n"; 
binmode MAP; 
for (my $i=1; $i <= $total; $i++) { 
 print MAP pack("C1", $map[$i]); 
}
close(MAP); 

## get control file in place 
open(CTL, ">$ctlfile") or die "Can not open ctl file: $!\n"; 
 print CTL <<ENDCTL; 
DSET  $mapfile
TITLE  dogmap 
UNDEF  0
XDEF   50  LINEAR        -176.4 7.2
YDEF   50  LINEAR         -58.5 3
ZDEF    1   LINEAR           1    1
TDEF     1  LINEAR  00Z01jan2002    1mo
VARS     1
var   1  -1,40,1  map 
ENDVARS

ENDCTL

close(CTL); 

#`/data1/grads/gradsc -bl << EOSCRP
`export GADDIR=/data1/grads/lib; export GASCRP=/data1/grads/lib/scripts; /data1/grads-1.8sl11/xgrads -bl << EOSCRP
open $ctlfile
open $blockfile
set mpdset hires
set mpdraw on 
set grads off
set dfile 1 
set lon -180 180
set gxout grfill
set clevs 0 100
d var
set dfile 2
set ccolor 10
set cthick 4
set gxout grid
set mpdraw on 
set clevs 0 100
d var 
draw title $title{$key}
printim $imgfile gif
quit
EOSCRP`; 

print <<HTMLHEADER;
Content-type: text/html

<html>
<head>
<META HTTP-EQUIV="expires" CONTENT="Wed, 26 Feb 1997 08:21:57 GMT">
<title>LIS Cluster System Hardware Monitor</title>
</head>
<body><img src="$imglink"> </body></html>

HTMLHEADER






