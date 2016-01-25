#!/usr/bin/perl 
# $Id$
# CGI to invoke nodeplot.pl, after getting host, ic, and ir 
# Request: http://lis2.sci.gsfc.nasa.gov:9090/cgi-bin/gdsplot.pl?var=clay&url=http://x8:9995/dods/LIS_Params/BCS/clay60&X=7200&Y=3000&Z=3&T=1

use CGI qw(:standard); 

# define env variables for GrADS
$GADDIR = "/home/gds/grads-1.9b3/lib"; 
$GASCRP = "/home/gds/grads-1.9b3/lib/scripts"; 
$GRADS = "/home/gds/grads-1.9b3/bin/gradsdods";    # GrADS executable

%vars = (); 
#-----required variables, no default 
$vars{var} = param("var"); 
$url = param("url"); 
$vars{E} = param("E"); 
$vars{W} = param("W"); 
$vars{S} = param("S"); 
$vars{N} = param("N"); 

#------- optional variables. 
$vars{lat1} = param("lat1") or $vars{lat1} = 30; 
$vars{lat2} = param("lat2") or $vars{lat2} = 45; 
$vars{lon1} = param("lon1") or $vars{lon1} = -100; 
$vars{lon2} = param("lon2") or $vars{lon2} = -70; 

# variables for futher use: size of the 4 dimensions
$vars{Z} = param("Z"); 
$vars{T} = param("T"); 

# other parameters.
$vars{tp} = param("tp") or $vars{tp} = 1; 
$vars{t} = param("t") or $vars{t} = $vars{T}; 
$vars{z} = param("z") or $vars{z} = 1; 

#sanity check
$url =~ s/[^a-zA-Z0-9\/\+-?\._\&]//g;
foreach $key (keys %vars) {
  $vars{$key} =~ s/[^a-zA-Z0-9\/\+-\._]//g;
}
$vars{url} = $url; 

$vars{imglink} = "/LE/$vars{var}-$$.gif";  # used for html
$imgfile = "/www/LE/$vars{var}-$$.gif";

$h = $vars{lat2} - $vars{lat1}; 
$w = $vars{lon2} - $vars{lon1}; 

# Pan operation
if ($pan = param("P") ) {
PAN: {
    if ($pan eq 'u') { $vars{lat1} += $h/2; $vars{lat2} += $h/2; last PAN; }
    if ($pan eq 'd') { $vars{lat1} -= $h/2; $vars{lat2} -= $h/2; last PAN; }
    if ($pan eq 'l') { $vars{lon1} -= $w/2; $vars{lon2} -= $w/2; last PAN; }
    if ($pan eq 'r') { $vars{lon1} += $w/2; $vars{lon2} += $w/2; last PAN; }
    }
}

#Zoom operation
if ($zoom = param("ZM") ){ 
ZOOM: {
    if ($zoom eq '1') { $vars{lat1} += $h/4; $vars{lat2} -= $h/4;
                        $vars{lon1} += $w/4; $vars{lon2} -= $w/4;
                        last ZOOM; }
    if ($zoom eq '-1') { $vars{lat1} -= $h/4; $vars{lat2} += $h/4;
                        $vars{lon1} -= $w/4; $vars{lon2} += $w/4;
                        last ZOOM; }
    } 
}

# plot type
$type{"1"} = "shaded"; 
$type{"2"} = "contour"; 
$type{"3"} = "grfill"; 

# confine the plot region to be within (E, W, S, N)
$nborder = $vars{N} + 3.0 < 90.0 ? $vars{N} + 3.0 : 90.0; 
$sborder = $vars{S} - 3.0 > -90.0 ? $vars{S} - 3.0 : -90.0; 
if ($vars{lat1} < $sborder ) {
   $ushift = $sborder - $vars{lat1}; 
   $vars{lat1} = $sborder; 
   $vars{lat2} = $vars{lat2} + $ushift < $nborder ? $vars{lat2} + $ushift : $nborder; 
}
if ($vars{lat2} > $nborder) {
   $dshift = $vars{lat2} - $nborder; 
   $vars{lat2} = $nborder; 
   $vars{lat1} = $vars{lat1} - $dshift > $sborder ? $vars{lat1} - $dshift : $sborder; 
}
($vars{lon1}, $vars{lon2}) = $vars{lon1} < -180.0 ? 
      ($vars{lon1} + 360.0, $vars{lon2} + 360.0) : ($vars{lon1}, $vars{lon2}); 
($vars{lon1}, $vars{lon2}) = $vars{lon2} > 360.0 ? 
      ($vars{lon1} - 360.0, $vars{lon2} - 360.0) : ($vars{lon1}, $vars{lon2}); 
#$vars{lon1} = $vars{lon1} > $vars{W} && $vars{lon1} < $vars{E} ? $vars{lon1} : $vars{W}; 
#$vars{lon2} = $vars{lon2} > $vars{W} && $vars{lon2} < $vars{E} ? $vars{lon2} : $vars{E}; 

$color = "rainbow"; 
if ( $vars{lon1} == $vars{lon2} || $vars{lat1} == $vars{lat2} ){ $color = "4" }; 
# round off 
foreach $key ("lat1", "lat2", "lon1", "lon2") {
 $vars{$key} = sprintf "%.2f", $vars{$key}; 
}

$vars{output}=`export GADDIR=$GADDIR; export GASCRP=$GASCRP; export TERM=vt100; $GRADS -bl << EOSCRP
set background 1
reinit
sdfopen $url
set parea 0.7 10.5 0.8 8.0
set mpdset hires
set mpdraw on
set map 0
set grads off
set gxout $type{$vars{tp}} 
set xlopts 0 4 0.13
set ylopts 0 4 0.13
set annot 0 4
set cmark 0 
set lat $vars{lat1} $vars{lat2}
set lon $vars{lon1} $vars{lon2}
set t $vars{t}
set z $vars{z}
set ccolor $color
d $vars{var}
cbar
gds_title $vars{var}
printim $imgfile gif x680 y550
quit
EOSCRP`;

$vars{output} .= $?;  # append  error number
print <<HTMLHEADER;
Content-type: text/html
Expires: Thu, 29 Oct 1998 17:04:19 GMT

HTMLHEADER

print template("gdsplot.template", \%vars); 

exit;

sub template {
  my ($filename, $fillings) = @_;
  my $text;
  local $/;
  local *F;
  open(F, "< $filename\0")  || return;
   $text = <F>;
  close(F);
  #replace quoted workds with values in %$fillings hash
  $text =~ s{ %% ( .*? ) %% }
            { exists( $fillings->{$1} )
                    ? $fillings->{$1}
                    : ""
            }gsex;
  return $text;
}


