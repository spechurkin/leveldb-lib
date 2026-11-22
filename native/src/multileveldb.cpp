#include "multileveldb.h"
#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/write_batch.h>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>

#ifdef _WIN32
leveldb::Env* ml_windows_env();
#endif
struct ml_error { ml_status code; std::string message; };
struct ml_options {
  uint32_t kind;
  leveldb::Options open;
  leveldb::ReadOptions read;
  leveldb::WriteOptions write;
  uint64_t cache_size = 0;
  bool custom_cache = false;
};
struct Database {
  std::unique_ptr<leveldb::Cache> cache;
  std::unique_ptr<leveldb::DB> engine;
};
struct ml_db { std::mutex mutex; std::shared_ptr<Database> state; };
struct Snapshot {
  std::shared_ptr<Database> db;
  const leveldb::Snapshot* native;
  ~Snapshot() { db->engine->ReleaseSnapshot(native); }
};
struct ml_snapshot { std::shared_ptr<Snapshot> state; };
struct ml_iterator {
  std::shared_ptr<Database> db;
  std::shared_ptr<Snapshot> snapshot;
  std::unique_ptr<leveldb::Iterator> native;
};
struct ml_batch { leveldb::WriteBatch native; };

namespace {
struct Failure { ml_status code; std::string message; };
[[noreturn]] void fail(ml_status code, const char* message) { throw Failure{code, message}; }
void require(bool condition, const char* message) {
  if (!condition) fail(ML_INVALID_ARGUMENT, message);
}
template<class F> ml_status boundary(ml_error** error, F&& f) noexcept {
  if (error) *error = nullptr;
  ml_status code = ML_OK;
  try { f(); return ML_OK; }
  catch (const Failure& e) {
    code = e.code;
    if (error) { try { *error = new ml_error{code, e.message}; } catch (...) {} }
  } catch (const std::exception& e) {
    code = ML_INTERNAL_ERROR;
    if (error) { try { *error = new ml_error{code, e.what()}; } catch (...) {} }
  } catch (...) {
    code = ML_INTERNAL_ERROR;
    if (error) { try { *error = new ml_error{code, "Unknown native exception"}; } catch (...) {} }
  }
  return code;
}
void checked(const leveldb::Status& s) {
  if (s.ok()) return;
  ml_status code = ML_INTERNAL_ERROR;
  if (s.IsNotFound()) code = ML_NOT_FOUND;
  else if (s.IsCorruption()) code = ML_CORRUPTION;
  else if (s.IsInvalidArgument()) code = ML_INVALID_ARGUMENT;
  else if (s.IsNotSupportedError()) code = ML_UNSUPPORTED;
  else if (s.IsIOError()) {
    code = ML_IO_ERROR;
    const std::string m = s.ToString();
    if (m.find("multileveldb:database_locked") != std::string::npos ||
        m.find("already held by process") != std::string::npos ||
        (m.find("lock ") != std::string::npos &&
         m.find("Resource temporarily unavailable") != std::string::npos)) code = ML_DATABASE_LOCKED;
  }
  throw Failure{code, s.ToString()};
}
leveldb::Slice bytes(const uint8_t* p, size_t n) {
  require(p || !n, "Nonempty buffer has a NULL pointer");
  return leveldb::Slice(p ? reinterpret_cast<const char*>(p) : "", n);
}
leveldb::Slice key_bytes(const uint8_t* p, size_t n) {
  require(n <= UINT32_MAX - UINT64_C(8), "Key exceeds LevelDB's 32-bit internal-key encoding");
  return bytes(p,n);
}
leveldb::Slice value_bytes(const uint8_t* p, size_t n) {
  require(n <= UINT32_MAX, "Value exceeds LevelDB's 32-bit length encoding");
  return bytes(p,n);
}
void output(const leveldb::Slice& s, uint8_t** out, size_t* n) {
  require(out && n, "NULL buffer output");
  auto* p = static_cast<uint8_t*>(std::malloc(s.size() ? s.size() : 1));
  if (!p) throw std::bad_alloc();
  if (s.size()) std::memcpy(p, s.data(), s.size());
  *out = p; *n = s.size();
}
std::shared_ptr<Database> database(ml_db* db) {
  require(db != nullptr, "NULL database handle");
  std::lock_guard<std::mutex> lock(db->mutex);
  if (!db->state) fail(ML_DATABASE_CLOSED, "Database is closed");
  return db->state;
}
void kind(const ml_options* o, uint32_t k) {
  require(!o || o->kind == k, "Wrong options kind");
}
leveldb::ReadOptions reading(const ml_options* o, const ml_snapshot* snapshot,
                            const std::shared_ptr<Database>& db) {
  kind(o, ML_OPTIONS_READ);
  auto r = o ? o->read : leveldb::ReadOptions();
  if (snapshot) {
    require(snapshot->state->db == db, "Snapshot belongs to another database");
    r.snapshot = snapshot->state->native;
  }
  return r;
}
leveldb::WriteOptions writing(const ml_options* o) {
  kind(o, ML_OPTIONS_WRITE);
  return o ? o->write : leveldb::WriteOptions();
}
bool valid_utf8(const std::string& s) {
  size_t i = 0;
  while (i < s.size()) {
    uint32_t c = static_cast<unsigned char>(s[i++]);
    if (c < 0x80) { if (!c) return false; continue; }
    unsigned count; uint32_t min;
    if (c >= 0xc2 && c <= 0xdf) { count=1; c &= 31; min=0x80; }
    else if (c >= 0xe0 && c <= 0xef) { count=2; c &= 15; min=0x800; }
    else if (c >= 0xf0 && c <= 0xf4) { count=3; c &= 7; min=0x10000; }
    else return false;
    if (i + count > s.size()) return false;
    while (count--) {
      const auto d = static_cast<unsigned char>(s[i++]);
      if ((d & 0xc0) != 0x80) return false;
      c = (c << 6) | (d & 63);
    }
    if (c < min || c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff)) return false;
  }
  return true;
}
void iter(ml_iterator* i) { require(i, "NULL iterator handle"); }
void positioned(ml_iterator* i) {
  iter(i); checked(i->native->status());
  if (!i->native->Valid()) fail(ML_NOT_FOUND, "Iterator is not positioned on a record");
}
ml_status open_database(const uint8_t* p, size_t n, const ml_options* o,
                        ml_db** out, ml_error** e, bool create_new) {
  if (out) *out = nullptr;
  return boundary(e, [&] {
    require(out, "NULL database output"); kind(o, ML_OPTIONS_OPEN);
    std::string path = bytes(p,n).ToString();
    require(!path.empty() && valid_utf8(path), "Path must be nonempty UTF-8 without NUL");
    auto state = std::make_shared<Database>();
    auto opts = o ? o->open : leveldb::Options();
    if (create_new) {
      opts.create_if_missing = true;
      opts.error_if_exists = true;
    }
#ifdef _WIN32
    opts.env = ml_windows_env();
#endif
    if (o && o->custom_cache) {
      state->cache.reset(leveldb::NewLRUCache(static_cast<size_t>(o->cache_size)));
      opts.block_cache = state->cache.get();
    }
    leveldb::DB* raw = nullptr;
    auto status = leveldb::DB::Open(opts, path, &raw);
    state->engine.reset(raw); checked(status);
    auto db = std::make_unique<ml_db>(); db->state = std::move(state); *out = db.release();
  });
}
}

