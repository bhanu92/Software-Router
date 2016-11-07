#!/usr/bin/perl

# usage: test.pl topFile command inf
# supported commands: ping, sping, tr, web, unreach, log

#ip_gateway_eth0=172.29.15.201
#ip_vrhost_eth0=172.29.15.200
#ip_vrhost_eth1=172.29.15.198
#ip_vrhost_eth2=172.29.15.214
#ip_server1_eth0=172.29.15.193
#ip_server2_eth0=172.29.15.194
#ip_server3_eth0=172.29.15.211
#ip_server4_eth0=172.29.15.212

#@allinf = ('v1-eth0', 'v2-eth1', 'v3-eth2', 'app1', 'app2');
@allinf = ("eth0", "eth1", "eth2", "s1", "s2", "s3", "s4");
# ping 10 times
$ping = 'ping -c 10';
# ping only once
$ping30 = 'ping -c 30';
# traceroute, min TTL 10, max TTL 20.
$tr = 'traceroute -n -f 10 -m 20 -N 1 -q 1';

$topFile = shift @ARGV;
open(INF, "< $topFile") || die("cannot open topology file!\n");
while($line = <INF>) {
    chomp($line);
    ($f, $i) = split('=', $line);
    $addr{$f} = $i;
#    print "$addr{$f}, $f.\n";
}
close(INF);

$addr{'gw'} = $addr{'ip_gateway_eth0'};
$addr{'eth0'} = $addr{'ip_vrhost_eth0'};
$addr{'eth1'} = $addr{'ip_vrhost_eth1'};
$addr{'eth2'} = $addr{'ip_vrhost_eth2'};
$addr{'s1'} = $addr{'ip_server1_eth0'};
$addr{'s2'} = $addr{'ip_server2_eth0'};
$addr{'s3'} = $addr{'ip_server3_eth0'};
$addr{'s4'} = $addr{'ip_server4_eth0'};

$command = shift @ARGV;

if($ARGV[0] eq 'all') {
    if($command eq 'ping') {
        @ifs = ("eth0", "eth1", "eth2", "s1", "s3");
    }
    elsif ($command eq 'tr') {
        @ifs = ("s1", "s3");
    }
    elsif ($command eq 'web') {
        @ifs = ("s2", "s4");
    }
} else {
    @ifs = @ARGV;
}
#print $command, "\n", @ifs, "\n"; exit;

foreach $f(@ifs) {
    $ip = $addr{$f};
    if($command eq 'ping') {
        print "ping $f 10 times: ";
        $ret = `$ping $ip`;
        if($ret =~ /(\d+)\% packet loss/) {;
            print 100-$1, "%\n";
        } else {
            print $ret;
        }
    }

    if($command eq 'tr') {
        print "traceroute $f: ";
        $ret = `$tr $ip | grep $ip | grep -v traceroute`;
        if($ret =~ /$ip/) {
            print "SUCCEED\n$ret\n";
        } else {
            print "failed:\n$ret\n";
        }
    }

    if($command eq 'web') {
#        `/bin/rm -f  index.html Earl_Johnson--Aint_Nobodys_Business.mp3`;
#        `wget -t 0 "http://$ip/index.html" `;
        `wget "http://$ip:16280" -O /dev/null `;
#        `wget "http://$ip:16280/beautiful-dreamer.mp3" -O /dev/null `;
        `wget "http://$ip:16280/64MB.bin" -O /dev/null `;
#        `/bin/rm -f index.html Earl_Johnson--Aint_Nobodys_Business.mp3`;
#    `/bin/rm -f congrats.jpg strong_bad_natl_anthem.mp3`;
#    `wget -t 0 -c http://$inf{'app1'}/congrats.jpg`;
#    `wget -c http://$inf{'app1'}/strong_bad_natl_anthem.mp3`;
#    `/bin/rm -f congrats.jpg strong_bad_natl_anthem.mp3`;
    }

#if($command eq 'ftp') {
#    `/bin/rm -f sbnatan.mp3`;
#    `wget -t 0 -c "ftp://$inf{'app1'}/pub/sbnatan.mp3" `;
#    `/bin/rm -f sbnatan.mp3`;
#}
}

#if($command eq 'log') {
#    $ip = $addr{"app1"};
#    print "Make sure you've started the router with logging turned on by -l\n";
#    print "sleeping for 30 seconds...\n";
#    sleep 30;
#    `/bin/rm -f  index.html`;
#    `wget "http://$ip/index.html" `;
#    print "sleeping for 20 seconds...\n";
#    sleep 20;
#    `/bin/rm -f  index.html`;
#    `wget "http://$ip/index.html" `;
#    `/bin/rm -f  index.html`;
#}

if($command eq 'unreach') {
    $ip = $addr{"s1"};
#    print "Make sure you've started the router with modified rtable\n";
    print "ping $ip, taking more than 30 seconds: ";
    $ret = `$ping30 $ip`;
    if($ret =~ /Destination Host Unreachable/i) {
        print "Received ICMP Host Unreachable\n";
    } else {
        print "Error: No Host Unreachable\n";
        print $ret;
    }
# making TCP connection to eth0
    $ip = $addr{"eth0"};
    `wget --connect-timeout=2 --tries=1 http://$ip:16280`;
}
      
