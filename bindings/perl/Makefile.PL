use strict;
use warnings;
use ExtUtils::MakeMaker;
WriteMakefile(
    NAME             => 'MultiLevelDB', VERSION_FROM => 'lib/MultiLevelDB.pm',
    ABSTRACT         => 'Binary-safe bindings to the multileveldb C ABI',
    AUTHOR           => 'MultiLevelDB contributors', LICENSE => 'open_source',
    MIN_PERL_VERSION => '5.020',
    PERL             => $^X, FULLPERL => $^X,
    META_MERGE       => {
        'meta-spec' => { version => 2 },
        resources   => {
            license    => ['https://opensource.org/license/0bsd'],
            repository => { type => 'git', url => 'https://github.com/spechurkin/leveldb-lib.git', web => 'https://github.com/spechurkin/leveldb-lib' },
            bugtracker => { web => 'https://github.com/spechurkin/leveldb-lib/issues' },
        },
    },
    PREREQ_PM        => { 'FFI::Platypus' => '2.11', 'Encode' => 0, 'Test::More' => 0 },
);
