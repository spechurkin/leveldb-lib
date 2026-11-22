# MultiLevelDB for Perl

Install Perl 5.20 or later, FFI::Platypus 2.11 or later and the native
MultiLevelDB shared library. Set `MULTILEVELDB_LIBRARY` to the absolute
path to the library file.

## Build and install

From `bindings/perl`, run the following commands.

```sh
perl Makefile.PL
make test
make install
```

On Strawberry Perl, use the make program named by Perl's `$Config{make}`,
typically `gmake`.

## Open a database and write bytes

```perl
use MultiLevelDB;

my $db = MultiLevelDB::Database->open('data', { create_if_missing => 1 });
my $key = "\0\xff";
$db->put($key, "\0\x80");
my $value = $db->get($key);
$db->close;
```

Pass octet strings for keys and values. A missing key returns `undef`. A
stored empty string is defined and has length zero. Native errors throw a
`MultiLevelDB::Error` with `code` and `message` methods.

Close cursors, batches and snapshots when you finish with them. `DESTROY`
provides fallback cleanup. Open a separate database directory in each Perl
ithread. After installing, run `perldoc MultiLevelDB` for the full reference.
