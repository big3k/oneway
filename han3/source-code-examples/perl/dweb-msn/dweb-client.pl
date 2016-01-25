#!/usr/bin/perl
# dweb msn bot. 
# modified from cpan msn.pm by oneway, 4/5/2006
# $Id: dweb-client.pl,v 1.2 2006/04/05 20:44:31 oneway Exp oneway $
# usage:
# dweb-client.pl accountfile msnip.txt req_logf msn_logf
# accountfile saves user and password info in two lines

use strict;
use warnings;

use Net::MSN;
use IO::Select;
use POSIX;

use Data::Dumper;

if (!defined $ARGV[3]) {
    print "usage: dweb-client.pl accountfile msnip.txt req_logf msn_logf\n"; 
    exit;
}
my $D = 1;     # always run in demon mode

my $oldt = 0; 
my $age = 300;     # time interval to read $IPFile
my $ActFile = $ARGV[0]; 
my $IPFile = $ARGV[1]; 
my $ReqFile = $ARGV[2]; 
my $LogFile = $ARGV[3]; 
my @msg;  # reply message 
my $logsave='';  # buffer log messages 

open(ACT,"<$ActFile") or die "Can not open account file $ActFile: $!\n";
my $handle = <ACT>; 
my $password = <ACT>; 
chomp $handle;
chomp $password; 
close(ACT); 

my $timeout = 0.01;
my $s;


my %admin = (
  'nosuchdweb002@hotmail.com' => 1
);


if ($D == 1) {
  &demonize_me();
} else {
  $s = IO::Select->new();
  $s->add(\*STDIN);
}

my $client = new Net::MSN(
  Debug           =>  1,
  Debug_Lvl       =>  1,
  Debug_STDERR    =>  1,
  Debug_LogCaller =>  1,
  Debug_LogTime   =>  1,
  Debug_LogLvl    =>  1,
  Debug_Log       =>  $LogFile
);

$client->set_event(
  on_connect => \&on_connect,
  on_status  => \&on_status,
  on_answer  => \&on_answer,
  on_message => \&on_message,
  on_join    => \&on_join,
  on_bye     => \&on_bye,
  auth_add   => \&auth_add
);

$client->connect($handle, $password);

while (1) {
  $client->check_event();
  &checkSTDIN() unless ($D == 1);
}

sub checkSTDIN {
  if (my @r = $s->can_read($timeout)) {
    foreach my $fh (@r) {
      my $input = <$fh>;
      print '> '. $input;
      chomp($input);
      
      my ($cmd, @data) = split(/ /, $input);

      next unless (defined $cmd && $cmd);

      if ($cmd eq 'call') {
	if (defined $data[0]) {
	  unless ($client->call($data[0])) {
	    print $data[0]. " is not online or not on your contact list\n";
	  }
	} else {
	  print "no party specified to call!\n";
	}
      } elsif ($cmd eq 'msg') {
	my $calling = shift @data;
	my $message = join(' ', @data);
	my $r = $client->sendmsg($calling, $message);
	print $calling. " is not online or not on your contact list\n"
	  unless (defined $r && $r);
      } elsif ($cmd eq 'list') {
	$client->send('LST', 'RL');
      } elsif ($cmd eq 'quit') {
	$client->disconnect();
	exit;
      } elsif ($cmd eq 'ping') {
	$client->sendnotrid('PNG');
      }	elsif ($cmd eq 'who') {
	my $calling = shift @data;
	my $response = &who();
	$client->sendmsg($calling, $response);
      } elsif ($cmd =~ /die/) {
	die;
      } elsif ($cmd eq 'send') {
	my ($command, @payload) = @data;
	my $payload = '';
	if (@payload && @payload >= 1) {
	  $payload =  join(' ', @payload);
	}
	print STDERR "SEND: ". $command. ' '. $payload. "\n";
	$client->send($command, $payload);
      } elsif ($cmd eq 'dump') {
	use Data::Dumper;
	print '$client = '. Dumper($client). "\n";
      }
    }
  }
}

