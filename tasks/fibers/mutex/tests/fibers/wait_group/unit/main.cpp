#include <wheels/test/test_framework.hpp>

#include <exe/tp/thread_pool.hpp>
#include <exe/fibers/sync/wait_group.hpp>

#include <wheels/support/cpu_time.hpp>

#include <atomic>
#include <thread>

using namespace exe::tp;
using namespace exe::fibers;

TEST_SUITE(WaitGroup) {
  SIMPLE_TEST(OneWaiter) {
    ThreadPool scheduler{/*threads=*/4};

    WaitGroup wg;
    std::atomic<size_t> workers{0};
    std::atomic<bool> waiter{false};

    static const size_t kWorkers = 5;

    wg.Add(kWorkers);

    Go(scheduler, [&]() {
      wg.Wait();
      waiter = true;
    });

    for (size_t i = 0; i < kWorkers; ++i) {
      Go(scheduler, [&]() {
        wg.Done();
        ++workers;
      });
    }

    scheduler.WaitIdle();

    ASSERT_EQ(workers, kWorkers);
    ASSERT_TRUE(waiter);

    scheduler.Stop();
  }

  SIMPLE_TEST(MultipleWaiters) {
    ThreadPool scheduler{/*threads=*/4};

    WaitGroup wg;

    std::atomic<size_t> workers{0};
    std::atomic<size_t> waiters{0};

    static const size_t kWorkers = 5;
    static const size_t kWaiters = 5;

    wg.Add(kWorkers);

    for (size_t i = 0; i < kWaiters; ++i) {
      Go(scheduler, [&]() {
        wg.Wait();
        ++waiters;
      });
    }

    for (size_t i = 0; i < kWorkers; ++i) {
      Go(scheduler, [&]() {
        wg.Done();
        ++workers;
      });
    }

    scheduler.WaitIdle();

    ASSERT_EQ(workers, kWorkers);
    ASSERT_EQ(waiters, kWaiters);

    scheduler.Stop();
  }

  SIMPLE_TEST(BlockingWait) {
    ThreadPool scheduler{/*threads=*/4};

    WaitGroup wg;
    std::atomic<size_t> workers = 0;

    wheels::ProcessCPUTimer timer;

    wg.Add(1);

    Go(scheduler, [&wg]() {
      wg.Wait();
    });

    for (size_t i = 0; i < 3; ++i) {
      Go(scheduler, [&]() {
        std::this_thread::sleep_for(1s);
        wg.Done();
        ++workers;
      });
    }

    scheduler.WaitIdle();

    ASSERT_EQ(workers, 3);
    ASSERT_TRUE(timer.Elapsed() < 100ms);

    scheduler.Stop();
  }
}

RUN_ALL_TESTS()