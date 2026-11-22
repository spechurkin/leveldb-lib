#include "multileveldb.h"
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc,char** argv) {
  assert(argc==2); const std::string root=argv[1];
  ml_error* e=nullptr;
  auto expect=[&](ml_status s,ml_status wanted) {
    if(s!=wanted) std::fprintf(stderr,"Expected %d, got %d: %s\n",wanted,s,ml_error_message(e));
    assert(s==wanted); assert(e); assert(ml_error_code(e)==wanted); assert(*ml_error_message(e)); ml_error_free(e); e=nullptr;
  };
  ml_options* o=nullptr;
  expect(ml_options_create(99,&o,&e),ML_INVALID_ARGUMENT); assert(!o);
  expect(ml_options_create(0,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_options_set(nullptr,1,0,&e),ML_INVALID_ARGUMENT);
  assert(ml_options_create(0,&o,&e)==ML_OK);
  for(uint32_t field:{4u,5u,6u,7u}) expect(ml_options_set(o,field,0,&e),ML_INVALID_ARGUMENT);
  expect(ml_options_set(o,5,UINT64_MAX,&e),ML_INVALID_ARGUMENT);
  expect(ml_options_set(o,8,2,&e),ML_INVALID_ARGUMENT);
  assert(ml_has_feature(ML_FEATURE_SNAPPY)); assert(ml_options_set(o,8,1,&e)==ML_OK);
  assert(ml_options_set(o,1,1,&e)==ML_OK);
  auto open=[&](const std::string& path,ml_options* opts,ml_db** out) {
    return ml_db_open(reinterpret_cast<const uint8_t*>(path.data()),path.size(),opts,out,&e);
  };
  auto create=[&](const std::string& path,ml_options* opts,ml_db** out) {
    return ml_db_create(reinterpret_cast<const uint8_t*>(path.data()),path.size(),opts,out,&e);
  };
  const std::string fresh=root+"-create";
  std::filesystem::remove_all(std::filesystem::u8path(fresh));
  ml_db* created=nullptr;
  expect(ml_db_create(nullptr,0,nullptr,&created,&e),ML_INVALID_ARGUMENT); assert(!created);
  expect(ml_db_create(nullptr,1,nullptr,&created,&e),ML_INVALID_ARGUMENT); assert(!created);
  expect(create(std::string("bad\0path",8),nullptr,&created),ML_INVALID_ARGUMENT); assert(!created);
  expect(create(std::string("\xff",1),nullptr,&created),ML_INVALID_ARGUMENT); assert(!created);
  expect(create(fresh,nullptr,nullptr),ML_INVALID_ARGUMENT);
  assert(create(fresh,nullptr,&created)==ML_OK);
  assert(ml_db_put(created,nullptr,0,reinterpret_cast<const uint8_t*>("kept"),4,nullptr,&e)==ML_OK);
  ml_db* rejected=nullptr;
  expect(create(fresh,nullptr,&rejected),ML_DATABASE_LOCKED); assert(!rejected);
  ml_db_destroy(created);
  expect(create(fresh,o,&rejected),ML_INVALID_ARGUMENT); assert(!rejected);
  assert(open(fresh,o,&created)==ML_OK);
  uint8_t* kept=nullptr; size_t kept_size=0;
  assert(ml_db_get(created,nullptr,0,nullptr,nullptr,&kept,&kept_size,&e)==ML_OK);
  assert(kept_size==4 && std::string(reinterpret_cast<char*>(kept),kept_size)=="kept");
  ml_buffer_free(kept); ml_db_destroy(created);
  std::filesystem::remove_all(std::filesystem::u8path(fresh));
  assert(ml_options_set(o,ML_CREATE_IF_MISSING,0,&e)==ML_OK);
  assert(ml_options_set(o,ML_CACHE_SIZE,0,&e)==ML_OK);
  assert(create(fresh,o,&created)==ML_OK); ml_db_destroy(created);
  std::filesystem::remove_all(std::filesystem::u8path(fresh));
  expect(open(fresh,o,&created),ML_INVALID_ARGUMENT); assert(!created);
  const auto missing_parent=create(fresh+"/missing/db",nullptr,&created);
  expect(missing_parent,missing_parent==ML_NOT_FOUND?ML_NOT_FOUND:ML_IO_ERROR); assert(!created);
  assert(ml_options_set(o,ML_CREATE_IF_MISSING,1,&e)==ML_OK);
  ml_db *a=nullptr,*b=nullptr;
  assert(open(root+"-a",o,&a)==ML_OK); assert(open(root+"-b",o,&b)==ML_OK);
  ml_snapshot* s=nullptr; assert(ml_snapshot_create(a,&s,&e)==ML_OK);
  uint8_t* value=nullptr; size_t n=0;
  expect(ml_db_get(b,nullptr,0,nullptr,s,&value,&n,&e),ML_INVALID_ARGUMENT);
  ml_iterator* i=nullptr;
  expect(ml_iterator_create(b,nullptr,s,&i,&e),ML_INVALID_ARGUMENT);
  assert(ml_iterator_create(a,nullptr,s,&i,&e)==ML_OK);
  expect(ml_iterator_key(i,&value,&n,&e),ML_NOT_FOUND);
  expect(ml_iterator_value(i,&value,&n,&e),ML_NOT_FOUND);
  expect(ml_iterator_previous(i,&e),ML_NOT_FOUND);
  ml_snapshot_release(s); ml_db_destroy(a);
  assert(ml_iterator_seek_first(i,&e)==ML_OK); ml_iterator_destroy(i);
  expect(ml_db_get(b,nullptr,0,nullptr,nullptr,nullptr,&n,&e),ML_INVALID_ARGUMENT);
  expect(ml_db_get(b,nullptr,1,nullptr,nullptr,&value,&n,&e),ML_INVALID_ARGUMENT);
  const uint8_t tiny=0;
  expect(ml_db_put(b,&tiny,SIZE_MAX,&tiny,1,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_db_put(b,&tiny,1,&tiny,SIZE_MAX,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_batch_write(b,nullptr,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_snapshot_create(b,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_iterator_create(b,nullptr,nullptr,nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_iterator_status(nullptr,&e),ML_INVALID_ARGUMENT);
  expect(ml_db_close(nullptr,&e),ML_INVALID_ARGUMENT);
  assert(ml_iterator_valid(nullptr)==0); assert(ml_error_code(nullptr)==ML_OK); assert(!*ml_error_message(nullptr));
  ml_options* read=nullptr; assert(ml_options_create(1,&read,&e)==ML_OK);
  expect(open(root+"-wrong",read,&a),ML_INVALID_ARGUMENT);
  expect(create(fresh,read,&a),ML_INVALID_ARGUMENT); assert(!a);
  ml_options_destroy(read);
  assert(ml_options_set(o,2,1,&e)==ML_OK);
  ml_db_destroy(b);
  expect(open(root+"-b",o,&a),ML_INVALID_ARGUMENT);
  expect(open(root+"-missing",nullptr,&a),ML_INVALID_ARGUMENT);
  const std::string corrupt=root+"-corrupt";
  std::filesystem::create_directories(corrupt);
  { std::ofstream current(corrupt+"/CURRENT",std::ios::binary); current<<"invalid-current"; }
  expect(open(corrupt,nullptr,&a),ML_CORRUPTION);
  ml_options_destroy(o);
  return 0;
}
