#include <wheels/test/test_framework.hpp>

#include <tinyfibers/test/test.hpp>

#include <tinyfibers/api.hpp>
#include <tinyfibers/sync/nursery.hpp>
#include <tinyfibers/sync/mutex.hpp>
#include <tinyfibers/sync/condvar.hpp>

#include <wheels/support/time.hpp>

#include <memory>
#include <chrono>
#include <type_traits>

using namespace tinyfibers;

using namespace std::chrono_literals;

TEST_SUITE(Core) {
  SIMPLE_TEST(JustWorks) {
    RunScheduler([]() {
      self::Yield();
    });
  }

  // Runs test routine in fiber scheduler
  TINY_FIBERS_TEST(TestMacro) {
    self::Yield();
  }

  TINY_FIBERS_TEST(Join) {
    bool done = false;
    JoinHandle h = Spawn([&]() {
      done = true;
    });
    ASSERT_FALSE(done);
    h.Join();
    ASSERT_TRUE(done);
  }

  TINY_FIBERS_TEST(JoinCompleted) {
    bool done = false;
    JoinHandle h = Spawn([&]() {
      done = true;
    });
    self::Yield();
    ASSERT_TRUE(done);
    h.Join();
  }

  // At least does not panic
  TINY_FIBERS_TEST(Detach) {
    JoinHandle h = Spawn([&]() {
      self::Yield();
    });
    h.Detach();
  }

  TINY_FIBERS_TEST(MoveJoinHandle) {
    JoinHandle h = Spawn([]() {});
    self::Yield();
    JoinHandle g{std::move(h)};
    g.Join();
  }

  TINY_FIBERS_TEST(Ids) {
    FiberId init_id = self::GetId();

    auto first = [&]() {
      ASSERT_EQ(self::GetId(), init_id + 1);
    };

    auto second = [&]() {
      ASSERT_EQ(self::GetId(), init_id + 2);
    };

    JoinHandle f1 = Spawn(first);
    JoinHandle f2 = Spawn(second);

    self::Yield();

    ASSERT_EQ(init_id, self::GetId());

    f1.Join();
    f2.Join();
  }

  TINY_FIBERS_TEST(PingPong) {
    int count = 0;

    auto first = [&count]() {
      for (size_t i = 0; i < 10; ++i) {
        ++count;
        self::Yield();
        ASSERT_EQ(count, 0);
      }
      ++count;
    };

    auto second = [&count]() {
      for (size_t i = 0; i < 10; ++i) {
        --count;
        self::Yield();
        ASSERT_EQ(count, 1);
      }
    };

    Nursery nursery;
    nursery.Spawn(first);
    nursery.Spawn(second);
    nursery.Wait();
  }

  TINY_FIBERS_TEST(SleepFor) {
    wheels::StopWatch stop_watch;
    self::SleepFor(1s);
    ASSERT_GE(stop_watch.Elapsed(), 1s);
  }

  TINY_FIBERS_TEST(FifoScheduling) {
    static const size_t kFibers = 5;
    static const size_t kRounds = 5;

    size_t next = 0;

    auto routine = [&](size_t k) {
      for (size_t i = 0; i < kRounds; ++i) {
        ASSERT_EQ(next, k);
        next = (next + 1) % kFibers;
        self::Yield();
      }
    };

    Nursery nursery;
    for (size_t k = 0; k < kFibers; ++k) {
      nursery.Spawn([&, k]() {
        routine(k);
      });
    }
    nursery.Wait();
  }

  TINY_FIBERS_TEST(Mutex) {
    Mutex mutex;
    bool critical = false;

    auto routine = [&]() {
      for (size_t i = 0; i < 10; ++i) {
        mutex.Lock();
        ASSERT_FALSE(critical);
        critical = true;
        for (size_t j = 0; j < 3; ++j) {
          self::Yield();
        }
        ASSERT_TRUE(critical);
        critical = false;
        mutex.Unlock();
        self::Yield();
      }
    };

    Nursery nursery;
    nursery.Spawn(routine).Spawn(routine);
    nursery.Wait();
  }

  TINY_FIBERS_TEST(MutexTryLock) {
    Mutex mutex;

    Nursery nursery;

    nursery.Spawn([&mutex]() {
      mutex.Lock();
      self::Yield();
      mutex.Unlock();
    });

    nursery.Spawn([&mutex]() {
      ASSERT_FALSE(mutex.TryLock());
      self::Yield();
      ASSERT_TRUE(mutex.TryLock());
      mutex.Unlock();
    });

    nursery.Wait();
  }

  TINY_FIBERS_TEST(ConditionVariable) {
    Mutex mutex;
    CondVar ready;
    std::string message;

    Nursery nursery;

    nursery.Spawn([&]() {
      std::unique_lock lock(mutex);

      ready.Wait(mutex);
      ASSERT_EQ(message, "Hello");
    });

    nursery.Spawn([&]() {
      std::lock_guard guard(mutex);

      for (size_t i = 0; i < 100; ++i) {
        self::Yield();
      }
      message = "Hello";
      ready.NotifyOne();
    });

    nursery.Wait();
  }

  class OnePassBarrier {
   public:
    OnePassBarrier(size_t threads) : threads_(threads) {
    }

    void Arrive() {
      std::unique_lock lock(mutex_);
      --threads_;
      if (threads_ == 0) {
        all_arrived_.NotifyAll();
      } else {
        all_arrived_.Wait(lock, [this]() {
          return threads_ == 0;
        });
      }
    }

   private:
    size_t threads_;
    Mutex mutex_;
    CondVar all_arrived_;
  };

  TINY_FIBERS_TEST(Barrier) {
    static const size_t kFibers = 100;

    OnePassBarrier barrier{kFibers};
    size_t arrived = 0;

    Nursery nursery;
    for (size_t i = 0; i < kFibers; ++i) {
      nursery.Spawn([&]() {
        ++arrived;
        barrier.Arrive();
        ASSERT_EQ(arrived, kFibers);
      });
    }
    nursery.Wait();
  }
}

RUN_ALL_TESTS()