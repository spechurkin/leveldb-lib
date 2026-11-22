package MultiLevelDB::Database;
use strict;
use warnings;
use MultiLevelDB::Native;
use MultiLevelDB::Resource;
use MultiLevelDB::Options;
use MultiLevelDB::Snapshot;
use MultiLevelDB::Batch;
use MultiLevelDB::Iterator;
use FFI::Platypus::Buffer qw(scalar_to_buffer);
use Encode qw(encode decode FB_CROAK);
our @ISA = ('MultiLevelDB::Resource');
sub open {
    return _open($_[0], $_[1], $_[2], 'db_open');
}
sub create {
    return _open($_[0], $_[1], $_[2], 'db_create');
}
sub _open {
    my ($class, $path, $opts, $operation) = @_;
    die "Expected a path string\n" unless defined($path) && !ref($path);
    my $unicode = utf8::is_utf8($path) ? $path : decode('UTF-8', $path, FB_CROAK);
    my $encoded = encode('UTF-8', $unicode, FB_CROAK);
    my ($p, $n) = scalar_to_buffer($encoded);
    my $o = MultiLevelDB::Options::create(0, $opts);
    return bless({ handle => MultiLevelDB::Native::create($operation, $p, $n, MultiLevelDB::Options::handle($o)), free => 'db_destroy' }, $class);
}
sub get {
    my ($self, $key, $opts, $snapshot) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    my ($p, $n) = scalar_to_buffer($key);
    my $o = MultiLevelDB::Options::create(1, $opts);
    my $s = defined($snapshot) ? MultiLevelDB::Native::handle($snapshot, 'MultiLevelDB::Snapshot') : undef;
    my $result;
    my $ok = eval {
        $result = MultiLevelDB::Native::read_buffer('db_get', MultiLevelDB::Native::handle($self, __PACKAGE__), $p, $n, MultiLevelDB::Options::handle($o), $s);
        1;
    };
    my $error = $@;
    return undef if !$ok && ref($error) && $error->isa('MultiLevelDB::Error') && $error->code eq 'NOT_FOUND';
    die $error unless $ok;
    return $result;
}
sub put {
    my ($self, $key, $value, $opts) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    $value = MultiLevelDB::Native::bytes($value);
    my ($k, $n) = scalar_to_buffer($key);
    my ($v, $z) = scalar_to_buffer($value);
    my $o = MultiLevelDB::Options::create(2, $opts);
    MultiLevelDB::Native::call('db_put', MultiLevelDB::Native::handle($self, __PACKAGE__), $k, $n, $v, $z, MultiLevelDB::Options::handle($o));
    return $self;
}
sub delete {
    my ($self, $key, $opts) = @_;
    $key = MultiLevelDB::Native::bytes($key);
    my ($k, $n) = scalar_to_buffer($key);
    my $o = MultiLevelDB::Options::create(2, $opts);
    MultiLevelDB::Native::call('db_delete', MultiLevelDB::Native::handle($self, __PACKAGE__), $k, $n, MultiLevelDB::Options::handle($o));
    return $self;
}
sub create_batch {
    MultiLevelDB::Native::handle($_[0], __PACKAGE__);
    return MultiLevelDB::Batch->new;
}
sub write {
    my ($self, $batch, $opts) = @_;
    my $o = MultiLevelDB::Options::create(2, $opts);
    MultiLevelDB::Native::call('batch_write', MultiLevelDB::Native::handle($self, __PACKAGE__), MultiLevelDB::Native::handle($batch, 'MultiLevelDB::Batch'), MultiLevelDB::Options::handle($o));
}
sub create_snapshot {
    my ($self) = @_;
    return bless({ handle => MultiLevelDB::Native::create('snapshot_create', MultiLevelDB::Native::handle($self, __PACKAGE__)), free => 'snapshot_release' }, 'MultiLevelDB::Snapshot');
}
sub iterator {
    my ($self, $opts, $snapshot) = @_;
    my $o = MultiLevelDB::Options::create(1, $opts);
    my $s = defined($snapshot) ? MultiLevelDB::Native::handle($snapshot, 'MultiLevelDB::Snapshot') : undef;
    return bless({ handle => MultiLevelDB::Native::create('iterator_create', MultiLevelDB::Native::handle($self, __PACKAGE__), MultiLevelDB::Options::handle($o), $s), free => 'iterator_destroy' }, 'MultiLevelDB::Iterator');
}

1;
