#include <multileveldb.h>
#include <stdio.h>
#include <string.h>
int main(void) {
  ml_options* options=NULL; ml_db* db=NULL; ml_error* error=NULL;
  uint8_t* value=NULL; size_t length=0; int result=1;
  const uint8_t key[]={0,255}, data[]={0,128,255};
  if(ml_options_create(ML_OPTIONS_OPEN,&options,&error)) goto cleanup;
  if(ml_options_set(options,ML_CREATE_IF_MISSING,1,&error)) goto cleanup;
  if(ml_db_open((const uint8_t*)"example-db",10,options,&db,&error)) goto cleanup;
  if(ml_db_put(db,key,sizeof key,data,sizeof data,NULL,&error)) goto cleanup;
  if(ml_db_get(db,key,sizeof key,NULL,NULL,&value,&length,&error)) goto cleanup;
  printf("Read %zu bytes\n",length); result=0;
cleanup:
  if(error) fprintf(stderr,"%s\n",ml_error_message(error));
  ml_buffer_free(value); ml_error_free(error); ml_db_destroy(db); ml_options_destroy(options);
  return result;
}
