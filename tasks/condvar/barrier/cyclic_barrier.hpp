#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

// std::lock_guard, std::unique_lock
#include <mutex>
#include <cstdint>

namespace solutions {

// CyclicBarrier allows a set of threads to all wait for each other
// to reach a common barrier point

// The barrier is called cyclic because
// it can be re-used after the waiting threads are released.

class CyclicBarrier {
 public:
  explicit CyclicBarrier(size_t total) {
      total_ = total;
  }

  // Blocks until all participants have invoked Arrive()
  void Arrive() {
      std::unique_lock<twist::stdlike::mutex> lock(mutex_);
      new_come_.wait(lock, [&](){ return !all_come_predicate_; });

      ++arrived_;
      if (arrived_ == total_) {
          all_come_predicate_ = true;
          all_come_.notify_all();
      }
      else
          all_come_.wait(lock, [&]() { return all_come_predicate_; });

      --arrived_;
      if (arrived_ == 0) {
          all_come_predicate_ = false;
          new_come_.notify_all();
      }
  }

 private:
  std::uint32_t arrived_{0};
  bool all_come_predicate_{false};
  std::uint32_t total_;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable all_come_;
  twist::stdlike::condition_variable new_come_;

};

}  // namespace solutions
