#pragma once

#include <mtf/fibers/sync/channel_impl.hpp>

#include <memory>

namespace mtf::fibers {

// Buffered channel
// https://tour.golang.org/concurrency/3

// Does not support void type
// Use wheels::Unit instead (from <wheels/support/unit.hpp>)

template <typename T>
class Channel {
  using Impl = detail::ChannelImpl<T>;

 public:
  // Bounded channel, `capacity` > 0
  Channel(size_t capacity) : impl_(std::make_shared<Impl>(capacity)) {
  }

  // Unbounded channel
  Channel() : Channel(std::numeric_limits<size_t>::max()) {
  }

  // Blocking
  void Send(T value) {
    impl_->Send(std::move(value));
  }

  // Blocking
  T Receive() {
    return impl_->Receive();
  }

 private:
  std::shared_ptr<Impl> impl_;
};

}  // namespace mtf::fibers