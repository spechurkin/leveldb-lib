package MultiLevelDB;
use strict;
use warnings;
our $VERSION = '1.0.0';
use MultiLevelDB::Error;
use MultiLevelDB::Database;
use MultiLevelDB::Snapshot;
use MultiLevelDB::Batch;
use MultiLevelDB::Iterator;

1;

__END__

=encoding UTF-8

=head1 NAME

MultiLevelDB - binary-safe Google LevelDB access through stable C ABI 1

=head1 SYNOPSIS

    use MultiLevelDB;
    my $db = MultiLevelDB::Database->open(
        'data', { create_if_missing => 1 }
    );
    $db->put("\0\xff", "\0\x80", { sync => 1 });
    my $value = $db->get("\0\xff"); # octets, or undef if missing
    my $snapshot = $db->create_snapshot;
    my $batch = $db->create_batch;
    $batch->put('a', 'one')->delete('b');
    $db->write($batch);
    my $cursor = $db->iterator(undef, $snapshot);
    $snapshot->close; # cursor keeps its native snapshot reference
    $cursor->seek_to_first;
    while ($cursor->is_valid) {
        my ($key, $bytes) = ($cursor->key, $cursor->value);
        $cursor->next;
    }
    $cursor->status;
    $cursor->close;
    $batch->close;
    $db->close;

=head1 DESCRIPTION

The public packages are the loading/FFI module C<MultiLevelDB>,
C<MultiLevelDB::Database>, C<MultiLevelDB::Batch>, C<MultiLevelDB::Snapshot>,
C<MultiLevelDB::Iterator>, and the shared C<Resource> and C<Error> classes.
The engine is the official Google LevelDB; no alternate storage format or
language adapter is involved. All operations are synchronous in the calling
Perl thread. The native ABI major is checked when the module is loaded.

Set C<MULTILEVELDB_LIBRARY> before loading the module to the matching native
shared library's absolute filename. Otherwise the FFI loader searches for
C<multileveldb>. Keep its platform runtime dependencies discoverable as well.
Perl 5.20+ and L<FFI::Platypus> 2.11+ are required. Resources cannot be shared
across Perl ithreads; open independent directories in separate threads.

=head2 Binary data and paths

Keys and values are defined, non-reference octet strings, including C<''>,
NUL bytes and bytes above 127. Characters above 255 are rejected instead of
being UTF-8 encoded. Perl strings with the UTF8 flag but only byte-range
characters are downgraded to the same octets. Encode application text explicitly
before calling the binary API. Returned strings are owned copies and survive
cursor movement/close. A missing get returns C<undef>; an existing empty value
is the defined empty string.

Paths accept Unicode character strings or valid UTF-8 octets. Encoding and
decoding use C<FB_CROAK>; malformed text is rejected. The native path must be
nonempty without NUL, and relative paths use the process's working directory.
The engine encodes key lengths in 32 bits including an eight-byte internal
suffix, so keys cannot exceed C<UINT32_MAX-8>; values cannot exceed
C<UINT32_MAX>. Platform allocation limits may be lower.

=head2 Errors

Native failures C<die> with a C<MultiLevelDB::Error> object. Copy C<$@> before
another eval. For example:

    eval { $db->put($key, $value) };
    my $error = $@;
    if (ref($error) && $error->isa('MultiLevelDB::Error')) {
        warn $error->code . ': ' . $error->message;
    } elsif ($error) {
        die $error;
    }

Codes are C<NOT_FOUND>, C<INVALID_ARGUMENT>, C<IO_ERROR>, C<CORRUPTION>,
C<DATABASE_LOCKED>, C<DATABASE_CLOSED>, C<UNSUPPORTED>, C<INTERNAL_ERROR>.
Missing gets alone map NOT_FOUND to undef. Invalid cursor access still throws.
Argument-shape errors and invalid Perl option values use ordinary string
exceptions; path conversion may throw Encode errors. Native diagnostics are
preserved rather than parsed by callers. Loading an incompatible/missing ABI
or a missing FFI dependency fails during C<use>.

=head1 MultiLevelDB::Database

An explicitly owned directory handle. There is no C<new> constructor; use C<open>
or C<create>.
Database operations require an open handle. All methods can throw native I/O,
corruption or allocation failures in addition to the specific errors below.

=head2 open

    my $db = MultiLevelDB::Database->open($path, \%open_options);
    my $db = MultiLevelDB::Database->open($path);

