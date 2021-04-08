#include <await/executors/static_thread_pool.hpp>
#include <await/executors/label_thread.hpp>

#include <wheels/test/test_framework.hpp>

#include "../helpers.hpp"

#include <thread>
#include <chrono>

using namespace await::executors;
using namespace std::chrono_literals;

TEST_SUITE(ThreadPool) {
  SIMPLE_TEST(ExecuteTask) {
    bool done = false;

    auto tp = MakeStaticThreadPool(4, "test");

    auto task = [&done]() {
      std::cout << "Hello from thread pool!" << std::endl;
      ExpectThread("test");
      done = true;
    };
    tp->Execute(task);
    tp->Join();

    ASSERT_TRUE(done);
    ASSERT_EQ(tp->ExecutedTaskCount(), 1);
  }

  SIMPLE_TEST(ExecuteManyTasks) {
    static const size_t kTasks = 1024;

    auto tp = MakeStaticThreadPool(3, "increments");

    std::atomic<size_t> completed{0};
    for (size_t i = 0; i < kTasks; ++i) {
      tp->Execute([&completed]() {
        ExpectThread("increments");
        completed.fetch_add(1);
      });
    }

    tp->Join();

    ASSERT_EQ(completed, kTasks);
    ASSERT_EQ(tp->ExecutedTaskCount(), kTasks);
  }

  SIMPLE_TEST(ExecutedTaskCount) {
    auto tp = MakeStaticThreadPool(3, "test");

    tp->Execute([]() { /* do nothing */ });
    tp->Execute([]() {
      std::this_thread::sleep_for(1s);
    });

    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(tp->ExecutedTaskCount(), 1);

    tp->Join();
    ASSERT_EQ(tp->ExecutedTaskCount(), 2);
  }

  SIMPLE_TEST(AfterJoin) {
    auto tp = MakeStaticThreadPool(1, "single");
    tp->Join();

    tp->Execute([]() {
      FAIL_TEST("Executed after Join");
    });

    std::this_thread::sleep_for(500ms);
  }

  SIMPLE_TEST(Shutdown) {
    auto tp = MakeStaticThreadPool(1, "test");

    tp->Execute([]() {
      // bubble
      std::this_thread::sleep_for(1s);
    });

    std::this_thread::sleep_for(100ms);

    bool done = false;
    tp->Execute([&done]() {
      done = true;
    });
    tp->Shutdown();

    ASSERT_FALSE(done);

    tp->Execute([&done]() {
      done = true;
    });
    std::this_thread::sleep_for(1s);

    ASSERT_FALSE(done);
  }

  SIMPLE_TEST(JoinThreads1) {
    // Use `Shutdown`
    auto tp = MakeStaticThreadPool(4, "test");
  }

  SIMPLE_TEST(JoinThreads2) {
    std::weak_ptr<IThreadPool> weak;
    {
      auto tp = MakeStaticThreadPool(4, "test");
      weak = tp;
    }
    ASSERT_TRUE(weak.expired())
  }

  SIMPLE_TEST(BlockingQueue) {
    auto tp = MakeStaticThreadPool(3, "test");

    // Warmup
    tp->Execute([]() {});

    {
      test_helpers::CPUTimeBudgetGuard budget(100ms);

      std::this_thread::sleep_for(1s);
      tp->Join();
    }
  }

  SIMPLE_TEST(BlockingJoin) {
    auto tp = MakeStaticThreadPool(2, "test");

    tp->Execute([]() {
      std::this_thread::sleep_for(1s);
    });

    {
      test_helpers::CPUTimeBudgetGuard budget(100ms);
      tp->Join();
    }
  }

  SIMPLE_TEST(Fifo) {
    auto tp = MakeStaticThreadPool(1, "thread");

    size_t next_task = 0;

    static const size_t kTasks = 256;
    for (size_t t = 0; t < kTasks; ++t) {
      tp->Execute([t, &next_task]() {
        ASSERT_EQ(next_task, t);
        ++next_task;
      });
    }

    tp->Join();
  }

#if !defined(TWIST_FAULTY)

  SIMPLE_TEST(RacyCounter) {
    auto tp = MakeStaticThreadPool(13, "pool");

    std::atomic<size_t> counter = 0;

    static const size_t kIncrements = 123456;
    for (size_t i = 0; i < kIncrements; ++i) {
      tp->Execute([&counter]() {
        auto old = counter.load();
        std::this_thread::yield();
        counter.store(old + 1);
      });
    };
    tp->Join();

    std::cout << "Racy counter value after " << kIncrements
              << " increments: " << counter.load() << std::endl;

    ASSERT_NE(counter.load(), kIncrements);
    ASSERT_EQ(tp->ExecutedTaskCount(), kIncrements);
  }

#endif

  SIMPLE_TEST(TwoThreadPools) {
    auto tp1 = MakeStaticThreadPool(3, "tp1");
    auto tp2 = MakeStaticThreadPool(3, "tp2");

    tp1->Execute([]() {
      ExpectThread("tp1");
    });

    tp2->Execute([]() {
      ExpectThread("tp2");
    });

    tp2->Join();
    tp1->Join();
  }

  SIMPLE_TEST(UseAllThreads) {
    auto tp = MakeStaticThreadPool(3, "sleepers");

    std::atomic<size_t> done = 0;

    auto sleeper = [&done]() {
      std::this_thread::sleep_for(1s);
      ExpectThread("sleepers");
      done.fetch_add(1);
    };

    wheels::StopWatch stop_watch;

    tp->Execute(sleeper);
    tp->Execute(sleeper);
    tp->Execute(sleeper);

    tp->Join();

    auto elapsed = stop_watch.Elapsed();

    ASSERT_EQ(done, 3);
    ASSERT_GE(elapsed, 1s);
    ASSERT_LT(elapsed, 1500ms);
  }

  void CountDown(size_t & count, IExecutorPtr e) {
    e->Execute([&count, e]() mutable {
      std::this_thread::sleep_for(100ms);
      count--;
      if (count > 0) {
        CountDown(count, e);
      }
    });
  }

  SIMPLE_TEST(CountDown) {
    auto tp = MakeStaticThreadPool(3, "test");
    size_t count = 10;
    CountDown(count, tp);
    tp->Join();
    ASSERT_EQ(count, 0);
  }

  SIMPLE_TEST(ConcurrentExecutes) {
    auto tp = MakeStaticThreadPool(2, "test");

    static const size_t kProducers = 5;
    static const size_t kTasks = 1024;

    test_helpers::OnePassBarrier barrier{kProducers};
    std::atomic<int> done{0};

    auto task = [&done]() {
      ExpectThread("test");
      done.fetch_add(1);
    };

    std::vector<std::thread> producers;

    for (size_t i = 0; i < kProducers; ++i) {
      producers.emplace_back([tp, &task, &barrier]() {
        barrier.Pass();
        for (size_t j = 0; j < kTasks; ++j) {
          tp->Execute(task);
        }
      });
    }

    for (auto& t : producers) {
      t.join();
    }

    tp->Join();

    ASSERT_EQ(tp->ExecutedTaskCount(), kProducers * kTasks);
    ASSERT_EQ(done.load(), kProducers * kTasks);
  }
}
