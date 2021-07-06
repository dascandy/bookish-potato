#pragma once

#include <experimental/coroutine>
#include <memory>
#include <utility>
#include <type_traits>

template <typename T>
struct promise;

template<typename T>
struct future {
    using handle_type = std::experimental::coroutine_handle<promise<T>>;
    using promise_type = promise<T>;
    future() {}
    future(handle_type h)
    : coro(h) {
    }
    future(future && rhs) {
        coro = s2::exchange(rhs.coro, {});
    }
    const future& operator=(future&& rhs) {
        coro = s2::exchange(rhs.coro, {});
        return *this;
    }
    ~future() {
    }
    bool await_ready() {
        return coro.promise().has_value;
    }
    void await_suspend(std::experimental::coroutine_handle<> awaiting) {
        coro.promise().awaiting = awaiting;
    }
    auto await_resume() {
        return get_value();
    }
    auto get_value() {
        if constexpr (s2::is_same_v<T, void>) {
            return;
        } else {
            return s2::move((T&)coro.promise().value_storage);
        }
    }
    handle_type coro;
};

template <typename T>
struct alignas(alignof(T)) promise {
    using handle_type = std::experimental::coroutine_handle<promise<T>>;
    char value_storage[sizeof(T)];
    bool has_value = false;
    std::experimental::coroutine_handle<> awaiting = {};
    promise() 
    {
    }
    ~promise() {
    }
    auto get_return_object() {
        return future<T>{handle_type::from_promise(*this)};
    }
    auto get_future() {
        return get_return_object();
    }
    auto initial_suspend() {
        return std::experimental::suspend_never{};
    }
    auto return_value(T v) {
        new (value_storage) T(s2::move(v));
        if (awaiting) {
          s2::exchange(awaiting, {}).resume();
        } else
          has_value = true;
        return std::experimental::suspend_never{};
    }
    auto final_suspend() noexcept {
        return std::experimental::suspend_never{};
    }
    void unhandled_exception() {
        // TODO
    }
};

template <>
struct promise<void> {
    using handle_type = std::experimental::coroutine_handle<promise<void>>;
    bool has_value = false;
    std::experimental::coroutine_handle<> awaiting = {};
    promise() 
    {
    }
    ~promise() {
    }
    auto get_return_object() {
        return future<void>{handle_type::from_promise(*this)};
    }
    auto initial_suspend() {
        return std::experimental::suspend_never{};
    }
    auto return_void() {
        if (awaiting) {
          s2::exchange(awaiting, {}).resume();
        } else
          has_value = true;
        return std::experimental::suspend_never{};
    }
    auto final_suspend() noexcept {
        return std::experimental::suspend_never{};
    }
    void unhandled_exception() {
        // TODO
    }
};
