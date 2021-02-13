#include "../mutex.hpp"

#include <twist/test/test.hpp>

#include <twist/test/util/cpu_timer.hpp>

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

using solutions::Mutex;

using twist::strand::stdlike::thread;
using twist::strand::stdlike::this_thread::sleep_for;

TEST_SUITE(UnitTest) {
  SIMPLE_TWIST_TEST(LockUnlock) {
    Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
  }

SIMPLE_TWIST_TEST(SequentialLockUnlock) {
    Mutex mutex;
    mutex.Lock();
    mutex.Unlock();
    mutex.Lock();
    mutex.Unlock();
  }

  SIMPLE_TWIST_TEST(NoSharedLocations) {
    Mutex mutex;
    mutex.Lock();

    Mutex mutex2;
    mutex2.Lock();
    mutex2.Unlock();

    mutex.Unlock();
  }

#if !defined(TWIST_FIBER) && !defined(TWIST_FAULTY)

  SIMPLE_TWIST_TEST(Blocking) {
    Mutex mutex;

    thread sleeper([&]() {
      mutex.Lock();
      sleep_for(3s);
      mutex.Unlock();
    });

    thread waiter([&]() {
      sleep_for(1s);

      twist::test::util::CPUTimer cpu_timer;

      mutex.Lock();
      mutex.Unlock();

      auto cpu_time = cpu_timer.RunningTime();

      std::cout << "Lock/Unlock cpu time in sleeper thread: " << cpu_time << " seconds\n";
      ASSERT_TRUE(cpu_time < 0.2);
    });

    sleeper.join();
    waiter.join();
  }

#endif
}

RUN_ALL_TESTS()
