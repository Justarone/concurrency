#include <exe/executors/thread_pool.hpp>

#include <exe/futures/core/future.hpp>
#include <exe/futures/util/execute.hpp>

using namespace exe;

int main() {
  executors::ThreadPool pool{4};

  auto f = futures::Execute(pool, []() {
    return 42;
  });

  std::move(f).Via(pool).Subscribe([](wheels::Result<int> result) {
    std::cout << "Result = " << result.ValueOrThrow();
  });

  pool.WaitIdle();
  pool.Stop();

  return 0;
}
