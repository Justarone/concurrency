#pragma once

#include <mutex>
#include <twist/stdlike/mutex.hpp>

namespace util {

//////////////////////////////////////////////////////////////////////

// Safe API for mutual exclusion

template <typename T>
class Mutexed {
  using MutexImpl = twist::stdlike::mutex;

  class UniqueRef {
    // Non-copyable
    UniqueRef(const UniqueRef&) = delete;

    // Non-movable
    UniqueRef(UniqueRef&&) = delete;

  private:
    T& data_;
    std::unique_lock<MutexImpl> guard_;

  public:
    UniqueRef(T& data, std::unique_lock<MutexImpl>&& guard) : data_(data), guard_(std::move(guard))
    {}
    T& operator*() { return data_; }
    T* operator->() { return &data_; }
  };

 public:
  // https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
  template <typename... Args>
  explicit Mutexed(Args&&... args) : object_(std::forward<Args>(args)...) {
  }

  UniqueRef Lock() {
    std::unique_lock guard(mutex_);
    return UniqueRef(object_, std::move(guard));
  }

 private:
  T object_;
  MutexImpl mutex_;  // Guards access to object_
};

//////////////////////////////////////////////////////////////////////

// Helper function for single operations over shared object:
// Usage:
//   Mutexed<vector<int>> ints;
//   Locked(ints)->push_back(42);

template <typename T>
auto Locked(Mutexed<T>& object) {
  return object.Lock();
}

}  // namespace util
