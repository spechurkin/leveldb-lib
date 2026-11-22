package MultiLevelDB::Batch;
use strict;
use warnings;
use MultiLevelDB::Native;
use MultiLevelDB::Resource;
use FFI::Platypus::Buffer qw(scalar_to_buffer);
our @ISA = ('MultiLevelDB::Resource');
sub new {bless({ handle => MultiLevelDB::Native::create('batch_create'), free => 'batch_destroy' }, $_[0])}
sub put {
    my ($self, $key, $value) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    $value = MultiLevelDB::Native::bytes($value);
    my ($k, $n) = scalar_to_buffer($key);
    my ($v, $z) = scalar_to_buffer($value);
    MultiLevelDB::Native::call('batch_put', MultiLevelDB::Native::handle($self, __PACKAGE__), $k, $n, $v, $z);
    return $self;
}
sub delete {
    my ($self, $key) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    my ($k, $n) = scalar_to_buffer($key);
    MultiLevelDB::Native::call('batch_delete', MultiLevelDB::Native::handle($self, __PACKAGE__), $k, $n);
    return $self;
}
sub clear {MultiLevelDB::Native::call('batch_clear', MultiLevelDB::Native::handle($_[0], __PACKAGE__));}

1;
