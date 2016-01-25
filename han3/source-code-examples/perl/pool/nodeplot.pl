#!/usr/bin/perl
# $Id: nodeplot.pl,v 1.1 2004/03/01 22:21:28 lis Exp lis $
# This program plots the 1km single-var output data on a compute node. 
# Usage: nodeplot.pl ic ir var inputfile outputimg

if ( $#ARGV != 4 ) {
 print "usage: ic ir var inputfile outputimg \n"; 
 exit; 
}

($ic, $ir, $var, $inputfile, $outputimg) = @ARGV; 

$ctlfile = "/tmp/nodeplot.ctl"; 
print "$ctlfile\n";
$slon = -179.995 + ($ic -1) * 720 * 0.01 ; 
$slat = -59.995 + ($ir -1) * 300 * 0.01 ; 
$elon = -179.995 + ($ic) * 720 * 0.01 ; 
$elat = -59.995 + ($ir) * 300 * 0.01 ; 

## get control file in place 
open(CTL, ">$ctlfile") or die "Can not open ctl file: $!\n"; 
 print CTL <<ENDCTL; 
DSET  $inputfile
options big_endian
TITLE output for $ic-$ir, $var
UNDEF -9999
XDEF 720 LINEAR $slon 0.01
YDEF 300 LINEAR  $slat 0.01 
ZDEF    1 LINEAR 1 1
TDEF    1 LINEAR 00Z11jun2001 3hr
VARS 1
$var   1  99  map 
ENDVARS

ENDCTL

close(CTL); 

#$output=`export GADDIR=/data1/grads/lib; export GASCRP=/data1/grads/lib/scripts; /data1/grads-1.8sl11/xgrads -bl << EOSCRP
$output=`export GADDIR=/data1/grads/lib; export GASCRP=/data1/grads/lib/scripts; /data1/grads/old/grads -bl << EOSCRP
open $ctlfile
set mpdset hires
set mpdraw on 
set grads off
set gxout grfill
d $var
cbar
draw title $var at ($slon, $slat) - ($elon, $elat) 
printim $outputimg gif
quit
EOSCRP`; 

#print $output;




