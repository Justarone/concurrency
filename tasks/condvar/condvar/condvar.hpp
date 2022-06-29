#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdint>

namespace stdlike {

class CondVar {
 public:
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex& m) {
    auto value = state_.load();
    m.unlock();
    state_.wait(value);
    m.lock();
  }

  void NotifyOne() {
      state_.fetch_add(1);
      state_.notify_one();
  }

  void NotifyAll() {
      state_.fetch_add(1);
      state_.notify_all();
  }

 private:
  twist::stdlike::atomic<std::uint32_t> state_{0};
};

}  // namespace stdlike
