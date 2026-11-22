#include "job.hpp"

namespace multileveldb_node {
struct Options {
  ml_options* p=nullptr;
  ~Options() { ml_options_destroy(p); }
};
void execute(napi_env,void* data) noexcept {
  auto& j=*static_cast<Job*>(data); ml_error* error=nullptr;
  try {
    Options options;
    if(!j.options.empty()) {
      uint32_t kind=(j.op=="open"||j.op=="create")?ML_OPTIONS_OPEN:(j.op=="get"||j.op=="iterator")?ML_OPTIONS_READ:ML_OPTIONS_WRITE;
      j.status=ml_options_create(kind,&options.p,&error);
      if(!j.status) for(auto v:j.options) {
        j.status=ml_options_set(options.p,v.first,v.second,&error); if(j.status) break;
      }
    }
    auto k=reinterpret_cast<const uint8_t*>(j.key.data());
    auto v=reinterpret_cast<const uint8_t*>(j.value.data());
    if(j.status) {} // Option errors keep their original native status and message.
    else if(j.op=="open" || j.op=="create") {
      ml_db* p=nullptr;
      j.status=(j.op=="create"?ml_db_create:ml_db_open)(k,j.key.size(),options.p,&p,&error);
      if(!j.status) { try { j.result=std::make_shared<Resource>(Kind::Database,p); } catch(...) { ml_db_destroy(p); throw; } }
    } else if(j.op=="batch") {
      if(j.a) j.resource(j.a,Kind::Database);
      ml_batch* p=nullptr; j.status=ml_batch_create(&p,&error);
      if(!j.status) { try { j.result=std::make_shared<Resource>(Kind::Batch,p); } catch(...) { ml_batch_destroy(p); throw; } }
    } else if(j.op=="close") { if(j.a) j.a->close(); }
    else if(j.op=="get" || j.op=="put" || j.op=="delete" || j.op=="write" || j.op=="snapshot" || j.op=="iterator") {
      auto db=static_cast<ml_db*>(j.resource(j.a,Kind::Database));
      if(j.op=="get" || j.op=="iterator") {
        auto s=j.b?static_cast<ml_snapshot*>(j.resource(j.b,Kind::Snapshot)):nullptr;
        if(j.op=="get") { j.has_buffer=true; j.status=ml_db_get(db,k,j.key.size(),options.p,s,&j.output,&j.length,&error); }
        else {
          ml_iterator* p=nullptr; j.status=ml_iterator_create(db,options.p,s,&p,&error);
          if(!j.status) { try { j.result=std::make_shared<Resource>(Kind::Iterator,p); } catch(...) { ml_iterator_destroy(p); throw; } }
        }
      } else if(j.op=="put") j.status=ml_db_put(db,k,j.key.size(),v,j.value.size(),options.p,&error);
      else if(j.op=="delete") j.status=ml_db_delete(db,k,j.key.size(),options.p,&error);
      else if(j.op=="write") j.status=ml_batch_write(db,static_cast<ml_batch*>(j.resource(j.b,Kind::Batch)),options.p,&error);
      else {
        ml_snapshot* p=nullptr; j.status=ml_snapshot_create(db,&p,&error);
        if(!j.status) { try { j.result=std::make_shared<Resource>(Kind::Snapshot,p); } catch(...) { ml_snapshot_release(p); throw; } }
      }
    } else if(j.op=="batchPut" || j.op=="batchDelete" || j.op=="batchClear") {
      auto p=static_cast<ml_batch*>(j.resource(j.a,Kind::Batch));
      if(j.op=="batchPut") j.status=ml_batch_put(p,k,j.key.size(),v,j.value.size(),&error);
      else if(j.op=="batchDelete") j.status=ml_batch_delete(p,k,j.key.size(),&error);
      else j.status=ml_batch_clear(p,&error);
    } else {
      auto p=static_cast<ml_iterator*>(j.resource(j.a,Kind::Iterator));
      if(j.op=="seekToFirst") j.status=ml_iterator_seek_first(p,&error);
      else if(j.op=="seekToLast") j.status=ml_iterator_seek_last(p,&error);
      else if(j.op=="seek") j.status=ml_iterator_seek(p,k,j.key.size(),&error);
      else if(j.op=="next") j.status=ml_iterator_next(p,&error);
      else if(j.op=="previous") j.status=ml_iterator_previous(p,&error);
      else if(j.op=="isValid") j.boolean=ml_iterator_valid(p)!=0;
      else if(j.op=="status") j.status=ml_iterator_status(p,&error);
      else if(j.op=="key") { j.has_buffer=true; j.status=ml_iterator_key(p,&j.output,&j.length,&error); }
      else if(j.op=="value") { j.has_buffer=true; j.status=ml_iterator_value(p,&j.output,&j.length,&error); }
      else throw std::invalid_argument("Unknown operation");
    }
    if(error) j.message=ml_error_message(error);
  } catch(const std::invalid_argument& e) { j.status=ML_INVALID_ARGUMENT; try { j.message=e.what(); } catch(...) {} }
  catch(const std::exception& e) { if(!j.status) j.status=ML_INTERNAL_ERROR; try { j.message=e.what(); } catch(...) {} }
  catch(...) { j.status=ML_INTERNAL_ERROR; }
  ml_error_free(error);
}
}
