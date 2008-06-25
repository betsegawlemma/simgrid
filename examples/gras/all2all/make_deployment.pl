#! /usr/bin/perl

use strict;

sub usage {
    print STDERR <<EOH
Usage: all2all_make_deployment.pl platform_file.xml nb_host size_msg (bcast source?)
  
This script generates a deployment file for the all2all program. It takes 
a SimGrid platform file as first argument and the number of wanted peers as 
second argument. If the amount of peers exceeds the amount of available 
hosts in the deployment file, several peers will be placed on the same host.

The third argument is a size of the message to send ( octets )  

If a fourth argument is passed, this is the source of the broadcast
(given as a number between 0 and nb_host-1).
EOH
      ;
    die "\n";
}

my $input    = shift @ARGV || usage();
my $size_msg = shift @ARGV || usage();
my $source   = shift @ARGV || "";
my $nb_hosts = shift @ARGV || 0;

my @host;

open IN,$input || die "Cannot open $input: $!\n";

while (<IN>) {
  next unless /<host id="([^"]*)"/; # "
  
  push @host, $1;
}

# map { print "$_\n" } @host;

die "No host found in $input. Is it really a SimGrid platform file?\nCheck that you didn't pass a deployment file, for example.\n"
  unless (scalar @host);

if (! $nb_hosts) {
    $nb_hosts = scalar @host;
}


# 
# Build the receiver string

my @receivers;    # strings containing sender argument describing all receivers.

my $it_host=0;    # iterator
my $it_port=4000; # iterator, in case we have so much processes to add that we must change it

for (my $i=0; $i<$nb_hosts; $i++) {
  push (@receivers, "$host[$it_host]:$it_port");
  $it_host ++;
  if ($it_host == scalar @host) {
    $it_host=0;
    $it_port++;
  }
}

#
# and now, really generate the file. Receiver first.

print "<?xml version='1.0'?>\n";
print "<!DOCTYPE platform SYSTEM \"simgrid.dtd\">\n";
print "<platform version=\"2\">\n\n";

for my $r (@receivers) {
    my ($h, $p) = split(':', $r);
    print "  <process host=\"".$h."\" function=\"receiver\">\n";
    print "    <argument value=\"$p\"/><argument value=\"".(length($source)?1:$nb_hosts)."\"/>\n";
    print "  </process>\n\n";
}

#
# Here come the sender(s)

if(length($source)) {
    print "  <process host=\"".$host[$source % (scalar @host)].
	"\" function=\"sender\">\n";
    for my $r (@receivers) {
	print "    <argument value=\"$r\"/>\n"; 
    }
    print "    <argument value=\"$size_msg\"/>\n"; 
    print "  </process>\n";
} else {
    my $i = 0; 
    for my $r (@receivers) {
	my ($h, $p) = split(":", $r); 
        print "  <process host=\"".$h."\" function=\"sender\">\n";
        for (my $j = 0; $j < $nb_hosts; $j++) {
           my $r2 = $receivers[($i + $j) % ($nb_hosts)]; 
           print "    <argument value=\"$r2\"/>\n"; 
        }
        print "    <argument value=\"$size_msg\"/>\n"; 
        print "  </process>\n";
        $i++;
    }
}

print "</platform>\n";

# print "source='$source' nb_hosts=$nb_hosts\n";
