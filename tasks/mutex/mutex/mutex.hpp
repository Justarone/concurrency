#pragma once

#include <twist/stdlike/atomic.hpp>
#include <twist/util/spin_wait.hpp>

#include <cstdlib>

namespace solutions {

using twist::util::SpinWait;

class Mutex {
 public:
  void Lock() {
    // Your code goes here
  }

  void Unlock() {
    // Your code goes here
  }

 private:
  // ???
};

}  // namespace solutions
