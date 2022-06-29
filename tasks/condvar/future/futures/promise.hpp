#pragma once

#include <futures/future.hpp>

#include <memory>

namespace stdlike {

template <typename T>
class Promise {
 public:
  Promise() : state_(std::make_shared<detail::SharedState<T>>()) {
  }

  // Non-copyable
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  // Movable
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // One-shot
  Future<T> MakeFuture() {
      return Future<T>(state_);
  }

  // One-shot
  // Fulfill promise with value
  void SetValue(T value) {
      state_->set_value(std::move(value));
  }

  // One-shot
  // Fulfill promise with exception
  void SetException(std::exception_ptr exc) {
      state_->set_exception(exc);
  }

 private:
  std::shared_ptr<detail::SharedState<T>> state_;
};

}  // namespace stdlike
