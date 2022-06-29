#pragma once

#include <memory>
#include <cassert>
#include "shared_state.hpp"

namespace stdlike {

template <typename T>
class Future {
  template <typename U>
  friend class Promise;

 public:
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  // Movable
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  Future(std::shared_ptr<detail::SharedState<T>> state) : state_(std::move(state)) {
  }

  // One-shot
  // Wait for result (value or exception)
  T Get() {
    return state_->get();
  }

 private:
  Future() {
  }

 private:
  std::shared_ptr<detail::SharedState<T>> state_;
};

}  // namespace stdlike