Class method returning a newly owned database. C<$path> follows the path rules
above. The optional hash contains opening settings listed below; C<undef>
uses engine defaults, including creation disabled. Parents must already exist.
Settings are copied and need not outlive the call. Throws INVALID_ARGUMENT for
invalid path/options/existence policy, DATABASE_LOCKED for identified directory
lock contention, or another native failure. Closing does not delete data.

=head2 create

    my $db = MultiLevelDB::Database->create($path, \%open_options);
    my $db = MultiLevelDB::Database->create($path);

Creates a new LevelDB and returns its open handle. Parent directories must
exist. Copies optional opening settings, forcing C<create_if_missing> and
C<error_if_exists> true without changing the supplied hash. Existing databases
throw INVALID_ARGUMENT; directory contention may throw DATABASE_LOCKED first.
Path validation and other native errors follow C<open>. Requires native ABI 1.1.

=head2 get

    my $value = $db->get($key, \%read_options, $snapshot);
    my $value = $db->get($key);

Returns an owned octet string or undef if the binary key is absent. Empty keys
and stored empty values are valid. Omitted/undef read options select defaults;
omitted/undef snapshot reads the current view. A snapshot must be an open
C<MultiLevelDB::Snapshot> from this exact DB instance. Throws INVALID_ARGUMENT
for a foreign snapshot/engine length overflow, DATABASE_CLOSED for any closed
participating resource, or engine I/O/corruption/native failures.

=head2 put

    $db->put($key, $value, \%write_options);

Inserts or replaces the exact binary value and returns C<$db> for chaining.
Arguments are borrowed only during this synchronous call. Omitted/undef write
options use nonsynchronous defaults. Empty keys/values are valid. Throws
INVALID_ARGUMENT for encoded-length overflow, DATABASE_CLOSED for a closed
handle, or a native write failure. Invalid Perl byte/option shapes die normally.

=head2 delete

    $db->delete($key, \%write_options);

Deletes the binary key and returns C<$db>. An already absent key also succeeds;
no existence or old-value result is returned. Omitted/undef options select
default writes. Throws for invalid engine lengths, closed DB or write failure.

=head2 create_batch

    my $batch = $db->create_batch;

Returns a new owned, empty C<MultiLevelDB::Batch>. This convenience method
requires an open DB, but the resulting batch is independent and reusable with
any DB. Throws DATABASE_CLOSED or a native allocation error. Also see
C<MultiLevelDB::Batch-E<gt>new>.

=head2 write

    $db->write($batch, \%write_options);

Atomically applies the operations of a live C<MultiLevelDB::Batch> in insertion
order. The batch is neither consumed nor cleared. The optional write-options
hash selects durability. There is no meaningful return value. Throws on invalid
batch/option shapes, closed participating resources or an engine write failure.
This is LevelDB WriteBatch atomicity without transactional reads/conflict checks.

=head2 create_snapshot

    my $snapshot = $db->create_snapshot;

Returns an owned immutable C<MultiLevelDB::Snapshot> of current committed data.
It belongs only to this DB instance. Holding it may retain old table versions
and the engine's lock. Throws DATABASE_CLOSED or a native failure.

=head2 iterator

    my $cursor = $db->iterator(\%read_options, $snapshot);
    my $cursor = $db->iterator;

Returns an owned, initially unpositioned streaming C<MultiLevelDB::Iterator>.
Omitted/undef options select read defaults. Omitted/undef snapshot selects a
current-view cursor. The snapshot must come from this instance. Native cursor
state retains the engine/snapshot independently, so their wrappers may close
after cursor construction. Throws for closed resources, foreign snapshots,
invalid Perl argument shapes or native cursor-creation errors.

=head2 close, DESTROY, CLONE_SKIP

Inherited from C<MultiLevelDB::Resource>; see its individual method contracts.

=head1 MultiLevelDB::Batch

A reusable, database-independent ordered operation sequence. Mutating the batch
does not change a database until C<Database::write> applies it.

=head2 new

    my $batch = MultiLevelDB::Batch->new;

Class constructor returning a new owned empty batch. Throws a native allocation
error on failure. No database or options argument is accepted.

=head2 put

    $batch->put($key, $value);

Appends a binary put, copies both arguments and returns C<$batch> for chaining.
Empty inputs are valid. Throws DATABASE_CLOSED, INVALID_ARGUMENT for engine
length overflow, allocation failure, or ordinary byte-argument errors.

