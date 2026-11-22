package MultiLevelDB::Resource;
use strict;
use warnings;
use MultiLevelDB::Native;
sub close {
    my ($self) = @_;
    my $h = delete $self->{handle};
    MultiLevelDB::Native::release($self->{free}, $h) if $h;
}
sub DESTROY {$_[0]->close}
sub CLONE_SKIP {1}

1;
