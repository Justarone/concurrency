#include <tp/static_thread_pool.hpp>

#include <tp/helpers.hpp>

#include <twist/util/thread_local.hpp>

namespace tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<StaticThreadPool> pool;

////////////////////////////////////////////////////////////////////////////////

StaticThreadPool::StaticThreadPool(size_t /*workers*/) {
  // Not implemented
}

StaticThreadPool::~StaticThreadPool() {
  // Not implemented
}

void StaticThreadPool::Submit(Task /*task*/) {
  // Not implemented
}

void StaticThreadPool::Join() {
  // Not implemented
}

void StaticThreadPool::Shutdown() {
  // Not implemented
}

StaticThreadPool* StaticThreadPool::Current() {
  return pool;
}

}  // namespace tp
