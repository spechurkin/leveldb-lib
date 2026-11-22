#pragma once

#include "multileveldb.h"
#include <memory>

namespace multileveldb_node {
enum class Kind { Database, Iterator, Snapshot, Batch };
struct Resource {
  Kind kind; void* pointer;
  Resource(Kind k,void* p):kind(k),pointer(p) {}
  void close() noexcept {
    if(!pointer) return;
    switch(kind) {
      case Kind::Database: ml_db_destroy(static_cast<ml_db*>(pointer)); break;
      case Kind::Iterator: ml_iterator_destroy(static_cast<ml_iterator*>(pointer)); break;
      case Kind::Snapshot: ml_snapshot_release(static_cast<ml_snapshot*>(pointer)); break;
      case Kind::Batch: ml_batch_destroy(static_cast<ml_batch*>(pointer)); break;
    }
    pointer=nullptr;
  }
  ~Resource() { close(); }
};
using Shared=std::shared_ptr<Resource>;
}