=head2 delete

    $batch->delete($key);

Appends a copied deletion and returns C<$batch>. An absent key will be harmless
when applied. Throws closed/length/allocation errors or ordinary argument errors.

=head2 clear

    $batch->clear;

Discards all pending operations without changing a database; the batch remains
reusable. No meaningful return value. Throws DATABASE_CLOSED or a native error.

=head2 close, DESTROY, CLONE_SKIP

Inherited from C<MultiLevelDB::Resource>.

=head1 MultiLevelDB::Snapshot

Immutable native read view returned by C<Database::create_snapshot>. There is no
public constructor or mutation method. Pass it to C<get> or C<iterator> on its
originating database. The wrapper retains the engine, including its directory
lock; it is not a copied database or backup.

=head2 close, DESTROY, CLONE_SKIP

Inherited from C<MultiLevelDB::Resource>. Closing releases this wrapper's
reference. Previously created snapshot cursors retain their own references.
Do not pass a closed snapshot wrapper to another read.

=head1 MultiLevelDB::Iterator

Streaming bidirectional cursor returned by C<Database::iterator>. No public
constructor exists. It is initially unpositioned. Keys follow the engine's
bytewise comparator; records are not loaded as one in-memory collection.

=head2 seek_to_first

    $cursor->seek_to_first;

Positions at the first record. An empty view succeeds with C<is_valid> false.
No meaningful return value. Throws DATABASE_CLOSED or an engine/native failure.

=head2 seek_to_last

    $cursor->seek_to_last;

Positions at the last record; an empty view succeeds and becomes invalid.
No meaningful return value. Throws closed-resource or engine/native errors.

=head2 seek

    $cursor->seek($key);

Seeks to the first key greater than or equal to the binary target. No qualifying
key is a successful invalid position, not NOT_FOUND. No meaningful return value.
Throws ordinary byte errors, invalid engine-length, closed or engine/native errors.

=head2 next

    $cursor->next;

Advances one record. Moving past the last succeeds and invalidates the cursor.
An already invalid cursor throws NOT_FOUND. No meaningful return value. Closed
resources and engine failures throw; call C<status> after a scan finishes.

=head2 previous

    $cursor->previous;

Moves back one record. Moving before the first succeeds and invalidates.
An already invalid cursor throws NOT_FOUND. No meaningful return value. Also
throws for closed cursors or engine/native failures.

=head2 is_valid

    my $valid = $cursor->is_valid;

Returns 1 for a current record, otherwise 0 for unpositioned, exhausted or failed
cursors. Does not inspect engine status; use C<status>. Throws DATABASE_CLOSED
after close instead of returning false for an unusable wrapper.

=head2 key

    my $key = $cursor->key;

Returns a copied binary current key, valid independently of later cursor movement
or close. Throws NOT_FOUND if invalid, DATABASE_CLOSED, or the engine/native error.

=head2 value

    my $value = $cursor->value;

Returns a copied binary current value, including C<''> for an existing empty
value. The copy survives movement/close. Throws NOT_FOUND if invalid, a closed
resource error, or the recorded engine/native error.

=head2 status

    $cursor->status;

Checks the engine cursor status without moving it. Ordinary unpositioned/end
state succeeds; I/O/corruption/native errors throw. No meaningful return value.
Also throws DATABASE_CLOSED after close.

=head2 close, DESTROY, CLONE_SKIP

Inherited from C<MultiLevelDB::Resource>.

=head1 MultiLevelDB::Resource

Base ownership class for Database, Batch, Snapshot, Iterator and internal options.
It has no public constructor. Call C<close> explicitly to release resources and
locks promptly; destructor timing is only a fallback. Objects are not ithread-safe.

=head2 close

    $resource->close;

Releases the owned native handle once. Repeated close is harmless; there is no
meaningful return value. Further operations throw DATABASE_CLOSED. Closing a DB
does not destroy data; dependent snapshots/cursors retain their native state and
directory lock. Closing a snapshot does not invalidate existing snapshot cursors.

=head2 DESTROY

Perl destruction hook calling C<close>. Do not invoke it manually or free native
pointers yourself. It supplies fallback cleanup if explicit close was omitted.

=head2 CLONE_SKIP

Perl ithread-cloning hook returning 1. Resource instances must not be cloned and
shared as native handles between interpreter threads. Do not call this hook as an
application method; open independent directories in separate threads.

=head1 MultiLevelDB::Error

