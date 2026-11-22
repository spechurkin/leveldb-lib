/**
 * @file multileveldb.h
 * @brief Binary-safe, opaque-handle C ABI 1 for Google LevelDB.
 *
 * Inputs are borrowed during calls. NULL byte pointers require zero length.
 * Keys/values are never text-decoded. LevelDB's encoded-length bounds are
 * UINT32_MAX-8 bytes per key and UINT32_MAX bytes per value; runtime allocation
 * capacities can impose lower limits. Output buffers use ml_buffer_free only.
 *
 * Error outputs are optional; valid output slots must be distinct writable
 * objects. Free a previous error before reusing its slot. A native error
 * allocation failure can leave the error output NULL while status is nonzero.
 * Fallible exports catch C++ exceptions. Invalid/forged pointers are not handles.
 *
 * Free/destroy functions accept NULL; owned non-NULL objects are freed once.
 * Logical ml_db_close is repeatable. DB operations/close can run concurrently,
 * but handle destruction requires exclusive access. Per-object cursor, batch
 * and options operations require external serialization. Dependent objects
 * retain the engine and its directory lock independently of the DB wrapper.
 */
#ifndef MULTILEVELDB_H
#define MULTILEVELDB_H
#include <stddef.h>
#include <stdint.h>
#ifdef _WIN32
# ifdef ML_BUILD
/** Export annotation when compiling the native Windows library. */
#  define ML_API __declspec(dllexport)
# else
/** Import annotation for Windows clients. */
#  define ML_API __declspec(dllimport)
# endif
/** Windows cdecl calling convention, independent of C++ ABI. */
# define ML_CALL __cdecl
#else
/** Public symbol visibility on Unix-like targets. */
# define ML_API __attribute__((visibility("default")))
/** Platform-default C calling convention on Unix-like targets. */
# define ML_CALL
#endif
#ifdef __cplusplus
extern "C" {
#endif
/** ABI major in the high 16 bits and minor in the low 16 bits. */
#define ML_ABI_VERSION UINT32_C(0x00010001)
/** Signed 32-bit status; compare with the ML_* result constants. */
typedef int32_t ml_status;
/** Stable ABI identifiers; existing numeric values never change. */
enum {
  ML_OK=0, /**< Success. */
  ML_NOT_FOUND=1, /**< Missing key or invalid cursor position. */
  ML_INVALID_ARGUMENT=2, /**< Invalid path/input/options/encoded length or foreign snapshot. */
  ML_IO_ERROR=3, /**< Engine/filesystem I/O failure, including unclassified lock diagnostics. */
  ML_CORRUPTION=4, /**< Engine-detected malformed/corrupt data. */
  ML_DATABASE_LOCKED=5, /**< Identified DB-directory lock contention. */
  ML_DATABASE_CLOSED=6, /**< A logically closed database handle was used. */
  ML_UNSUPPORTED=7, /**< Unknown setting or unavailable capability. */
  ML_INTERNAL_ERROR=8, /**< Unexpected native exception or unrecognized status. */
};
/** Stable ABI identifiers; existing numeric values never change. */
enum {
  ML_OPTIONS_OPEN=0, /**< Opening/engine tuning settings. */
  ML_OPTIONS_READ=1, /**< Read checksum/cache settings. */
  ML_OPTIONS_WRITE=2, /**< Write durability settings. */
};
/** Stable ABI identifiers; existing numeric values never change. */
enum {
  ML_CREATE_IF_MISSING=1, /**< OPEN boolean; default 0; parent directories must exist. */
  ML_ERROR_IF_EXISTS=2, /**< OPEN boolean; default 0; directory lock acquisition can fail first. */
  ML_PARANOID_CHECKS=3, /**< OPEN boolean; default 0; extra recovery consistency checks. */
  ML_WRITE_BUFFER_SIZE=4, /**< OPEN positive bytes; default 4 MiB; engine clamps. */
  ML_MAX_OPEN_FILES=5, /**< OPEN positive C-int file budget; default 1000; engine clamps. */
  ML_BLOCK_SIZE=6, /**< OPEN positive uncompressed block byte target; default 4096; not a record limit. */
  ML_BLOCK_RESTART_INTERVAL=7, /**< OPEN positive C-int keys between restarts; default 16. */
  ML_COMPRESSION=8, /**< OPEN compression ID; default SNAPPY; affects new blocks only. */
  ML_CACHE_SIZE=9, /**< OPEN size_t byte capacity; zero installs a zero-capacity LRU; unset uses engine policy. */
  ML_VERIFY_CHECKSUMS=10, /**< READ boolean; default 0; verify blocks read from storage. */
  ML_FILL_CACHE=11, /**< READ boolean; default 1; false still permits already-cached reads. */
  ML_SYNC=12, /**< WRITE boolean; default 0; request the engine synchronous flush. */
};
/** Stable ABI identifiers; existing numeric values never change. */
enum {
  ML_COMPRESSION_NONE=0, /**< Write uncompressed new table blocks. */
  ML_COMPRESSION_SNAPPY=1, /**< Compress new blocks with official Snappy. */
};
/** Stable ABI identifiers; existing numeric values never change. */
enum {
  ML_FEATURE_SNAPSHOTS=1, /**< Native snapshot read views. */
  ML_FEATURE_BATCHES=2, /**< Atomic native write batches. */
  ML_FEATURE_UTF8_PATHS=3, /**< Strict UTF-8 paths, using wide Win32 filesystem calls on Windows. */
  ML_FEATURE_SNAPPY=4, /**< Linked Snappy table compression/decompression. */
};
/** Owned DB handle; logical close retains the handle, destroy frees it. */
typedef struct ml_db ml_db;
/** Opaque kind-specific settings copied by database operations. */
typedef struct ml_options ml_options;
/** Owned streaming cursor retaining engine and optional snapshot. */
typedef struct ml_iterator ml_iterator;
/** Owned, reusable ordered sequence for atomic WriteBatch application. */
typedef struct ml_batch ml_batch;
/** Owned immutable read view retaining its originating engine. */
typedef struct ml_snapshot ml_snapshot;
/** Owned status/message object; borrowed message lasts until error_free. */
typedef struct ml_error ml_error;

/**
 * @brief Query the loaded ABI without creating resources.
 * @return Packed (major << 16) plus minor; require major 1.
 */
ML_API uint32_t ML_CALL ml_abi_version(void);
/**
 * @brief Query distribution SemVer, independent of API and engine versions.
 * @return Static NUL-terminated string; never free it.
 */
ML_API const char* ML_CALL ml_library_version(void);
/**
 * @brief Query public API SemVer.
 * @return Static NUL-terminated string; never free it.
 */
ML_API const char* ML_CALL ml_api_version(void);
/**
 * @brief Query the linked Google LevelDB version.
 * @return Static NUL-terminated string; never free it.
 */
ML_API const char* ML_CALL ml_leveldb_version(void);
/**
 * @brief Test an optional capability without creating resources.
 * @param feature ML_FEATURE_* capability identifier.
 * @return 1 if supported, otherwise 0, including unknown feature IDs.
 */
ML_API uint32_t ML_CALL ml_has_feature(uint32_t feature);
/**
 * @brief Inspect an error without transferring ownership.
 * @param error Borrowed error object, or NULL; no ownership is transferred.
 * @return Stable status, or ML_OK for NULL.
 */
ML_API int32_t ML_CALL ml_error_code(const ml_error* error);
/**
 * @brief Borrow the original native diagnostic.
 * @param error Borrowed error object, or NULL; no ownership is transferred.
 * @return NUL-terminated message valid until error_free; empty static string for NULL.
 */
ML_API const char* ML_CALL ml_error_message(const ml_error* error);
/**
 * @brief Free one owned error and invalidate its borrowed message.
 *
 * Accepts NULL. Free a non-NULL allocation once.
 * @param error Owned error allocation, or NULL; free a non-NULL object once.
 */
ML_API void ML_CALL ml_error_free(ml_error* error);
/**
 * @brief Free an allocation returned by get or iterator key/value.
 *
 * Accepts NULL. Never pass user input, stack memory or an already freed allocation.
 * @param buffer Owned library allocation, or NULL.
 */
ML_API void ML_CALL ml_buffer_free(uint8_t* buffer);

/* Options are opaque and extensible. NULL means the engine defaults.
 * Each option belongs to exactly one kind; invalid combinations are rejected. */
/**
 * @brief Allocate options initialized to the selected kind's engine defaults.
 *
 * The output owns one ml_options; destroy with ml_options_destroy.
 * @param kind ML_OPTIONS_OPEN, ML_OPTIONS_READ or ML_OPTIONS_WRITE.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for invalid kind/output, or a native allocation error.
 */
ML_API ml_status ML_CALL ml_options_create(uint32_t kind, ml_options** out, ml_error** error);
/**
 * @brief Change a kind-appropriate option with range validation.
 *
 * Booleans accept only 0/1. Sizes fit size_t; positive file/restart counts fit C int. LevelDB may clamp accepted tuning values. Cache zero installs a zero-capacity LRU; omitting cache-size uses engine policy.
 * @param options Owned options object; only destroy permits NULL.
 * @param field Stable option ID; must belong to this options kind.
 * @param value Unsigned numeric setting: 0/1 boolean, compression ID, or engine tuning value.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK; ML_INVALID_ARGUMENT for wrong kind/range; ML_UNSUPPORTED for an unknown field/unavailable Snappy; or a native error.
 */
ML_API ml_status ML_CALL ml_options_set(ml_options* options, uint32_t field, uint64_t value, ml_error** error);
/**
 * @brief Destroy options after calls have copied their settings.
 *
 * Accepts NULL; a non-NULL handle must be destroyed once with no concurrent use.
 * @param options Owned options object; only destroy permits NULL.
 */
ML_API void ML_CALL ml_options_destroy(ml_options* options);

/* Paths are strict UTF-8, length-delimited, and cannot contain NUL.
 * close is idempotent and keeps the handle allocated; destroy also frees it.
 * Dependent objects retain the engine. Their iterators remain usable after close. */
/**
 * @brief Open a standard Google LevelDB directory with the bytewise comparator.
 *
 * Options must have OPEN kind; NULL disables creation. Parent directories must exist. The output owns one DB handle. Retained dependents can keep the engine lock alive after close.
 * @param path Borrowed nonempty strict UTF-8 path without embedded NUL; relative to working directory.
 * @param length Corresponding buffer byte length.
 * @param options Kind-appropriate live options, or NULL when the operation allows defaults.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK; ML_INVALID_ARGUMENT for invalid path/options or failed existence policy; ML_DATABASE_LOCKED for identified contention; or an engine/native error.
 */
ML_API ml_status ML_CALL ml_db_open(const uint8_t* path, size_t length, const ml_options* options, ml_db** out, ml_error** error);
/**
 * @brief Create a new standard Google LevelDB directory and return its open handle.
 *
 * Copies OPEN settings (or defaults for NULL), forcing create-if-missing and
 * error-if-exists without changing the supplied options. Parent directories
 * must exist. An existing database is never opened or overwritten. Directory
 * lock acquisition can fail before the existence check. Available since ABI 1.1.
 * @param path Borrowed nonempty strict UTF-8 path without embedded NUL; relative to working directory.
 * @param length Corresponding buffer byte length.
 * @param options OPEN settings, or NULL for defaults; copied during the call.
 * @param out Required owned database output; NULL on failure; destroy with ml_db_destroy.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK; ML_INVALID_ARGUMENT for an existing DB or invalid path/options;
 * ML_DATABASE_LOCKED for identified contention; or an engine/native error.
 */
ML_API ml_status ML_CALL ml_db_create(const uint8_t* path, size_t length, const ml_options* options, ml_db** out, ml_error** error);
/**
 * @brief Logically close this live handle without deleting data or freeing the handle.
 *
 * Repeatable. Further DB operations return ML_DATABASE_CLOSED; in-flight calls may finish. Existing iterators remain usable. Later call ml_db_destroy.
 * @param db Live handle, including logically closed handles; destroy also permits NULL.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for NULL, or a native error.
 */
ML_API ml_status ML_CALL ml_db_close(ml_db* db, ml_error** error);
/**
 * @brief Close if needed and free one database handle.
 *
 * Accepts NULL. Requires exclusive handle access; never reuse a destroyed pointer. Existing cursors/snapshots independently retain the engine and lock.
 * @param db Live handle, including logically closed handles; destroy also permits NULL.
 */
ML_API void ML_CALL ml_db_destroy(ml_db* db);
/**
 * @brief Read one binary value as an independent allocation.
 *
 * Free a successful value with ml_buffer_free. Output pointer/length reset to NULL/zero before work. A snapshot must originate from this exact DB state.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param read_options READ settings, or NULL for checksum-off/cache-fill-on defaults; copied by the call.
 * @param snapshot Same-instance snapshot, or NULL for a current read view.
 * @param value Required owned-buffer output; NULL on failure, non-NULL even for empty success.
 * @param value_len Required byte-length output; zero on failure or valid empty data.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK (non-NULL allocation even when empty); ML_NOT_FOUND if absent; ML_DATABASE_CLOSED, ML_INVALID_ARGUMENT, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_db_get(ml_db* db, const uint8_t* key, size_t key_len, const ml_options* read_options, const ml_snapshot* snapshot, uint8_t** value, size_t* value_len, ml_error** error);
/**
 * @brief Insert or replace exactly the supplied binary value.
 *
 * Empty keys and values are valid. Inputs are borrowed only during the call; no text conversion occurs.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param value Borrowed binary value; NULL only when its byte length is zero.
 * @param value_len Value length in bytes, including embedded zero bytes.
 * @param write_options WRITE settings, or NULL for nonsynchronous defaults; copied by the call.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT, ML_DATABASE_CLOSED, or an engine/native write error.
 */
ML_API ml_status ML_CALL ml_db_put(ml_db* db, const uint8_t* key, size_t key_len, const uint8_t* value, size_t value_len, const ml_options* write_options, ml_error** error);
/**
 * @brief Delete one binary key; an already absent key also succeeds.
 *
 * No previous value or existence result is returned.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param write_options WRITE settings, or NULL for nonsynchronous defaults; copied by the call.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT, ML_DATABASE_CLOSED, or an engine/native write error.
 */
ML_API ml_status ML_CALL ml_db_delete(ml_db* db, const uint8_t* key, size_t key_len, const ml_options* write_options, ml_error** error);
/**
 * @brief Allocate an initially unpositioned streaming cursor.
 *
 * The owned cursor retains its engine and optional snapshot independently. Seek before accessing records; closing their wrappers does not invalidate this cursor. Destroy promptly to release versions/locks.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param read_options READ settings, or NULL for checksum-off/cache-fill-on defaults; copied by the call.
 * @param snapshot Same-instance snapshot, or NULL for a current read view.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT (including foreign snapshot), ML_DATABASE_CLOSED, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_create(ml_db* db, const ml_options* read_options, const ml_snapshot* snapshot, ml_iterator** out, ml_error** error);
/**
 * @brief Destroy a cursor and release its snapshot/engine references.
 *
 * Accepts NULL; destroy a non-NULL cursor once with no concurrent calls.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 */
ML_API void ML_CALL ml_iterator_destroy(ml_iterator* iterator);
/**
 * @brief Seek to the first record; an empty view succeeds with an invalid position.
 *
 * Check ml_iterator_valid before accessing a record.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for NULL, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_seek_first(ml_iterator* iterator, ml_error** error);
/**
 * @brief Seek to the last record; an empty view succeeds with an invalid position.
 *
 * Check ml_iterator_valid before accessing a record.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for NULL, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_seek_last(ml_iterator* iterator, ml_error** error);
/**
 * @brief Seek to the first key greater than or equal to the target in bytewise order.
 *
 * No qualifying record leaves the cursor invalid; it is not ML_NOT_FOUND.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK even when no qualifying record exists; ML_INVALID_ARGUMENT or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_seek(ml_iterator* iterator, const uint8_t* key, size_t key_len, ml_error** error);
/**
 * @brief Advance one record; passing the last record succeeds and invalidates the cursor.
 *
 * Check ml_iterator_status after a scan finishes.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_NOT_FOUND if already invalid, ML_INVALID_ARGUMENT for NULL, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_next(ml_iterator* iterator, ml_error** error);
/**
 * @brief Move backward; passing the first record succeeds and invalidates the cursor.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_NOT_FOUND if already invalid, ML_INVALID_ARGUMENT for NULL, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_previous(ml_iterator* iterator, ml_error** error);
/**
 * @brief Inspect positioning without checking the recorded engine status.
 *
 * A false result does not replace ml_iterator_status. Serialize against movement/destruction.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @return 1 for a current record; 0 for NULL, unpositioned, exhausted or failed cursors.
 */
ML_API uint32_t ML_CALL ml_iterator_valid(const ml_iterator* iterator);
/* Copies, including iterator key/value, always belong to the caller. */
/**
 * @brief Copy the current binary key into a caller-owned allocation.
 *
 * Free with ml_buffer_free. The copy survives movement/destruction. Outputs reset to NULL/zero before work.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param length Required byte-length output; initialized to zero.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_NOT_FOUND if invalid, ML_INVALID_ARGUMENT, or a recorded engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_key(ml_iterator* iterator, uint8_t** out, size_t* length, ml_error** error);
/**
 * @brief Copy the current binary value, preserving zero-byte values.
 *
 * Free with ml_buffer_free. The copy survives movement/destruction. Outputs reset to NULL/zero before work.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param length Required byte-length output; initialized to zero.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_NOT_FOUND if invalid, ML_INVALID_ARGUMENT, or a recorded engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_value(ml_iterator* iterator, uint8_t** out, size_t* length, ml_error** error);
/**
 * @brief Inspect the engine cursor error without moving it.
 * @param iterator Live cursor; externally serialize access against movement and destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK for ordinary exhaustion/unpositioned state; ML_INVALID_ARGUMENT for NULL; or the recorded engine/native error.
 */
ML_API ml_status ML_CALL ml_iterator_status(ml_iterator* iterator, ml_error** error);
/**
 * @brief Allocate an empty, database-independent operation sequence.
 *
 * The output owns one batch. Adding operations does not write until ml_batch_write.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for NULL output, or a native allocation error.
 */
ML_API ml_status ML_CALL ml_batch_create(ml_batch** out, ml_error** error);
/**
 * @brief Free a batch without applying queued operations.
 *
 * Accepts NULL; destroy a non-NULL handle once with exclusive access.
 * @param batch Live database-independent batch; externally serialize use/mutation/destruction.
 */
ML_API void ML_CALL ml_batch_destroy(ml_batch* batch);
/**
 * @brief Discard queued operations without touching a database.
 *
 * The same batch remains usable.
 * @param batch Live database-independent batch; externally serialize use/mutation/destruction.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for NULL, or a native error.
 */
ML_API ml_status ML_CALL ml_batch_clear(ml_batch* batch, ml_error** error);
/**
 * @brief Append a put by copying both byte sequences.
 *
 * Empty keys/values are supported. Does not modify any database yet.
 * @param batch Live database-independent batch; externally serialize use/mutation/destruction.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param value Borrowed binary value; NULL only when its byte length is zero.
 * @param value_len Value length in bytes, including embedded zero bytes.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for invalid lengths/inputs, or a native error.
 */
ML_API ml_status ML_CALL ml_batch_put(ml_batch* batch, const uint8_t* key, size_t key_len, const uint8_t* value, size_t value_len, ml_error** error);
/**
 * @brief Append a copied deletion without applying it.
 *
 * An absent key is harmless when the batch is applied.
 * @param batch Live database-independent batch; externally serialize use/mutation/destruction.
 * @param key Borrowed binary key/target; NULL only when key_len is zero.
 * @param key_len Key length in bytes, including embedded zero bytes.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT for invalid input/length, or a native error.
 */
ML_API ml_status ML_CALL ml_batch_delete(ml_batch* batch, const uint8_t* key, size_t key_len, ml_error** error);
/**
 * @brief Atomically apply batch operations in insertion order.
 *
 * The batch is neither consumed nor cleared and can be reused with any DB. This supplies WriteBatch atomicity, not transactional reads/conflict detection. Serialize batch mutation/destruction against this call.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param batch Live database-independent batch; externally serialize use/mutation/destruction.
 * @param write_options WRITE settings, or NULL for nonsynchronous defaults; copied by the call.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT, ML_DATABASE_CLOSED, or an engine/native error.
 */
ML_API ml_status ML_CALL ml_batch_write(ml_db* db, const ml_batch* batch, const ml_options* write_options, ml_error** error);
/**
 * @brief Capture an immutable native read view of current committed data.
 *
 * The owned snapshot retains its originating engine/lock. Pass only to that DB instance. Holding it can retain older table versions.
 * @param db Live open database handle, unless close/destroy semantics explicitly permit closed/NULL.
 * @param out Required owned-resource/buffer output slot; NULL on failure.
 * @param error Optional owned-error output, initialized to NULL; free failures with ml_error_free.
 * @return ML_OK, ML_INVALID_ARGUMENT, ML_DATABASE_CLOSED, or a native error.
 */
ML_API ml_status ML_CALL ml_snapshot_create(ml_db* db, ml_snapshot** out, ml_error** error);
/**
 * @brief Release this owned snapshot reference.
 *
 * Accepts NULL; release a non-NULL handle once without concurrent use of that handle. Existing snapshot iterators retain their own references; immutable snapshot reads may otherwise run concurrently.
 * @param snapshot Owned snapshot reference, or NULL.
 */
ML_API void ML_CALL ml_snapshot_release(ml_snapshot* snapshot);
#ifdef __cplusplus
}
#endif
#endif
