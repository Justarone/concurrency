#pragma once

#include <exception>
#include <stdexcept>
#include <variant>
#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex> // unique_lock

#include <iostream>
#include "twist/stdlike/condition_variable.hpp"
#include "twist/stdlike/mutex.hpp"

namespace stdlike::detail {
struct EmptyState {};

template <typename T>
class SharedState {
public:
    SharedState() = default;
    T get() {
        std::unique_lock<twist::stdlike::mutex> lock(value_access_);
        has_value_.wait(lock, [this]() { return value_.index() != 0; });
        if (value_.index() == 2)
            return std::move(std::get<2>(value_));
        std::rethrow_exception(std::move(std::get<1>(value_)));
    }

    void set_value(T value) {
        std::lock_guard<twist::stdlike::mutex> lock(value_access_);
        value_ = std::variant<EmptyState, std::exception_ptr, T>(std::in_place_index_t<2>(), std::move(value));
        has_value_.notify_all();
    }

    void set_exception(std::exception_ptr exc) {
        std::lock_guard<twist::stdlike::mutex> lock(value_access_);
        value_ = std::variant<EmptyState, std::exception_ptr, T>(std::in_place_index_t<1>(), std::move(exc));
        has_value_.notify_all();
    }

private:
    std::variant<EmptyState, std::exception_ptr, T> value_{EmptyState{}};
    twist::stdlike::mutex value_access_;
    twist::stdlike::condition_variable has_value_;
};

}  // namespace stdlike
