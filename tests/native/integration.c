#include "multileveldb.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ml_error* error;
static void ok(ml_status s) {
  if(s!=ML_OK) { fprintf(stderr,"%d: %s\n",s,ml_error_message(error)); abort(); }
  assert(error==NULL);
}
#define OK(call) ok(call)
static void expect(ml_db* db,const uint8_t* k,size_t n,const uint8_t* v,size_t z,const ml_snapshot* s) {
  uint8_t* out=NULL; size_t len=0;
  OK(ml_db_get(db,k,n,NULL,s,&out,&len,&error));
  assert(len==z && (!z || memcmp(out,v,z)==0)); ml_buffer_free(out);
}
int main(int argc,char** argv) {
  assert(argc==2); assert(ml_abi_version()==ML_ABI_VERSION);
  assert(strcmp(ml_api_version(),"1.1.0")==0); assert(strcmp(ml_library_version(),"1.0.0")==0); assert(ml_leveldb_version());
  assert(ml_has_feature(ML_FEATURE_UTF8_PATHS)); assert(!ml_has_feature(999));
  ml_options *o=NULL,*r=NULL,*w=NULL;
  OK(ml_options_create(ML_OPTIONS_OPEN,&o,&error));
  OK(ml_options_create(ML_OPTIONS_READ,&r,&error));
  OK(ml_options_create(ML_OPTIONS_WRITE,&w,&error));
  OK(ml_options_set(o,ML_CREATE_IF_MISSING,1,&error));
  OK(ml_options_set(o,ML_ERROR_IF_EXISTS,0,&error));
  OK(ml_options_set(o,ML_PARANOID_CHECKS,1,&error));
  OK(ml_options_set(o,ML_WRITE_BUFFER_SIZE,65536,&error));
  OK(ml_options_set(o,ML_MAX_OPEN_FILES,40,&error));
  OK(ml_options_set(o,ML_BLOCK_SIZE,4096,&error));
  OK(ml_options_set(o,ML_BLOCK_RESTART_INTERVAL,16,&error));
  OK(ml_options_set(o,ML_COMPRESSION,ML_COMPRESSION_NONE,&error));
  OK(ml_options_set(o,ML_CACHE_SIZE,1048576,&error));
  OK(ml_options_set(r,ML_VERIFY_CHECKSUMS,1,&error));
  OK(ml_options_set(r,ML_FILL_CACHE,0,&error));
  OK(ml_options_set(w,ML_SYNC,1,&error));
  ml_db* db=NULL;
  OK(ml_db_open((const uint8_t*)argv[1],strlen(argv[1]),o,&db,&error));
  ml_db* duplicate=NULL;
  assert(ml_db_open((const uint8_t*)argv[1],strlen(argv[1]),o,&duplicate,&error)==ML_DATABASE_LOCKED);
  assert(duplicate==NULL && error); assert(ml_error_code(error)==ML_DATABASE_LOCKED); ml_error_free(error); error=NULL;
  const uint8_t key[]={0,255,1}, value[]={0,128,255,0};
  OK(ml_db_put(db,key,sizeof key,value,sizeof value,w,&error));
  expect(db,key,sizeof key,value,sizeof value,NULL);
  OK(ml_db_put(db,NULL,0,NULL,0,NULL,&error)); expect(db,NULL,0,NULL,0,NULL);
  uint8_t* out=NULL; size_t n=0;
  assert(ml_db_get(db,(const uint8_t*)"missing",7,r,NULL,&out,&n,&error)==ML_NOT_FOUND);
  assert(out==NULL && n==0); ml_error_free(error); error=NULL;
  ml_snapshot* snapshot=NULL; OK(ml_snapshot_create(db,&snapshot,&error));
  OK(ml_db_put(db,key,sizeof key,(const uint8_t*)"new",3,NULL,&error));
  expect(db,key,sizeof key,value,sizeof value,snapshot);
  ml_iterator* it=NULL; OK(ml_iterator_create(db,r,snapshot,&it,&error));
  ml_snapshot_release(snapshot); snapshot=NULL;
  OK(ml_iterator_seek(it,key,sizeof key,&error)); assert(ml_iterator_valid(it));
  OK(ml_iterator_value(it,&out,&n,&error)); assert(n==sizeof value && memcmp(out,value,n)==0); ml_buffer_free(out);
  OK(ml_iterator_key(it,&out,&n,&error)); assert(n==sizeof key && memcmp(out,key,n)==0); ml_buffer_free(out);
  OK(ml_iterator_seek_first(it,&error)); assert(ml_iterator_valid(it));
  OK(ml_iterator_next(it,&error)); OK(ml_iterator_previous(it,&error));
  OK(ml_iterator_seek_last(it,&error)); OK(ml_iterator_next(it,&error)); assert(!ml_iterator_valid(it));
  assert(ml_iterator_next(it,&error)==ML_NOT_FOUND); ml_error_free(error); error=NULL;
  OK(ml_iterator_status(it,&error));
  ml_batch* batch=NULL; OK(ml_batch_create(&batch,&error));
  OK(ml_batch_put(batch,key,sizeof key,value,sizeof value,&error));
  OK(ml_batch_delete(batch,NULL,0,&error)); OK(ml_batch_write(db,batch,w,&error));
  OK(ml_batch_clear(batch,&error)); ml_batch_destroy(batch);
  assert(ml_db_get(db,NULL,0,NULL,NULL,&out,&n,&error)==ML_NOT_FOUND); ml_error_free(error); error=NULL;
  size_t large_size=8*1024*1024; uint8_t* large=(uint8_t*)malloc(large_size); assert(large);
  memset(large,0xa5,large_size); OK(ml_db_put(db,(const uint8_t*)"large",5,large,large_size,NULL,&error));
  expect(db,(const uint8_t*)"large",5,large,large_size,NULL); free(large);
  for(int j=0;j<1000;++j) { OK(ml_db_put(db,key,sizeof key,value,sizeof value,NULL,&error)); expect(db,key,sizeof key,value,sizeof value,NULL); }
  OK(ml_db_delete(db,(const uint8_t*)"large",5,NULL,&error));
  assert(ml_db_put(db,NULL,1,value,sizeof value,NULL,&error)==ML_INVALID_ARGUMENT); ml_error_free(error); error=NULL;
  assert(ml_options_set(o,ML_SYNC,1,&error)==ML_INVALID_ARGUMENT); ml_error_free(error); error=NULL;
  assert(ml_options_set(o,999,1,&error)==ML_UNSUPPORTED); ml_error_free(error); error=NULL;
  assert(ml_options_set(o,ML_CREATE_IF_MISSING,2,&error)==ML_INVALID_ARGUMENT); ml_error_free(error); error=NULL;
  OK(ml_db_close(db,&error)); OK(ml_db_close(db,&error));
  assert(ml_db_get(db,key,sizeof key,NULL,NULL,&out,&n,&error)==ML_DATABASE_CLOSED); ml_error_free(error); error=NULL;
  ml_db_destroy(db);
  OK(ml_iterator_seek_first(it,&error)); ml_iterator_destroy(it);
  OK(ml_db_open((const uint8_t*)argv[1],strlen(argv[1]),o,&db,&error));
  expect(db,key,sizeof key,value,sizeof value,NULL); ml_db_destroy(db);
  ml_options_destroy(o); ml_options_destroy(r); ml_options_destroy(w);
  ml_buffer_free(NULL); ml_error_free(NULL); ml_iterator_destroy(NULL); ml_batch_destroy(NULL); ml_snapshot_release(NULL); ml_db_destroy(NULL);
  return 0;
}
