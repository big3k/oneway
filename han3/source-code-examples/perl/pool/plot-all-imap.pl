#!/usr/bin/perl
# $Id: plot-all-imap.pl,v 1.5 2004/03/09 22:44:30 bbuser Exp bbuser $
# This program plots the 1km tasks in all the queues: "bones", "munching", "done", 
# and "crashes".  Output an image map for the "done" blocks so results
# can be plotted. 

@keys=("crashes", "done", "bones", "munching");  # let "done" overwrite "crashes"
$blockfile = "/usr/local/apache/cgi-bin/pool/blocks.ctl"; 
$imglink = "/pool/all-map.gif"; 
$docroot = "/usr/local/apache/htdocs"; 
$imgfile = "$docroot//$imglink"; 
$mapfile = "/usr/local/apache/cgi-bin/pool/all-map.1gd1i"; 
$ctlfile = "/usr/local/apache/cgi-bin/pool/all-map.ctl"; 

$title{'bones'} = "Remaining jobs"; 
$title{'munching'} = "Current assignments"; 
$title{'done'} = "Finished jobs"; 
$title{'crashes'} = "Crashed jobs"; 

$maxic = 50; #max ic
$maxir = 50; #max ir
$total = $maxic * $maxir;  ### 50 by 50

$i = 1; # levels
foreach $key (@keys) {
 $items = "/data1/pool/$key"; 
 @bones = `ls -U $items`; 

 foreach $line (@bones) {
  chomp $line; 
   if($key eq 'bones') { #  only bones have no host name
    ($bonew, $tmp) = split /\-/, $line, 2; 
    $line = $tmp;
   } else {
    ($host, $bonew, $tmp) = split /\-/, $line, 3; 
    $line = $tmp;
   }
  ($ic, $ir) = split /_/, $line; 
  $index = ($ir -1) * $maxic + $ic; 
  # fill with numbers
  $map[$index] = $i; 
  $hosts[$index] = $host;      # record the machine to be used later 
 } # end foreach  @bones

$i++; 
} # end foreach @keys 

$map[$total] = 5;  # fill a data point to eliminate the msg "uniform data..." on plot. 

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
set lat -60 90
set clevs 1 2 3 4 5
set ccols 2 3 8 7 0
set gxout grfill
d var
set dfile 2
set ccolor 10
set cthick 4
set gxout grid
set mpdraw on 
set clevs 0 100
d var 
draw title ORANGE/tasks  YELLOW/doing  GREEN/done  RED/crashed
printim $imgfile gif
quit
EOSCRP`; 

# output image map
print <<HTMLHEADER;
Content-type: text/html

<html>
<head>
<META HTTP-EQUIV="expires" CONTENT="Wed, 26 Feb 1997 08:21:57 GMT">
<title>LIS Cluster System Hardware Monitor</title>
</head>
<body><img src="$imglink" usemap="#green" border="0"> 
<map name="green">
HTMLHEADER

($x0, $y0) = (36, 124);  # starting point of the image (UL corner)
($width, $height) = (726.0/$maxic, 352.0/$maxir); # average cell size
# now print the coordinates
for ($ir = 1; $ir <= $maxir; $ir++) {
  for ($ic = 1; $ic <= $maxic; $ic++) {
   $index = ($ir -1) * $maxic + $ic; 
   if($map[$index] == 2 ) { # this is a done piece
     ($x1, $y1) = ($x0 + int( ($ic-1)*$width ), $y0 + int( ($maxir - $ir)* $height) ); 
     ($x2, $y2) = ($x0 + int( $ic*$width ), $y0 + int(($maxir - $ir + 1)* $height) ); 
     $host = $hosts[$index]; 
    print <<COORD;
<area shape="rect" coords="$x1,$y1,$x2,$y2"
href="/cgi-bin/pool/call-nodeplot.pl?host=$host\&ic=$ic\&ir=$ir\&var=swnet"> 
COORD
    } # end if 
  } # end for ic
} # end for ir

print <<MAPEND;
</map>
</body></html>

MAPEND






