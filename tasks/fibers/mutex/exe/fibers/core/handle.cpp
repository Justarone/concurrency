#include <exe/fibers/core/handle.hpp>

#include <exe/fibers/core/fiber.hpp>

#include <wheels/support/assert.hpp>

#include <utility>

namespace exe::fibers {

Fiber* FiberHandle::Release() {
  WHEELS_ASSERT(fiber_ != nullptr, "Invalid fiber handle");
  return std::exchange(fiber_, nullptr);
}

void FiberHandle::Resume() {
  // Not implemented
}

void FiberHandle::ResumeHere() {
  // Not implemented
}

}  // namespace exe::fibers
