#include <node_api.h>
#include "job.hpp"
#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace multileveldb_node;
void checked(napi_status s) { if(s!=napi_ok) throw std::runtime_error("Node-API call failed"); }
void finalize(napi_env,void* data,void*) { delete static_cast<Shared*>(data); }
const napi_type_tag tag={0x5af78ec2399b10d1ULL,0x881ce02a7cc45412ULL};
Shared handle(napi_env env,napi_value v) {
  napi_valuetype type; checked(napi_typeof(env,v,&type));
  if(type==napi_null || type==napi_undefined) return {};
  bool matches=false; checked(napi_check_object_type_tag(env,v,&tag,&matches));
  if(!matches) throw std::invalid_argument("Invalid native resource");
  void* data; checked(napi_get_value_external(env,v,&data)); return *static_cast<Shared*>(data);
}
std::string bytes(napi_env env,napi_value v) {
  napi_valuetype type; checked(napi_typeof(env,v,&type));
  if(type==napi_null || type==napi_undefined) return {};
  bool typed=false; checked(napi_is_typedarray(env,v,&typed));
  if(!typed) throw std::invalid_argument("Expected Buffer or Uint8Array");
  napi_typedarray_type array_type; size_t len,offset; void* data; napi_value backing;
  checked(napi_get_typedarray_info(env,v,&array_type,&len,&data,&backing,&offset));
  if(array_type!=napi_uint8_array) throw std::invalid_argument("Expected Buffer or Uint8Array");
  return len ? std::string(static_cast<const char*>(data),len) : std::string();
}
std::string text(napi_env env,napi_value v) {
  size_t n=0; checked(napi_get_value_string_utf8(env,v,nullptr,0,&n));
  std::string s(n+1,'\0'); checked(napi_get_value_string_utf8(env,v,&s[0],s.size(),&n)); s.resize(n); return s;
}
void complete(napi_env env,napi_status status,void* data) noexcept {
  std::unique_ptr<Job> j(static_cast<Job*>(data));
  try {
    napi_value value;
    if(status!=napi_ok) { j->status=ML_INTERNAL_ERROR; j->message="Async operation cancelled"; }
    if(j->status && !(j->op=="get" && j->status==ML_NOT_FOUND)) {
      napi_value message,code; checked(napi_create_string_utf8(env,j->message.c_str(),j->message.size(),&message));
      checked(napi_create_error(env,nullptr,message,&value));
      static const char* codes[]={"OK","NOT_FOUND","INVALID_ARGUMENT","IO_ERROR","CORRUPTION","DATABASE_LOCKED","DATABASE_CLOSED","UNSUPPORTED","INTERNAL_ERROR"};
      checked(napi_create_string_utf8(env,codes[j->status>=0 && j->status<=8?j->status:8],NAPI_AUTO_LENGTH,&code));
      checked(napi_set_named_property(env,value,"code",code)); checked(napi_reject_deferred(env,j->deferred,value));
    } else {
      if(j->result) {
        auto box=std::make_unique<Shared>(j->result);
        checked(napi_create_external(env,box.get(),finalize,nullptr,&value)); box.release();
        checked(napi_type_tag_object(env,value,&tag));
      } else if(j->op=="get" && j->status==ML_NOT_FOUND) checked(napi_get_null(env,&value));
      else if(j->has_buffer) checked(napi_create_buffer_copy(env,j->length,j->output,nullptr,&value));
      else if(j->op=="isValid") checked(napi_get_boolean(env,j->boolean,&value));
      else checked(napi_get_undefined(env,&value));
      checked(napi_resolve_deferred(env,j->deferred,value));
    }
  } catch(const std::exception& e) {
    napi_value message,value;
    if(napi_create_string_utf8(env,e.what(),NAPI_AUTO_LENGTH,&message)==napi_ok && napi_create_error(env,nullptr,message,&value)==napi_ok)
      napi_reject_deferred(env,j->deferred,value);
  }
  napi_delete_async_work(env,j->work);
}
napi_value invoke(napi_env env,napi_callback_info info) {
  try {
    size_t argc=6; napi_value args[6]; checked(napi_get_cb_info(env,info,&argc,args,nullptr,nullptr));
    if(argc!=6) throw std::invalid_argument("Expected six arguments");
    auto j=std::make_unique<Job>(); j->op=text(env,args[0]); j->a=handle(env,args[1]); j->b=handle(env,args[2]);
    j->key=bytes(env,args[3]); j->value=bytes(env,args[4]);
    uint32_t len=0; checked(napi_get_array_length(env,args[5],&len));
    if(len%2) throw std::invalid_argument("Malformed options");
    for(uint32_t i=0;i<len;i+=2) {
      napi_value field,value; uint32_t f; double v;
      checked(napi_get_element(env,args[5],i,&field)); checked(napi_get_value_uint32(env,field,&f));
      checked(napi_get_element(env,args[5],i+1,&value)); checked(napi_get_value_double(env,value,&v));
      if(!std::isfinite(v) || v<0 || v>9007199254740991.0 || std::floor(v)!=v) throw std::invalid_argument("Option must be a nonnegative safe integer");
      j->options.emplace_back(f,static_cast<uint64_t>(v));
    }
    napi_value promise,name; checked(napi_create_string_utf8(env,"multileveldb",NAPI_AUTO_LENGTH,&name));
    checked(napi_create_promise(env,&j->deferred,&promise));
    checked(napi_create_async_work(env,nullptr,name,execute,complete,j.get(),&j->work));
    auto queued=napi_queue_async_work(env,j->work);
    if(queued!=napi_ok) { napi_delete_async_work(env,j->work); checked(queued); }
    j.release(); return promise;
  } catch(const std::invalid_argument& e) { napi_throw_type_error(env,"INVALID_ARGUMENT",e.what()); }
  catch(const std::exception& e) { napi_throw_error(env,"INTERNAL_ERROR",e.what()); }
  return nullptr;
}
napi_value init(napi_env env,napi_value exports) {
  if((ml_abi_version()>>16)!=1) { napi_throw_error(env,"UNSUPPORTED","Incompatible native ABI"); return nullptr; }
  napi_value fn;
  if(napi_create_function(env,"invoke",NAPI_AUTO_LENGTH,invoke,nullptr,&fn)!=napi_ok ||
     napi_set_named_property(env,exports,"invoke",fn)!=napi_ok) return nullptr;
  return exports;
}
}
NAPI_MODULE(multileveldb,init)
