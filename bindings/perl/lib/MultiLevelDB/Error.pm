package MultiLevelDB::Error;
use strict;
use warnings;
use overload '""' => sub {"$_[0]->{code}: $_[0]->{message}"}, fallback => 1;
sub code {$_[0]->{code}}
sub message {$_[0]->{message}}

1;
