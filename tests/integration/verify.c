#include "multileveldb.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static size_t unhex(const char* s,uint8_t* out) {
  if(!strcmp(s,"-")) return 0;
  size_t n=strlen(s)/2;
  for(size_t i=0;i<n;++i) { unsigned v=0; assert(sscanf(s+2*i,"%2x",&v)==1); out[i]=(uint8_t)v; }
  return n;
}
int main(void) {
  const char* path=getenv("ML_INTEROP_PATH"); assert(path);
  ml_db* db=NULL; ml_error* e=NULL;
  assert(ml_db_open((const uint8_t*)path,strlen(path),NULL,&db,&e)==ML_OK);
  FILE* f=fopen(getenv("ML_FIXTURE"),"r"); assert(f); char line[1024];
  while(fgets(line,sizeof line,f)) {
    if(line[0]=='#' || line[0]=='\n') continue;
    char* tab=strchr(line,'\t'); assert(tab); *tab++=0; tab[strcspn(tab,"\r\n")]=0;
    uint8_t key[256],expected[256]; size_t kn=unhex(line,key),vn=unhex(tab,expected);
    expected[vn++]=0; expected[vn++]=254;
    uint8_t* actual=NULL; size_t n=0;
    assert(ml_db_get(db,key,kn,NULL,NULL,&actual,&n,&e)==ML_OK);
    assert(n==vn && !memcmp(actual,expected,n)); ml_buffer_free(actual);
  }
  fclose(f);
  const char* adapters[]={"python","ruby","csharp","fsharp","go"};
  const uint8_t binary[]={0,128,255};
  for(size_t i=0;i<5;++i) {
    if(i==4 && getenv("ML_SKIP_GO") && !strcmp(getenv("ML_SKIP_GO"),"1")) continue;
    uint8_t key[32]; size_t kn=strlen(adapters[i]); memcpy(key,adapters[i],kn);
    key[kn++]=0; key[kn++]=255;
    uint8_t* actual=NULL; size_t n=0;
    assert(ml_db_get(db,key,kn,NULL,NULL,&actual,&n,&e)==ML_OK);
    assert(n==sizeof binary && !memcmp(actual,binary,n)); ml_buffer_free(actual);
  }
  ml_db_destroy(db); return 0;
}
