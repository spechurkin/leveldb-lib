package MultiLevelDB::Native;
use strict;
use warnings;
use subs qw(bind);
use FFI::Platypus 2.11;
use FFI::Platypus::Buffer qw(scalar_to_buffer buffer_to_scalar);
use MultiLevelDB::Error;
my $ffi = FFI::Platypus->new(api => 2);
$ffi->lib($ENV{MULTILEVELDB_LIBRARY} || 'multileveldb');
my %f;
sub bind {
    my ($name, $args, $return) = @_;
    $f{$name} = $ffi->function("ml_$name", $args, $return)->sub_ref;
}
bind('abi_version', [], 'uint32');
die "Incompatible multileveldb ABI\n" if ($f{abi_version}->() >> 16) != 1;
bind('error_message', [ 'opaque' ], 'string');
bind('error_free', [ 'opaque' ], 'void');
bind('buffer_free', [ 'opaque' ], 'void');
bind('options_create', [ 'uint32', 'opaque*', 'opaque*' ], 'sint32');
bind('options_set', [ 'opaque', 'uint32', 'uint64', 'opaque*' ], 'sint32');
bind('options_destroy', [ 'opaque' ], 'void');
bind('db_open', [ 'opaque', 'size_t', 'opaque', 'opaque*', 'opaque*' ], 'sint32');
bind('db_create', [ 'opaque', 'size_t', 'opaque', 'opaque*', 'opaque*' ], 'sint32');
bind('db_close', [ 'opaque', 'opaque*' ], 'sint32');
bind('db_destroy', [ 'opaque' ], 'void');
bind('db_get', [ 'opaque', 'opaque', 'size_t', 'opaque', 'opaque', 'opaque*', 'size_t*', 'opaque*' ], 'sint32');
bind('db_put', [ 'opaque', 'opaque', 'size_t', 'opaque', 'size_t', 'opaque', 'opaque*' ], 'sint32');
bind('db_delete', [ 'opaque', 'opaque', 'size_t', 'opaque', 'opaque*' ], 'sint32');
bind('iterator_create', [ 'opaque', 'opaque', 'opaque', 'opaque*', 'opaque*' ], 'sint32');
bind('iterator_destroy', [ 'opaque' ], 'void');
for my $op (qw(seek_first seek_last next previous status)) {
    bind("iterator_$op", [ 'opaque', 'opaque*' ], 'sint32');
}
bind('iterator_seek', [ 'opaque', 'opaque', 'size_t', 'opaque*' ], 'sint32');
bind('iterator_valid', [ 'opaque' ], 'uint32');
for my $op (qw(key value)) {bind("iterator_$op", [ 'opaque', 'opaque*', 'size_t*', 'opaque*' ], 'sint32');}
bind('batch_create', [ 'opaque*', 'opaque*' ], 'sint32');
bind('batch_destroy', [ 'opaque' ], 'void');
bind('batch_clear', [ 'opaque', 'opaque*' ], 'sint32');
bind('batch_put', [ 'opaque', 'opaque', 'size_t', 'opaque', 'size_t', 'opaque*' ], 'sint32');
bind('batch_delete', [ 'opaque', 'opaque', 'size_t', 'opaque*' ], 'sint32');
bind('batch_write', [ 'opaque', 'opaque', 'opaque', 'opaque*' ], 'sint32');
bind('snapshot_create', [ 'opaque', 'opaque*', 'opaque*' ], 'sint32');
bind('snapshot_release', [ 'opaque' ], 'void');
my @codes = qw(OK NOT_FOUND INVALID_ARGUMENT IO_ERROR CORRUPTION DATABASE_LOCKED DATABASE_CLOSED UNSUPPORTED INTERNAL_ERROR);
sub checked {
    my ($status, $error) = @_;
    my $message = $error ? $f{error_message}->($error) : '';
    $f{error_free}->($error) if $error;
    die bless({ code => $codes[$status] || 'INTERNAL_ERROR', message => $message }, 'MultiLevelDB::Error') if $status;
}
sub call {
    my ($name, @args) = @_;
    my $error;
    my $status = $f{$name}->(@args, \$error);
    checked($status, $error);
}
sub create {
    my ($name, @args) = @_;
    my $out;
    call($name, @args, \$out);
    return $out;
}
sub bytes {
    my ($value) = @_;
    die "Expected an octet string\n" if !defined($value) || ref($value);
    utf8::downgrade($value, 1) or die "Binary arguments must contain octets, not Unicode characters\n";
    return $value;
}
sub handle {
    my ($resource, $expected) = @_;
    die "Expected $expected\n" unless ref($resource) && $resource->isa($expected);
    die bless({ code => 'DATABASE_CLOSED', message => 'Resource is closed' }, 'MultiLevelDB::Error') unless $resource->{handle};
    return $resource->{handle};
}
sub read_buffer {
    my ($name, @args) = @_;
    my ($out, $length);
    call($name, @args, \$out, \$length);
    my $value;
    my $ok = eval {
        $value = buffer_to_scalar($out, $length);
        1
    };
    my $error = $@;
    $f{buffer_free}->($out);
    die $error unless $ok;
    return $value;
}

sub release {
    my ($name, $handle) = @_;
    $f{$name}->($handle);
}
sub valid {$f{iterator_valid}->($_[0]) ? 1 : 0}
1;