Native exception objects are created by the adapter, not via a public constructor.
Stringification yields C<CODE: diagnostic>. The native allocation has already
been freed; object fields are Perl-owned copies.

=head2 code

    my $code = $error->code;

Returns the stable status string listed under ERRORS. No arguments, ownership
transfer or I/O occurs; this method does not throw for a valid error object.

=head2 message

    my $message = $error->message;

Returns the original native diagnostic string, possibly empty if allocation
failed. No encoding of keys/values or error-text classification is performed.
No arguments, ownership transfer or I/O occurs.

=head1 OPTIONS

Options are optional hash references, validated for their operation kind.
Unknown names, references instead of hashes, invalid integer text and fields of
the wrong kind die with ordinary Perl errors. Numeric settings must be
nonnegative decimal integers; booleans accept 0/1 only. Native range rejection
throws INVALID_ARGUMENT. A hash is consumed only for the synchronous call.

=head2 Opening settings

=over 4

=item create_if_missing

Boolean, default 0. Create a missing database; parent directories must exist.

=item error_if_exists

Boolean, default 0. Reject an existing DB; lock acquisition may fail first.

=item paranoid_checks

Boolean, default 0. Extra engine consistency checks during opening/recovery.

=item write_buffer_size

Positive byte target, default 4 MiB. Engine minimum/maximum bounds can clamp it.

=item max_open_files

Positive C-int file budget, default 1000; engine bounds can clamp it.

=item block_size

Positive uncompressed block byte target, default 4096; not a record-size limit.

=item block_restart_interval

Positive C-int key count between restart points, default 16.

=item compression

C<none> or C<snappy>, default C<snappy>. Controls newly written table blocks;
does not rewrite existing blocks. Unavailable Snappy throws UNSUPPORTED.

=item cache_size

Nonnegative size_t byte capacity for an owned LRU. Zero means zero capacity.
Omitting this setting retains LevelDB's default cache policy.

=back

=head2 Reading settings

=over 4

=item verify_checksums

Boolean, default 0. Verify checksums for blocks read from storage.

=item fill_cache

Boolean, default 1. Populate the block cache; 0 still permits cached reads.

=back

The snapshot is a separate object argument, not an options hash field.

=head2 Writing settings

=over 4

=item sync

Boolean, default 0. Request LevelDB's synchronous storage flush before success.
This is engine durability, not a separate transaction facility.

=back

=head1 INTERNAL ROUTINES

These functions in C<MultiLevelDB::Native> and C<MultiLevelDB::Options> implement
FFI plumbing and are not exported application API. They are documented to make
source-level ownership auditable;
new adapters should use the C ABI directly rather than depending on them.

=over 4

=item bind($name, \@argument_types, $return_type)

Registers C<ml_$name> as a private Platypus callable in the lexical function map.
It is called during module loading; missing symbols/type definitions abort load.
No meaningful return value is part of the contract.

=item checked($status, $native_error)

Copies the diagnostic, frees the optional native error, then throws
C<MultiLevelDB::Error> for nonzero status. Success has no meaningful return.

=item call($name, @native_arguments)

Appends an owned-error output slot, invokes the registered function, and runs
C<checked>. No meaningful success result; native failures throw.

=item create($name, @native_arguments)

Appends handle/error outputs through C<call> and returns an owned native pointer.
The caller must put it into a resource wrapper with the correct release function.

=item bytes($scalar)

Returns a validated octet copy, downgrading only byte-range characters.
Undefined/reference/wide-character inputs die with an ordinary Perl error.

=item handle($resource, $expected_class)

Validates class and live handle, then returns a borrowed native pointer. Wrong
types die normally; closed objects throw DATABASE_CLOSED. Does not retain it.

=item MultiLevelDB::Options::create($kind, $hash_or_undef)

Returns undef for defaults, otherwise an owned internal Resource containing
kind-specific native options. Validates fields and converts compression names.
Its destructor releases settings even when a setter throws during construction.

=item MultiLevelDB::Options::handle($internal_options_or_undef)

Returns its borrowed options pointer or undef. No ownership transfer occurs.

=item read_buffer($name, @native_arguments)

Calls a buffer-producing native function, copies its explicit length into a Perl
string, and frees the allocation even if conversion throws. Returns octets;
native failure is passed through C<checked> without leaking the error.

=back

=head1 LICENSE

0BSD. The native package carries separate Google LevelDB and Snappy BSD
notices; FFI::Platypus has its own distribution license.

=cut
