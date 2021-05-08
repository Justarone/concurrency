#pragma once

#include <experimental/coroutine>

#include <cstdlib>

namespace gorr {

struct JoinHandle {
  struct Promise {
    auto get_return_object() {
      return JoinHandle{};
    }

    auto initial_suspend() noexcept {
      return std::experimental::suspend_never{};
    }

    auto final_suspend() noexcept {
      return std::experimental::suspend_never{};
    }

    void unhandled_exception() {
      std::terminate();
    }

    void return_void() {
      // Nop
    }
  };
};

}  // namespace gorr

template <typename... Args>
struct std::experimental::coroutine_traits<gorr::JoinHandle, Args...> {
  using promise_type = gorr::JoinHandle::Promise;
};
