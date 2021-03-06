#! /usr/bin/perl
use POSIX 'strftime';
use Time::HiRes qw ( setitimer ITIMER_REAL time );
use Crypt::OpenSSL::X509;

my $start = time;

open(IN, $ARGV[0]);

$certs_total = `wc -l $ARGV[0] | cut -d' ' -f1`;
$certs_total =~ s/\n//g;

$i = 1;

sub show_status {
        my $duration = time - $start;
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime $duration;
  		my $hour = $hour - 1;
        my $pct = sprintf ("%0.2f", ($i * 100) / $certs_total);
        my $remaining_time = ($certs_total - $i) * ($duration / $i);
        my ($sec2, $min2, $hour2, $mday2, $mon2, $year2, $wday2, $yday2, $isdst) = localtime $remaining_time;
        $hour2 -= 1;
        print "$i/$certs_total ($pct %) in $hour h $min m $sec s -- remaining time : ~ $hour2 h $min2 m $sec2 s\n";
};


$SIG{ALRM} = \&show_status;
setitimer(ITIMER_REAL, 2, 20);


open(IN2, ">certs_moduli");
while(<IN>) {
	chomp;
	$content = $_;
	system "echo \"-----BEGIN CERTIFICATE-----\" > tmp.pem";
	system "echo \"$content\" | cut -d',' -f2 >> tmp.pem";
	system "echo \"-----END CERTIFICATE-----\" >> tmp.pem";
	$tmp =  `fold -w 64 tmp.pem`;
	system "echo \"$tmp\" > tmp.pem";
	
	my $x509 = Crypt::OpenSSL::X509->new_from_file('tmp.pem');
	
	eval {
		$modulus =  `openssl x509 -noout -in tmp.pem -modulus 2> /dev/null | cut -d'=' -f2`;
		$modulus =~ s/\n//g;
		$str = $str ."\"". $modulus . "\", ";
		print IN2 $modulus."\n";
	} or do {
		print "erreur pubkey\n";
		next;
	};

	$i++;
}
close (IN2);
close(IN);
