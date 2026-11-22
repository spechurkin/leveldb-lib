#pragma once

#include "resource.hpp"
#include <node_api.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace multileveldb_node {
struct Job {
  napi_async_work work=nullptr; napi_deferred deferred=nullptr;
  std::string op,key,value,message;
  Shared a,b,result;
  std::vector<std::pair<uint32_t,uint64_t>> options;
  ml_status status=ML_OK;
  uint8_t* output=nullptr; size_t length=0;
  bool boolean=false,has_buffer=false;
  ~Job() { ml_buffer_free(output); }
  void* resource(const Shared& r,Kind kind) {
    if(!r || r->kind!=kind) throw std::invalid_argument("Wrong resource kind");
    if(!r->pointer) { status=ML_DATABASE_CLOSED; throw std::runtime_error("Resource is closed"); }
    return r->pointer;
  }
};
void execute(napi_env env, void* data) noexcept;
}
