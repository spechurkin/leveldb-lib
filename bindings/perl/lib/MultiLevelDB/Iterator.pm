package MultiLevelDB::Iterator;
use strict;
use warnings;
use MultiLevelDB::Native;
use MultiLevelDB::Resource;
use FFI::Platypus::Buffer qw(scalar_to_buffer);
our @ISA = ('MultiLevelDB::Resource');
sub seek_to_first {MultiLevelDB::Native::call('iterator_seek_first', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}
sub seek_to_last {MultiLevelDB::Native::call('iterator_seek_last', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}
sub next {MultiLevelDB::Native::call('iterator_next', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}
sub previous {MultiLevelDB::Native::call('iterator_previous', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}
sub status {MultiLevelDB::Native::call('iterator_status', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}
sub is_valid {MultiLevelDB::Native::valid(MultiLevelDB::Native::handle($_[0], __PACKAGE__))}
sub seek {
    my ($self, $key) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    my ($k, $n) = scalar_to_buffer($key);
    MultiLevelDB::Native::call('iterator_seek', MultiLevelDB::Native::handle($self, __PACKAGE__), $k, $n);
}
sub key {MultiLevelDB::Native::read_buffer('iterator_key', MultiLevelDB::Native::handle($_[0], __PACKAGE__))}
sub value {MultiLevelDB::Native::read_buffer('iterator_value', MultiLevelDB::Native::handle($_[0], __PACKAGE__))}

1;
