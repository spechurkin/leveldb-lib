// Direct C ABI use from C++; no separate C++ binding is required.
#include <multileveldb.h>
#include <cstdio>
int main() {
  ml_options* options=nullptr; ml_db* db=nullptr; ml_error* error=nullptr;
  if(ml_options_create(ML_OPTIONS_OPEN,&options,&error)!=ML_OK) return 1;
  ml_status status=ml_options_set(options,ML_CREATE_IF_MISSING,1,&error);
  if(status==ML_OK) status=ml_db_open(reinterpret_cast<const uint8_t*>("example-db"),10,options,&db,&error);
  const uint8_t key[]={0,255}, value[]={0,128,255};
  if(status==ML_OK) status=ml_db_put(db,key,sizeof key,value,sizeof value,nullptr,&error);
  if(error) std::fprintf(stderr,"%s\n",ml_error_message(error));
  ml_error_free(error); ml_db_destroy(db); ml_options_destroy(options);
  return status==ML_OK?0:1;
}
