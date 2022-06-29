#pragma once

#include "tagged_semaphore.hpp"

#include <deque>

namespace solutions {

struct ElementTag {};
struct AccessTag {};

// Bounded Blocking Multi-Producer/Multi-Consumer (MPMC) Queue

template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t capacity) : Empty(capacity) {
  }

  // Inserts the specified element into this queue,
  // waiting if necessary for space to become available.
  void Put(T value) {
    auto elementToken = Empty.Acquire();
    auto bufferAccessToken = BufferAccess.Acquire();
    buffer.push_back(std::move(value));
    BufferAccess.Release(std::move(bufferAccessToken));
    Full.Release(std::move(elementToken));
  }

  // Retrieves and removes the head of this queue,
  // waiting if necessary until an element becomes available
  T Take() {
    auto elementToken = Full.Acquire();
    auto bufferAccessToken = BufferAccess.Acquire();
    auto element = std::move(buffer.front());
    buffer.pop_front();
    BufferAccess.Release(std::move(bufferAccessToken));
    Empty.Release(std::move(elementToken));
    return element;
  }

 private:
  std::deque<T> buffer;
  TaggedSemaphore<ElementTag> Empty;
  TaggedSemaphore<ElementTag> Full{0};
  TaggedSemaphore<AccessTag> BufferAccess{1};
};

}  // namespace solutions
