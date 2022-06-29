#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>
#include <deque>

#include <optional>

namespace tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedBlockingQueue {
 public:
  bool Put(T new_element) {
    std::lock_guard<twist::stdlike::mutex> lock(mutex_);
    if (closed_for_puts_)
        return false;
    buffer_.push_back(std::move(new_element));
    has_elements_.notify_one();
    return true;
  }

  std::optional<T> Take() {
    std::unique_lock<twist::stdlike::mutex> lock(mutex_);
    has_elements_.wait(lock, [this]() { return !buffer_.empty() || closed_for_puts_; });
    if (buffer_.empty())
        return std::nullopt;
    auto element = std::move(buffer_.front());
    buffer_.pop_front();
    return std::make_optional(std::move(element));
  }

  void Close() {
    CloseImpl(/*clear=*/false);
  }

  void Cancel() {
    CloseImpl(/*clear=*/true);
  }

 private:
  void CloseImpl(bool clear) {
    std::lock_guard<twist::stdlike::mutex> lock(mutex_);
    closed_for_puts_ = true;
    if (clear)
        buffer_.clear();
    has_elements_.notify_all();
  }

 private:
  std::deque<T> buffer_;
  std::size_t capacity_;

  bool closed_for_puts_{false};
  twist::stdlike::condition_variable has_elements_;
  twist::stdlike::mutex mutex_;
};

}  // namespace tp
