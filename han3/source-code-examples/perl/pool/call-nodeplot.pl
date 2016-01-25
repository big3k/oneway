#!/usr/bin/perl -w
# $Id: call-nodeplot.pl,v 1.2 2004/06/01 15:05:20 bbuser Exp bbuser $ 
# CGI to invoke nodeplot.pl, after getting host, ic, and ir 

use CGI qw(:standard); 

require '/data1/pool/control/getconf.pl';

my %conf = ();
&getconf (\%conf);

$host = param("host"); 
$ic = param("ic"); 
$ir = param("ir"); 
$var = param("var"); 
#sanity check
$host =~ s/\W//g; 
$ic =~ s/\D//g;
$ir =~ s/\D//g;
$var =~ s/\W//g; 

$inputfile = 
  "/data/OUT1KM$ic-$ir/$conf{exp}/$conf{lsm}/2001/20010611/2001061121$var.gd4r"; 
#VIC  "/data/OUT1KM$ic-$ir/EXP999/VIC/2001/20010611/LDAS.EXP999.2001061121$var.VICbin"; 
$imglink = "/pool/nodeplot/$ic-$ir-$var.gif";  # used for html
$imgfile = "/data2/nodeplot/$ic-$ir-$var.gif"; 
`ssh -4 $host -l lis "/data1/bin/nodeplot.pl $ic $ir $var $inputfile $imgfile"`; 

print <<HTMLHEADER;
Content-type: text/html
Expires: Thu, 29 Oct 1998 17:04:19 GMT


<html>
<head>
<META HTTP-EQUIV="expires" CONTENT="Wed, 26 Feb 1997 08:21:57 GMT">
<META HTTP-EQUIV="CACHE-CONTROL" CONTENT="NO-CACHE">
<META HTTP-EQUIV="PRAGMA" CONTENT="NO-CACHE">
<title>OUTPUT</title>
</head>
<body>
<form method="post" action="/cgi-bin/pool/call-nodeplot.pl">
<input type="hidden" name="host" value="$host">
<input type="hidden" name="ic" value="$ic">
<input type="hidden" name="ir" value="$ir">
<b>Choose another variable: </b>
<select name="var">
<option value="qh">Qh
<option value="qle">Qle
<option value="qg">Qg
<option value="qs">Qs
<option value="lwnet">LWnet
<option value="swnet">SWnet
<option value="avgsurft">AvgSurfT
</select>
<input type="submit" value="Plot">
</form>
<img src="$imglink">
</body></html>

HTMLHEADER


