#include "multileveldb.h"
#include <leveldb/db.h>
#include <cassert>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

int main(int argc,char** argv) {
  assert(argc==2);
  const std::string path=argv[1];
  leveldb::Options options; options.create_if_missing=true;
  leveldb::DB* engine=nullptr;
  assert(leveldb::DB::Open(options,path,&engine).ok());
  assert(engine->Put(leveldb::WriteOptions(),std::string("a\0b",3),std::string("\0\xff",2)).ok());
  const std::string compressed(1024*1024,'x');
  assert(engine->Put(leveldb::WriteOptions(),"compressed",compressed).ok());
  engine->CompactRange(nullptr,nullptr);
  delete engine;
  ml_db* db=nullptr; ml_error* error=nullptr;
  assert(ml_db_open(reinterpret_cast<const uint8_t*>(path.data()),path.size(),nullptr,&db,&error)==ML_OK);
  uint8_t* value=nullptr; size_t n=0;
  assert(ml_db_get(db,reinterpret_cast<const uint8_t*>("a\0b"),3,nullptr,nullptr,&value,&n,&error)==ML_OK);
  assert(n==2 && value[0]==0 && value[1]==255); ml_buffer_free(value);
  assert(ml_db_get(db,reinterpret_cast<const uint8_t*>("compressed"),10,nullptr,nullptr,&value,&n,&error)==ML_OK);
  assert(n==compressed.size() && std::string(reinterpret_cast<char*>(value),n)==compressed); ml_buffer_free(value);
  std::vector<std::thread> threads;
  for(int t=0;t<4;++t) threads.emplace_back([db,t]{
    for(int j=0;j<200;++j) {
      std::string k=std::to_string(t)+":"+std::to_string(j); ml_error* e=nullptr;
      assert(ml_db_put(db,reinterpret_cast<const uint8_t*>(k.data()),k.size(),nullptr,0,nullptr,&e)==ML_OK);
    }
  });
  for(auto& t:threads) t.join();
  ml_db_destroy(db);
  assert(leveldb::DB::Open(options,path,&engine).ok());
  std::string v; assert(engine->Get(leveldb::ReadOptions(),std::string("a\0b",3),&v).ok());
  assert(v==std::string("\0\xff",2)); delete engine;
  const std::string unicode=path+reinterpret_cast<const char*>(u8"-данные-数据库-😀");
  ml_options* o=nullptr;
  assert(ml_options_create(ML_OPTIONS_OPEN,&o,&error)==ML_OK);
  assert(ml_options_set(o,ML_CREATE_IF_MISSING,1,&error)==ML_OK);
  assert(ml_db_open(reinterpret_cast<const uint8_t*>(unicode.data()),unicode.size(),o,&db,&error)==ML_OK);
  assert(std::filesystem::exists(std::filesystem::u8path(unicode)));
  ml_db_destroy(db); ml_options_destroy(o);
  const uint8_t invalid[]={0xc0,0xaf};
  assert(ml_db_open(invalid,2,nullptr,&db,&error)==ML_INVALID_ARGUMENT); ml_error_free(error);
  return 0;
}
