#!/usr/bin/perl

#
# Started as modifications to the wikiCalc(R) program, version 1.0.
# wikiCalc 1.0 was written by Software Garden, Inc.
#
# 2007-05-09:
#  Changes to wikiCalc 1.0 as part of the Original Code made by Socialtext, Inc., include:
#   Created socialcalccgi.pl as a renamed version of wikicalccgi.pl using the name SocialCalc.
#   Changed copyright, license, and other legal notices in WKC.pm, and in other files.
#
# 2007-06-19:
#  use lib 'lib';
#
# Portions (c) Copyright 2005, 2006, 2007 Software Garden, Inc.
# All Rights Reserved.
# Portions (c) Copyright 2007 Socialtext, Inc.
# All Rights Reserved.
#

=pod
The contents of this file are subject to the Common Public Attribution License
Version 1.0 (the "License"); you may not use this file except in compliance
with the License. You may obtain a copy of the License at
http://socialcalc.org/licenses/stpl-20/.

Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
the specific language governing rights and limitations under the License.

The Original Code is: SocialCalc 1.1 distributed by Socialtext.

The Initial Developer of the Original Code is Software Garden, Inc. and Socialtext, Inc. 

Portions created by Software Garden, Inc., are:
Copyright (C) 2005, 2006, 2007 Software Garden, Inc.
All Rights Reserved.

Portions created by Socialtext are:
Copyright (C) 2007 Socialtext, Inc. 
All Rights Reserved.

Additional Attribution Information as required by Exhibit B of the License:

See logo in: sclogo.gif
"Socialtext, provider of the leading commercial open source enterprise wiki."
http://www.socialtext.com

=cut

#
# SocialCalc
#
# This is the interface for using the program
# through a normal web server.
#
# Put this file in a cgi-bin directory or equivalent
# and use a browser to access the UI.
#

   use strict;

   use CGI::Cookie;

   use lib 'lib';
   use App::SocialCalc;
   use App::SocialCalc::Strings;

   use CGI::Carp qw(fatalsToBrowser);

   # Get query parameters

   my $query;
   if ($ENV{REQUEST_METHOD} eq 'POST') {
      read(STDIN, $query, $ENV{CONTENT_LENGTH});
      }
   else {
      $query = $ENV{QUERY_STRING};
      }

   # Get our cookie, if present

   my %cookies = parse CGI::Cookie(scalar $ENV{HTTP_COOKIE});
   my $cookievalue = $cookies{wkcdata}->{value}[0]; # Get our cookie to pass to request processing

   #oneway 3/28/08. check if authenticated by sqmail
   my $session_name = 'SQMSESSID';
   my $session_id = $cookies{$session_name}->{value}[0];

   my ($username, $authed); 
   if ($session_id) { # have the php session
      # the line below takes too long (5min). so do our own session extract
      # my $sqm_session = PHP::Session->new($session_id);
      open(SNF, "/tmp/sess_$session_id");
      read(SNF, my $str, 300);
      close(SNF);

      $str =~  /username\|s:\d+:"(.*)";user_is_logged_in\|b:(\d);/;

      #print "username: $1  logged in: $2\n";
      $username = $1; 
      $authed = $2; 
   }

   if ( ! $username or ! $authed) { 
       print "Content-type: text/plain\n\n";
       print "Authorization required.\n"; 
       exit(0); 
   }

   # pass sq username to socialCalc 
   $cookievalue="$cookievalue;squsername:$username"; 
   # Process the request and output the results

   my %responsedata = (); # holds results of processing request

   process_request($query, $cookievalue, \%responsedata, "", 1);
   my $content = $responsedata{content};
   my $type = $responsedata{contenttype};
   my $cookie = $responsedata{cookie};

   if (!$content) {
      $content = <<"EOF";
$WKCStrings{"wikicalccgiquitpage1"}
<a href="$ENV{SCRIPT_NAME}">
$WKCStrings{"wikicalccgiquitpage2"}
EOF
      }

   # Output header

   $type ||= "text/html; charset=UTF-8"; # default type

   my $header = "Content-type: $type\nExpires: Thu, 01 Jan 1970 00:00:00 GMT\n";

   if ($cookie) {
      my $fullcookie = new CGI::Cookie(-name => "wkcdata", -value => $cookie);
      $fullcookie->expires($responsedata{cookieexpires}) if $responsedata{cookieexpires};
      $header .= "Set-Cookie: $fullcookie\n";
      }

   print "$header\n"; # print header plus extra line

   # Output content

   print $content;

