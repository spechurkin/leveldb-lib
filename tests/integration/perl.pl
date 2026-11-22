use strict;
use warnings;
use MultiLevelDB;
my $db = MultiLevelDB::Database->open($ENV{ML_INTEROP_PATH});
open my $fixture, '<', $ENV{ML_FIXTURE} or die $!;
while (<$fixture>) {
    s/\r?\n\z//;
    next if /^#/ || !length;
    my ($k, $v) = split /\t/;
    my $key = $k eq '-' ? '' : pack('H*', $k);
    my $value = ($v eq '-' ? '' : pack('H*', $v)) . "\0\xfe";
    die "Perl fixture mismatch\n" unless $db->get($key) eq $value;
}
$db->close;
