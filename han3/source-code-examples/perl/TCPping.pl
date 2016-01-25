#!/usr/bin/perl
# Send TCP SYN to elicit RST or SYN-ACK to get a list of
# live IPs.
# This program reads a file "activeIPs.txt" to get the raw list IPs to test
# The final product is output.txt
# $Id: TCPping.pl,v 1.1 2003/10/01 03:28:58 dweb Exp dweb $

require "/usr/local/ipadmin/selfchecker/getIP.pl"; 

$threshold = 0.1;   # blocked if less then 60% responded. 
$oif = "eth0";    # outside interface 
$path = "/usr/local/ipadmin/selfchecker";    # raw IP list
$dbf = "$path/activeIPs.txt";    # raw IP list
#Use a non-blocked IP
$sIP = "66.98.139.135";
#$sIP = getIP($oif);  # local IP, pick a non-blocked one
$sport = 54287;   # source port
$dport = 80;   # dst port

open(DBF, "< $dbf") or die "Can't not open $dbf: $!";
my @lines = <DBF>;
close(DBF);
fisher_yates_shuffle( \@lines );    # shuffle lines 

# start sniffer
`/usr/sbin/tcpdump_tcpping -i $oif -n -q -p "dst port $sport  and src port $dport" >$path/results.txt &`;

$count = 0; 
foreach $line (@lines) {
   #$/="\r\n";
   chomp $line;
   `$path/win-pa -s $sIP -o $sport -d $line -p $dport -S`;
   $count++;
   select(undef, undef, undef, 0.05); # short sleep 
  # print "."; 
} # end of foreach

print "All $count packets sent.\n"; 
sleep 12;   # waiting for all the packets back

`/usr/bin/killall -TERM tcpdump_tcpping`;   # stop sniffer
$line = `wc -l $path/results.txt`;
chomp $line;
print $line, "\n"; 
print "percentage: ", $line/$count, "\n"; 
if ($line/$count < $threshold) {
   # trouble, email notice or change IP
   #  `$path/changeIP.pl`;
   #`mail zhenshi99\@yahoo.com -s "x1 blocked" < $path/message.txt`; 
   `mail billxia\@bellsouth.net -s "x1 blocked" < $path/message.txt`; 
   # or write to log file and quit
   `/bin/date >> $path/error.log`;
   `/bin/echo $sIP is blocked! >> $path/error.log`;
} else {
   `head -n 100 $path/results.txt |awk \'{print \$2}\'|sed -e "s/\.http//" > $path/output.txt`;
}
# Generate "reference.txt" on another port 
`$path/TCPping2.pl`; 

exit; 

    # fisher_yates_shuffle( \@array ) :
    # generate a random permutation of @array in place
    sub fisher_yates_shuffle {
        my $array = shift;
        my $i;
        for ($i = @$array; --$i; ) {
            my $j = int rand ($i+1);
            next if $i == $j;
            @$array[$i,$j] = @$array[$j,$i];
        }
    }
 


