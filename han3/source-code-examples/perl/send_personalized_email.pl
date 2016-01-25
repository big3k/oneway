#!/usr/bin/perl -w 
# $Id: send_personalized_email.pl,v 1.2 2011/07/27 04:46:57 oneway Exp oneway $ 

my ($gateway, $email_sub, $ibidemail, $sendmail, $webmaster, @list);
#--------- Setting up global variables and constants --------

$gateway = "Registration Form Gateway";
$email_sub = "Here is your NY Fahui ticket number";
$ibidemail="yanqingc\@dizi.dc";
$sendmail = "/usr/sbin/sendmail -t -n -oi";
$webmaster = "dizihelp\@dizi.dc"; 
#$regdate = `date`;

$listfile = "list.txt";   # each line has "userid  message" format

$tosend=1;    # Switch to turn off email. 0: off; 1: on.
#-----------------------------------------------------------
$|=1;  # disable buffering

# read in the list
open(LIST, "<$listfile") or die "Can not open list file: $!";
@list=<LIST>;
close(LIST);

foreach $addr (@list) {
  chomp $addr;
  ($user, $msg) = split(/ /, $addr, 2); 

  print "Emailing to $user\@dizi.dc ...\n"; 

#--------- Sending Email to customer --------------
if($tosend) {
open (SENDMAIL, "| $sendmail")   or  &return_error (504,
       "Server Error", "Sendmail open error");
 print SENDMAIL <<Mail_To_Customer;
From: Yudong <$ibidemail>
To: $user\@dizi.dc
Bcc: <yudongt\@dizi.dc> 
Reply-To: $ibidemail
Subject: $email_sub
X-Mailer: $gateway
X-Remote-Host: dizi.dc 

Dear $user: 
  Here is your NY Fahui ticket number(s) you've requested. 

  Your messge is: $msg 

  Should you have any questions, do not hesitate to contact 
  $ibidemail.

Mail_To_Customer

close SENDMAIL;
}

}
exit(0);

