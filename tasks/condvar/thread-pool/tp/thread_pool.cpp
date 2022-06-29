#include <mutex>
#include <tp/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<ThreadPool> pool;

////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t workers) {
  for (size_t i = 0; i < workers; i++)
    threads_.push_back(twist::stdlike::thread([this]() {
      return Worker();
    }));
}

void ThreadPool::Worker() {
  pool = this;
  while (std::optional<Task> task = queue_.Take()) {
    try {
      (*task)();
    } catch (...) {
    }
    std::lock_guard<twist::stdlike::mutex> lock(mutex_);
    ++processed_;
    wg_done_.notify_all();
  }
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::Submit(Task task) {
  queue_.Put(std::move(task));
  ++to_be_processed_;
}

void ThreadPool::WaitIdle() {
  std::unique_lock<twist::stdlike::mutex> lock(mutex_);
  wg_done_.wait(lock, [&]() {
    return processed_ >= to_be_processed_.load();
  });
  auto value = to_be_processed_.load();
  to_be_processed_ -= value;
  processed_ -= value;
}

void ThreadPool::Stop() {
  queue_.Cancel();
  for (auto& t : threads_) t.join();
  threads_.clear();
}

ThreadPool* ThreadPool::Current() {
  return pool;
}

}  // namespace tp
