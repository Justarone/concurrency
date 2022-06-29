#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdlib>
#include "wait_queue.hpp"

namespace stdlike {

class Mutex {
 public:
  void Lock() {
      std::uint32_t expected = 0;

      // 2 - вполне рабочая реализация, даже проходит тесты, наверное, не очень хороша тем,
      // что при освобождении ВСЕГДА дергается системный вызов, что может быть дороговато, конечно
      // while (atom_.exchange(1)) // 2
          // atom_.FutexWait(1); // 2
      // return; // 2

      if (atom_.compare_exchange_strong(expected, 1))
          return;

      do {
          if (expected == 2 || atom_.compare_exchange_strong(expected = 1, 2))
              atom_.FutexWait(2);
              // queue_.Park(); // 1
              // не сработает, потому что:
              // перед паркингом другой тред может сделать Unlock, тогда он установит 0 и wake нас
              // и только после этого мы встанем в wait (park) и оно будет бесконечное
              // (для этого нужно значение в фьютексе: он дает гарантии, что не будет одновременно
              // пропущенного уведомления и измененного значения, можно почитать в ядре)
      } while (!atom_.compare_exchange_strong(expected = 0, 2));
  }

  void Unlock() {
      // atom_.store(0); // 2
      // atom_.FutexWakeOne(); // 2
      return;

      if (atom_.fetch_sub(1) != 1) {
          atom_.store(0);
          atom_.FutexWakeOne();
          // queue_.WakeOne(); // 1
      }
  }

 private:
  twist::stdlike::atomic<std::uint32_t> atom_{0};
  // util::WaitQueue queue_; // 1
};

}  // namespace stdlike