#ifndef ML_LIBRARY_VERSION
#define ML_LIBRARY_VERSION "1.0.0"
#endif

extern "C" {
uint32_t ML_CALL ml_abi_version() { return ML_ABI_VERSION; }
const char* ML_CALL ml_library_version() { return ML_LIBRARY_VERSION; }
const char* ML_CALL ml_api_version() { return "1.1.0"; }
const char* ML_CALL ml_leveldb_version() { return "1.23"; }
uint32_t ML_CALL ml_has_feature(uint32_t f) {
  if (f == ML_FEATURE_SNAPPY) return ML_HAVE_SNAPPY;
  return f >= ML_FEATURE_SNAPSHOTS && f <= ML_FEATURE_UTF8_PATHS;
}
int32_t ML_CALL ml_error_code(const ml_error* e) { return e ? e->code : ML_OK; }
const char* ML_CALL ml_error_message(const ml_error* e) { return e ? e->message.c_str() : ""; }
void ML_CALL ml_error_free(ml_error* e) { delete e; }
void ML_CALL ml_buffer_free(uint8_t* p) { std::free(p); }
ml_status ML_CALL ml_options_create(uint32_t k, ml_options** out, ml_error** e) {
  if (out) *out = nullptr;
  return boundary(e, [&] {
    require(out && k <= ML_OPTIONS_WRITE, "Invalid options kind or output");
    auto o = std::make_unique<ml_options>(); o->kind = k; *out = o.release();
  });
}
void ML_CALL ml_options_destroy(ml_options* o) { delete o; }
ml_status ML_CALL ml_options_set(ml_options* o, uint32_t f, uint64_t v, ml_error** e) {
  return boundary(e, [&] {
    require(o, "NULL options");
    if (f <= ML_CACHE_SIZE && f >= ML_CREATE_IF_MISSING) kind(o, ML_OPTIONS_OPEN);
    else if (f == ML_VERIFY_CHECKSUMS || f == ML_FILL_CACHE) kind(o, ML_OPTIONS_READ);
    else if (f == ML_SYNC) kind(o, ML_OPTIONS_WRITE);
    else fail(ML_UNSUPPORTED, "Unknown options field");
    if (f == ML_CREATE_IF_MISSING || f == ML_ERROR_IF_EXISTS || f == ML_PARANOID_CHECKS ||
        f == ML_VERIFY_CHECKSUMS || f == ML_FILL_CACHE || f == ML_SYNC)
      require(v <= 1, "Boolean option must be 0 or 1");
    if (f == ML_WRITE_BUFFER_SIZE || f == ML_BLOCK_SIZE || f == ML_CACHE_SIZE)
      require(v <= std::numeric_limits<size_t>::max(), "Option exceeds size_t");
    if (f == ML_WRITE_BUFFER_SIZE || f == ML_BLOCK_SIZE) require(v > 0, "Size must be positive");
    if (f == ML_MAX_OPEN_FILES || f == ML_BLOCK_RESTART_INTERVAL)
      require(v > 0 && v <= INT_MAX, "Integer option must be positive and fit int");
    switch (f) {
      case ML_CREATE_IF_MISSING: o->open.create_if_missing = v != 0; break;
      case ML_ERROR_IF_EXISTS: o->open.error_if_exists = v != 0; break;
      case ML_PARANOID_CHECKS: o->open.paranoid_checks = v != 0; break;
      case ML_WRITE_BUFFER_SIZE: o->open.write_buffer_size = static_cast<size_t>(v); break;
      case ML_MAX_OPEN_FILES: o->open.max_open_files = static_cast<int>(v); break;
      case ML_BLOCK_SIZE: o->open.block_size = static_cast<size_t>(v); break;
      case ML_BLOCK_RESTART_INTERVAL: o->open.block_restart_interval = static_cast<int>(v); break;
      case ML_COMPRESSION:
        require(v <= ML_COMPRESSION_SNAPPY, "Unknown compression");
        if (v == ML_COMPRESSION_SNAPPY && !ML_HAVE_SNAPPY) fail(ML_UNSUPPORTED, "Snappy is not linked in this build");
        o->open.compression = static_cast<leveldb::CompressionType>(v); break;
      case ML_CACHE_SIZE: o->custom_cache = true; o->cache_size = v; break;
      case ML_VERIFY_CHECKSUMS: o->read.verify_checksums = v != 0; break;
      case ML_FILL_CACHE: o->read.fill_cache = v != 0; break;
      case ML_SYNC: o->write.sync = v != 0; break;
    }
  });
}
ml_status ML_CALL ml_db_open(const uint8_t* p, size_t n, const ml_options* o, ml_db** out, ml_error** e) {
  return open_database(p,n,o,out,e,false);
}
ml_status ML_CALL ml_db_create(const uint8_t* p, size_t n, const ml_options* o, ml_db** out, ml_error** e) {
  return open_database(p,n,o,out,e,true);
}
ml_status ML_CALL ml_db_close(ml_db* db, ml_error** e) {
  return boundary(e, [&] { require(db, "NULL database"); std::lock_guard<std::mutex> l(db->mutex); db->state.reset(); });
}
void ML_CALL ml_db_destroy(ml_db* db) { delete db; }
ml_status ML_CALL ml_db_get(ml_db* db, const uint8_t* k, size_t n, const ml_options* o,
  const ml_snapshot* s, uint8_t** out, size_t* len, ml_error** e) {
  if (out) *out = nullptr;
  if (len) *len = 0;
  return boundary(e, [&] {
    require(out && len, "NULL value output"); auto d = database(db); auto r = reading(o,s,d);
    std::string v; checked(d->engine->Get(r,key_bytes(k,n),&v)); output(v,out,len);
  });
}
ml_status ML_CALL ml_db_put(ml_db* db, const uint8_t* k, size_t n, const uint8_t* v, size_t z,
  const ml_options* o, ml_error** e) {
  return boundary(e, [&] { auto d = database(db); checked(d->engine->Put(writing(o),key_bytes(k,n),value_bytes(v,z))); });
}
ml_status ML_CALL ml_db_delete(ml_db* db, const uint8_t* k, size_t n, const ml_options* o, ml_error** e) {
  return boundary(e, [&] { auto d=database(db); checked(d->engine->Delete(writing(o),key_bytes(k,n))); });
}
ml_status ML_CALL ml_iterator_create(ml_db* db, const ml_options* o, const ml_snapshot* s,
  ml_iterator** out, ml_error** e) {
  if (out) *out = nullptr;
  return boundary(e, [&] {
    require(out, "NULL iterator output"); auto d = database(db); auto r = reading(o,s,d);
    auto i=std::make_unique<ml_iterator>(); i->db=d;
    if (s) i->snapshot=s->state;
    i->native.reset(d->engine->NewIterator(r)); checked(i->native->status()); *out=i.release();
  });
}
void ML_CALL ml_iterator_destroy(ml_iterator* i) { delete i; }
ml_status ML_CALL ml_iterator_seek_first(ml_iterator* i, ml_error** e) {
  return boundary(e,[&]{ iter(i); i->native->SeekToFirst(); checked(i->native->status()); });
}
ml_status ML_CALL ml_iterator_seek_last(ml_iterator* i, ml_error** e) {
  return boundary(e,[&]{ iter(i); i->native->SeekToLast(); checked(i->native->status()); });
}
ml_status ML_CALL ml_iterator_seek(ml_iterator* i, const uint8_t* k, size_t n, ml_error** e) {
  return boundary(e,[&]{ iter(i); i->native->Seek(key_bytes(k,n)); checked(i->native->status()); });
}
ml_status ML_CALL ml_iterator_next(ml_iterator* i, ml_error** e) {
  return boundary(e,[&]{ positioned(i); i->native->Next(); checked(i->native->status()); });
}
ml_status ML_CALL ml_iterator_previous(ml_iterator* i, ml_error** e) {
  return boundary(e,[&]{ positioned(i); i->native->Prev(); checked(i->native->status()); });
}
uint32_t ML_CALL ml_iterator_valid(const ml_iterator* i) { return i && i->native->Valid(); }
ml_status ML_CALL ml_iterator_status(ml_iterator* i, ml_error** e) {
  return boundary(e,[&]{ iter(i); checked(i->native->status()); });
}
ml_status ML_CALL ml_iterator_key(ml_iterator* i, uint8_t** out, size_t* n, ml_error** e) {
  if (out) *out=nullptr;
  if (n) *n=0;
  return boundary(e,[&]{ positioned(i); output(i->native->key(),out,n); });
}
ml_status ML_CALL ml_iterator_value(ml_iterator* i, uint8_t** out, size_t* n, ml_error** e) {
  if (out) *out=nullptr;
  if (n) *n=0;
  return boundary(e,[&]{ positioned(i); output(i->native->value(),out,n); });
}
ml_status ML_CALL ml_batch_create(ml_batch** out, ml_error** e) {
  if (out) *out=nullptr;
  return boundary(e,[&]{ require(out,"NULL batch output"); *out=new ml_batch; });
}
void ML_CALL ml_batch_destroy(ml_batch* b) { delete b; }
ml_status ML_CALL ml_batch_clear(ml_batch* b, ml_error** e) {
  return boundary(e,[&]{ require(b,"NULL batch"); b->native.Clear(); });
}
ml_status ML_CALL ml_batch_put(ml_batch* b, const uint8_t* k, size_t n, const uint8_t* v, size_t z, ml_error** e) {
  return boundary(e,[&]{ require(b,"NULL batch"); b->native.Put(key_bytes(k,n),value_bytes(v,z)); });
}
ml_status ML_CALL ml_batch_delete(ml_batch* b, const uint8_t* k, size_t n, ml_error** e) {
  return boundary(e,[&]{ require(b,"NULL batch"); b->native.Delete(key_bytes(k,n)); });
}
ml_status ML_CALL ml_batch_write(ml_db* db, const ml_batch* b, const ml_options* o, ml_error** e) {
  return boundary(e,[&]{ require(b,"NULL batch"); auto d=database(db); checked(d->engine->Write(writing(o),const_cast<leveldb::WriteBatch*>(&b->native))); });
}
ml_status ML_CALL ml_snapshot_create(ml_db* db, ml_snapshot** out, ml_error** e) {
  if (out) *out=nullptr;
  return boundary(e,[&]{
    require(out,"NULL snapshot output"); auto d=database(db);
    auto state=std::make_shared<Snapshot>(); state->db=d; state->native=d->engine->GetSnapshot();
    auto s=std::make_unique<ml_snapshot>(); s->state=std::move(state); *out=s.release();
  });
}
void ML_CALL ml_snapshot_release(ml_snapshot* s) { delete s; }
}