sub on_connect {
  $client->{_Log}("Connected to MSN @ ". $client->{_Host}. ':'. 
    $client->{Port}. ' as: '. $client->{ScreenName}. 
    ' ('. $client->{Handle}. ")", 3);
}

sub on_status {
  # FIXME
}

sub on_message {
  my ($sb, $chandle, $friendly, $message) = @_;
  $logsave = $logsave . scalar(localtime) . " | REQ: $friendly\n"; 

##!! oneway: Save $/ and restore it after reading the file. Otherwise
##!! it will crash the connection to msn!
  my $newt = time();    
  if ($newt - $oldt > $age) {  # read ip file 
    open(REQLOG,">>$ReqFile") or warn "Can not open request log file $ReqFile: $!\n";
    #print REQLOG scalar(localtime), " | REQ: $friendly\n"; 
    print REQLOG $logsave;
    close(REQLOG); 
    $logsave = ''; 
    my $tmpx = $/; 
    open(IPF, "<$IPFile") or warn "Can not open ip file: $!\n"; 
    @msg = <IPF>; 
    close(IPF); 
    $/ = $tmpx; 
    $oldt = $newt; 
  }

  srand(perlhash($friendly) ) ;
  my $mymsg='';
  for (my $nm=0; $nm < 6; $nm ++) {
    my $idx =  int(rand(@msg));
    $mymsg = $mymsg . $msg[$idx];
  }

  $sb->sendmsg($mymsg);
}

sub perlhash {
      my $hash = 0;
      foreach (split //, shift) {
          $hash = $hash*33 + ord($_);
      }
      return $hash % 32768;
}


sub help {
  return "msn-client's Command List\n\n".
    "Standard Commands:\n".
    " reply       - msn-client will send a message to you\n".
    " calc        - calculate an expression, ie 'calc 1 * 9'\n".
    " who         - shows whos online on msn-client's contact list\n".
    " msg msnname - message someone on msn-client's list\n\n".
    "Admin Commands:\n".
    " self destruct - cause msn-client to quit\n";
}

sub on_bye {
  my ($chandle) = @_;

  $client->{_Log}($chandle. " has left the conversation (switch board)", 3);  
}

sub on_join {
  my ($sb, $chandle, $friendly) = @_;

  $client->{_Log}($chandle. " has joined the conversation (switch board)", 3);  
}

sub on_answer {
  my $sb = shift;

  #print "Answer() called with parameters:\n";
  #print "   " . join(", ", @_), "\n";
}

sub auth_add {
  my ($chandle, $friendly) = @_;

  $client->{_Log}('recieved authorisation request to add '. $chandle. ' ('.
    $friendly. ')', 3);

  return 1;
}

sub who {
  my ($requestor) = @_;
  $requestor = $requestor || '';

  return 'Sorry, nobody is online :('
    unless (defined $client->{Buddies} &&
    ref $client->{Buddies} eq 'HASH');

  my $response;
  foreach my $username (keys %{$client->{Buddies}}) {
    next unless ($client->{Buddies}->{$username}->{StatusCode} eq 'NLN');
    #next if ($username eq $requestor);
    $response .= '* '. $username. ' ('. 
      $client->{Buddies}->{$username}->{DisplayName}.
      ') is '. $client->{Buddies}->{$username}->{Status}. "\n";
  }

  chomp($response);

  return (defined $response && $response) ?
     $response : 'Sorry, nobody is online :(';
}

sub demonize_me ($) {
  print "Daemonizing msn-client ...\n";
  defined (my $pid = fork) or die "Can't fork: $!";
  if ($pid) {
    # close parent process.
    exit;
  } else {
    # use the child process
    POSIX::setsid or die "Can't start a new session: $!";
    open (STDOUT,'>>'. $LogFile)
      or die "ERROR: Redirecting STDOUT to ". $LogFile. ': '. $!;
    open (STDERR,'>>'. $LogFile)
      or die "ERROR: Redirecting STDERR to ". $LogFile. ': '. $!;
    open (STDIN, '</dev/null') 
      or die "ERROR: Redirecting STDIN from /dev/null: $!";
  }
}
                                                                                                                           
sub is_running {
  return 1;
}

