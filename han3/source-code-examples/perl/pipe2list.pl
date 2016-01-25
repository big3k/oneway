#! /usr/bin/perl -U
# Usage: pipe2list.pl listfile
# ATTN: this script has to run as setuid root, and with -U option
# so be very careful. 
# save standard input in a physical file, and make a symbolic link
# to Maildir/new of everyone in the mailing list. That will save
# a lot disk space and operations.

$ENV{PATH}="/bin"; 

$homeroot="/usr/home";    # parent dir of users' home dir
$repos = "/usr/storeall";   # dir to save the physical copy 
$gid ="mail";     # gid of new messages

$listfile = shift; 

if ( !$listfile) { 
  print "Usage: pipe2list.pl listfile \n"; 
  exit(1); 
}

# read in the list
open(LIST, "<$listfile") or die "Can not open list file: $!"; 
@list=<LIST>; 
close(LIST); 

# construct message file name, perl qmail's Maildir spec.
$host = `/bin/hostname`; 
chomp $host; 
$timestamp=time(); 
$rand1=int(rand(89999)) + 10000; 
$rand2=int(rand(89999)) + 10000; 
$mfile = "$timestamp.H${rand1}P${rand2}.$host"; 

# save email to physical file
open (TMP, ">$repos/$mfile") or die "Can not open physical file: $!"; 
while (<>) { 
  print TMP; 
}
close(TMP);
chmod 0644, "$repos/$mfile"; 

foreach $addr (@list) { 
  chomp $addr; 
  $timestamp=time(); 
  $rand1=int(rand(89999)) + 10000; 
  $rand2=int(rand(89999)) + 10000; 
  $lfile = "$timestamp.H${rand1}P${rand2}.$host"; 
  ($user, $domain) = split(/\@/, $addr); 
  symlink "$repos/$mfile", "$homeroot/$user/Maildir/new/$lfile"; 
}
exit(0); 



