#include <wheels/test/test_framework.hpp>

#include <futures/promise.hpp>

#include <string>
#include <thread>
#include <variant>

using stdlike::Promise;
using stdlike::Future;

using namespace std::chrono_literals;

TEST_SUITE(Futures) {
  struct MoveOnly {
    MoveOnly() = default;

    // Non-copyable
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    // Movable
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
  };

  SIMPLE_TEST(MoveOnly) {
    Promise<MoveOnly> p;
    auto f = p.MakeFuture();

    std::thread producer([p = std::move(p)]() mutable {
      p.SetValue(MoveOnly{});
    });

    f.Get();
    producer.join();
  }

  SIMPLE_TEST(NonDefaultConstructable) {
    struct NonDefaultConstructable {
      explicit NonDefaultConstructable(int) {
      }
    };
    Promise<NonDefaultConstructable> p;
  }

  SIMPLE_TEST(MonoState) {
    Promise<std::monostate> p;
    auto f = p.MakeFuture();
    bool ok = false;

    std::thread producer([p = std::move(p), &ok]() mutable {
      std::this_thread::sleep_for(1s);
      ok = true;
      p.SetValue({});
    });

    f.Get();
    ASSERT_TRUE(ok);

    producer.join();
  }
}

RUN_ALL_TESTS()
