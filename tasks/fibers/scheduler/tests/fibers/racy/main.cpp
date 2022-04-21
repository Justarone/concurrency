#include <exe/fibers/core/api.hpp>
#include <exe/executors/tp/fast/thread_pool.hpp>

#include <wheels/test/test_framework.hpp>

#include <twist/test/test.hpp>

using exe::executors::tp::fast::ThreadPool;
using exe::fibers::Go;
using exe::fibers::self::Yield;

TEST_SUITE(Fibers) {

  class RacyCounter {
   public:
    void Increment() {
      value_.store(value_.load(std::memory_order_relaxed) + 1,
                   std::memory_order_relaxed);
    }
    size_t Get() const {
      return value_.load(std::memory_order_relaxed);
    }

   private:
    std::atomic<size_t> value_{0};
  };

  TEST(RacyCounter, wheels::test::TestOptions().TimeLimit(10s).AdaptTLToSanitizer()) {
    static const size_t kIncrements = 100'000;
    static const size_t kThreads = 4;
    static const size_t kFibers = 100;

    RacyCounter counter;

    auto racer = [&]() {
      for (size_t i = 0; i < kIncrements; ++i) {
        counter.Increment();
        if (i % 10 == 0) {
          Yield();
        }
      }
    };

    ThreadPool scheduler{kThreads};

    Go(scheduler, [&]() {
      for (size_t i = 0; i < kFibers; ++i) {
        Go(racer);
      }
    });

    scheduler.WaitIdle();

    std::cout << "Threads: " << kThreads << std::endl
              << "Fibers: " << kFibers << std::endl
              << "Increments per fiber: " << kIncrements << std::endl
              << "Racy counter value: " << counter.Get() << std::endl;

    ASSERT_GE(counter.Get(), 12345);
    ASSERT_LT(counter.Get(), kIncrements * kFibers);

    scheduler.Stop();
  }
}

RUN_ALL_TESTS()