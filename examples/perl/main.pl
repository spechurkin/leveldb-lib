use strict;
use warnings;
use MultiLevelDB;
my $db = MultiLevelDB::Database->open('example-db', { create_if_missing => 1 });
my $key = pack('C*', 0, 255);
$db->put($key, pack('C*', 0, 128, 255));
my $value = $db->get($key);
die 'Unexpected value' unless $value eq pack('C*', 0, 128, 255);
$db->close;
