#!/usr/bin/perl 
#$Id: learn-log.pl,v 1.3 2010/09/09 04:48:20 oneway Exp oneway $

#sample log entries
#2010-09-08 00:12:05,253 INFO  [btpool0-8921] [name=dizihelp@dizi.dc;oip=192.168.1.3;ua=zclient/6.0.0_BETA2_1547.RHEL5_64;] security - cmd=Auth; account=dizihelp@dizi.dc; protocol=soap;
#2010-09-08 00:12:27,202 INFO  [Pop3SSLServer-12505] [ip=68.98.153.60;] security - cmd=Auth; account=terriw@dizi.dc; protocol=pop3;
#2010-09-08 00:12:42,867 INFO  [Pop3SSLServer-12506] [ip=68.98.153.60;] security - cmd=Auth; account=terriw@dizi.dc; protocol=pop3;

# This program examines every user's access against behavior data established
# from trusted access in the past, and sends alert if anomalous access is 
# identified. Therefore, the program needs to run first with exsiting trusted
# logfiles to learn each user's access pattern (time, IP, and protocol), then 
# it compares real-time access data with the established pattern data to 
# identify suspicious access.

# Usage: 
# learn-log.pl [logfile-name]
#  When "logfile-name" is given in the command line, the program will use
# the logfile to learn/update behavior data. Otherwise, it will analyze 
# $LOGFILE and compare against existing behavior data, to identify anomalous 
# behavior.   

$LOGFILE = "/opt/zimbra/log/audit.log";
$PATTERN = "/home/oneway/zimbra-security/behavior.txt";  # past behavior data 
$trainf = $ARGV[0]; 
if (! -e $PATTERN and ! $trainf ) { 
   print("No behavior data and not training file given! \n"); 
   exit;
}
%ips = ();   # ip ranges a user accessed from  
%hrs = ();   # hours of access 
%prots = ();  # protocols

if ( -e $PATTERN ) { 
  open(PATTERN); 
  @lines = <PATTERN>; 
  close(PATTERN); 
  foreach $line (@lines) { 
    ($id, $hr, $ip, $prot) = split /\s/, $line; 
    $ips{$id} = $ip; 
    $hrs{$id} = $hr; 
    $prots{$id} = $prot; 
  }
}

if ( $trainf and -e $trainf ) {   # update behavior file 
  open(TRF, $trainf); 
  foreach $line (<TRF>) {
    chomp($line);         
    $line =~ m/(.+?),.*(INFO|WARN).*ip=(.+?);.*account=(.+?);.*protocol=(.+?);/; 
    if ($2 eq 'INFO') {    # only train with successful access
     $hr = $1; 
     $ip = $3; 
     ($ip1, $ip2, $ip3) = split /\./, $ip; 
     $ip = "$ip1.$ip2";   # only /16 range 
     $id = $4; 
     $prot = $5; 
     $hr =~ m/.*\s(.+?):.*/;
     $hr = $1;   # only hour
     if ( $ips{$id} !~ /$ip/ ) { $ips{$id} = "$ips{$id}|$ip|"; }  
     if ( $hrs{$id} !~ /$hr/ ) { $hrs{$id} = "$hrs{$id}|$hr|"; } 
     if ( $prots{$id} !~ /$prot/ ) { $prots{$id} = "$prots{$id}|$prot|"; } 
    }
   }
  close(TRF); 
  # update behavior file 
  open(OPTN, ">$PATTERN"); 
  foreach $id (keys %ips) { 
    print OPTN "$id $hrs{$id} $ips{$id} $prots{$id}\n"; 
  }
  close(OPTN); 
  exit; 
}

print "Now Chekcing Access\n"; 
open(LOGFILE) or die("Could not open log file.");
foreach $line (<LOGFILE>) {
     chomp($line);         
     $line =~ m/(.+?),.*(INFO|WARN).*ip=(.+?);.*account=(.+?);.*protocol=(.+?);/; 
     $hr0 = $1;
     $ip0 = $3;
     ($ip1, $ip2, $ip3) = split /\./, $ip0;
     $ip = "$ip1.$ip2";   # only /16 range
     $id = $4;
     $prot = "$5";
     $hr0 =~ m/.*\s(.+?):.*/;
     $hr = "$1";   # only hour
     #print "$id $hr $ip $prot\n"; 
     #print "behavior: $hrs{$id} $ips{$id} $prots{$id}\n"; 
     if ( $id ) { 
       if ( $ips{$id} !~ /\|$ip\|/ ) { 
        print "$id: Anomalous access from $ip0 at $hr0 with $prot\n"; }
       if ( $hrs{$id} !~ /\|$hr\|/ ) { 
        print "$id: Anomalous access at $hr0 from $ip0 with $prot\n"; }
       if ( $prots{$id} !~ /\|$prot\|/ ) { 
        print "$id: Anomalous access with $prot from $ip0 at $hr0\n";  }
     }
}

close(LOGFILE); 

