package MultiLevelDB::Options;
use strict;
use warnings;
use MultiLevelDB::Native;
use MultiLevelDB::Resource;

my %fields = (create_if_missing => [ 0, 1 ], error_if_exists => [ 0, 2 ], paranoid_checks => [ 0, 3 ],
    write_buffer_size           => [ 0, 4 ], max_open_files => [ 0, 5 ], block_size => [ 0, 6 ], block_restart_interval => [ 0, 7 ],
    compression                 => [ 0, 8 ], cache_size => [ 0, 9 ], verify_checksums => [ 1, 10 ], fill_cache => [ 1, 11 ], sync => [ 2, 12 ]);
sub create {
    my ($kind, $values) = @_;
    return undef unless defined $values;
    die "Expected options hash reference\n" unless ref($values) eq 'HASH';
    my $self = bless({ handle => MultiLevelDB::Native::create('options_create', $kind), free => 'options_destroy' }, 'MultiLevelDB::Resource');
    for my $name (sort keys %$values) {
        my $field = $fields{$name};
        die "Unknown option $name\n" unless $field && $field->[0] == $kind;
        my $v = $values->{$name};
        if ($name eq 'compression') {
            die "compression must be none or snappy\n" unless defined($v) && ($v eq 'none' || $v eq 'snappy');
            $v = $v eq 'snappy' ? 1 : 0;
        }
        die "Option $name must be a nonnegative integer\n" unless defined($v) && !ref($v) && $v =~ /\A\d+\z/;
        MultiLevelDB::Native::call('options_set', $self->{handle}, $field->[1], $v);
    }
    return $self;
}
sub handle {return defined $_[0] ? $_[0]->{handle} : undef;}
1;
