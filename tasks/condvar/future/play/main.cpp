#include <exception>
#include <futures/promise.hpp>

#include <iostream>
#include <thread>

int main() {
  stdlike::Promise<std::string> p;
  auto f = p.MakeFuture();

  try {
    throw std::runtime_error("exception");
  } catch (...) {
    p.SetException(std::current_exception());
  }
  // p.SetValue("Hi");
  std::cout << f.Get() << std::endl;

  return 0;
}
