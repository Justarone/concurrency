#include <gorr/runtime/thread_pool.hpp>
#include <gorr/runtime/yield.hpp>
#include <gorr/runtime/mutex.hpp>
#include <gorr/runtime/join_handle.hpp>

#include <wheels/test/test_framework.hpp>

#include <wheels/support/cpu_time.hpp>

TEST_SUITE(Mutex) {
  SIMPLE_TEST(JustWorks) {
    gorr::StaticThreadPool scheduler{/*threads=*/4};
    gorr::Mutex mutex;

    auto gorroutine = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();

      {
        auto guard = co_await mutex.Lock();
        // Critical section
      }

      {
        auto guard = co_await mutex.Lock();
        // Critical section
      }

      co_return;
    };

    gorroutine();  // Spawn

    scheduler.Join();
  }

  SIMPLE_TEST(DoNotBlockPoolThread) {
    gorr::StaticThreadPool scheduler{/*threads=*/2};

    gorr::Mutex mutex;

    size_t cs_count = 0;

    auto bubble = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();
      {
        auto guard = co_await mutex.Lock();
        std::this_thread::sleep_for(1s);
        ++cs_count;
      }
      co_return;
    };

    bubble();  // Spawn

    auto locker = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();
      {
        auto guard = co_await mutex.Lock();
        ++cs_count;
      }
      co_return;
    };

    for (size_t i = 0; i < 3; ++i) {
      locker();  // Spawn
    }

    std::this_thread::sleep_for(100ms);

    std::atomic<bool> free{false};

    auto runner = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();
      free.store(true);
    };

    runner();  // Spawn

    std::this_thread::sleep_for(100ms);

    ASSERT_TRUE(free.load());

    scheduler.Join();

    ASSERT_EQ(cs_count, 4);
  }

#if !__has_feature(address_sanitizer) && !__has_feature(thread_sanitizer)

  SIMPLE_TEST(BlockGorroutine) {
    wheels::ProcessCPUTimer cpu_timer;

    gorr::StaticThreadPool scheduler{/*threads=*/4};

    gorr::Mutex mutex;

    auto sleeper = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();
      {
        auto guard = co_await mutex.Lock();
        std::this_thread::sleep_for(1s);
      }
    };

    sleeper();  // Spawn

    std::this_thread::sleep_for(100ms);

    size_t cs_count{0};

    auto locker = [&]() -> gorr::JoinHandle {
      co_await scheduler.Schedule();

      {
        auto guard = co_await mutex.Lock();
        ++cs_count;
      }
    };

    for (size_t i = 0; i < 10; ++i) {
      locker();  // Spawn
    }

    scheduler.Join();

    // Do not waste cpu time in waiting lockers
    ASSERT_TRUE(cpu_timer.Elapsed() < 100ms);
    ASSERT_EQ(cs_count, 10);
  }

#endif

};
