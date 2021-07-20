#pragma once

#include <experimental/coroutine>
#include <mutex>
#include <memory>
#include <variant>
#include <vector>
#include <cassert>

namespace s2 {

struct Empty {};

template <typename T>
struct shared_state {
public:
  shared_state& operator=(const shared_state&) = delete;
  void set_value(T value) {
    s2::unique_lock<s2::mutex> l(m);
    if (is_ready()) {
      assert(false);
    }
    v = s2::move(value);
    cv.notify_all();
    for (auto& aw : awaitings) {
      aw.resume();
    }
    awaitings.clear();
  }
  T& get() {
    s2::unique_lock<s2::mutex> l(m);
    cv.wait(l, [&]{ return is_ready(); });
    switch(v.index()) {
      case 1:
        return s2::get<T>(v);
      default:
        assert(false);
    }
  }
  bool is_empty() {
    return v.index() == 0;
  }
  bool is_ready() {
    return v.index() > 0;
  }
  void add_awaiting(std::experimental::coroutine_handle<> awaiting) {
    awaitings.push_back(s2::move(awaiting));
  }
private:
  s2::vector<std::experimental::coroutine_handle<>> awaitings;
  s2::mutex m;
  s2::condition_variable cv;
  s2::variant<Empty, T> v;
};

template <typename T>
struct future;

template <typename T>
struct promise {
  s2::shared_ptr<shared_state<T>> state;
  promise() {
    state = s2::make_shared<shared_state<T>>();
  }
  ~promise() {
    if (state && state->is_empty()) 
      assert(false);
  }
  promise(promise&& rhs) 
  : state(rhs.state)
  {
    rhs.state.reset();
  }
  promise& operator=(promise&& rhs) {
    state = rhs.state;
    rhs.state.reset();
    return *this;
  }
  auto get_return_object() noexcept {
    return future<T>{state};
  }
  auto initial_suspend() noexcept {
    return std::experimental::suspend_never{};
  }
  void set_value(T v) {
    state->set_value(s2::move(v));
  }
  auto get_future() noexcept {
    return future<T>{state};
  }
  auto return_value(T v) {
    state->set_value(s2::move(v));
    return std::experimental::suspend_never{};
  }
  auto final_suspend() noexcept {
    return std::experimental::suspend_never{};
  }
  void unhandled_exception() {
    assert(false);
  }
};

template <typename T>
struct shared_future {
  s2::shared_ptr<shared_state<T>> state;
  auto get_value() {
    return state->get();
  }
  bool valid() {
    return state->is_ready();
  }
  bool await_ready() noexcept {
    return state->is_ready();
  }
  void await_suspend(std::experimental::coroutine_handle<> awaiting) {
    state->add_awaiting(s2::move(awaiting));
  }
  auto await_resume() {
    return state->get();
  }
};

template<typename T>
struct future {
  using promise_type = promise<T>;
  s2::shared_ptr<shared_state<T>> state;
  bool await_ready() noexcept {
    return state->is_ready();
  }
  void await_suspend(std::experimental::coroutine_handle<> awaiting) {
    state->add_awaiting(s2::move(awaiting));
  }
  auto await_resume() {
    return s2::move(state->get());
  }
  auto get_value() {
    return s2::move(state->get());
  }
  shared_future<T> share() { 
    return shared_future<T>(state); 
  }
};

template <typename T>
future<T> make_ready_future(T t) {
  s2::shared_ptr<shared_state<T>> state = s2::make_shared<shared_state<T>>();
  state->set_value(s2::move(t));
  return future<T>{s2::move(state)};
}

template <>
struct shared_state<void> {
public:
  struct EmptyValue {};
  shared_state& operator=(const shared_state&) = delete;
  void set_value() {
    s2::unique_lock<s2::mutex> l(m);
    if (is_ready()) {
      assert(false);
    }
    v = EmptyValue();
    cv.notify_all();
  }
  void get() {
    s2::unique_lock<s2::mutex> l(m);
    cv.wait(l, [&]{ return is_ready(); });
    switch(v.index()) {
      case 1:
        return;
      default:
        assert(false);
    }
  }
  bool is_empty() {
    return v.index() == 0;
  }
  bool is_ready() {
    return not is_empty();
  }
  void add_awaiting(std::experimental::coroutine_handle<> awaiting) {
    awaitings.push_back(s2::move(awaiting));
  }
private:
  s2::vector<std::experimental::coroutine_handle<>> awaitings;
  s2::mutex m;
  s2::condition_variable cv;
  s2::variant<Empty, EmptyValue> v;
};

template <>
struct shared_future<void> {
  s2::shared_ptr<shared_state<void>> state;
  void get_value() {
    return state->get();
  }
  bool await_ready() noexcept {
    return state->is_ready();
  }
  void await_suspend(std::experimental::coroutine_handle<> awaiting) {
    state->add_awaiting(s2::move(awaiting));
  }
  void await_resume() {
    state->get();
  }
  bool valid() {
    return state->is_ready();
  }
};

template<>
struct future<void> {
  using promise_type = promise<void>;
  s2::shared_ptr<shared_state<void>> state;
  bool await_ready() noexcept {
    return state->is_ready();
  }
  void await_suspend(std::experimental::coroutine_handle<> awaiting) {
    state->add_awaiting(s2::move(awaiting));
  }
  void await_resume() {
    state->get();
  }
  void get_value() {
    state->get();
  }
  shared_future<void> share() { 
    return shared_future<void>{state};
  }
};

template <>
struct promise<void> {
  s2::shared_ptr<shared_state<void>> state;
  promise() {
    state = s2::make_shared<shared_state<void>>();
  }
  ~promise() {
    if (state && state->is_empty()) 
      assert(false);
  }
  auto get_return_object() noexcept {
    return future<void>{state};
  }
  auto initial_suspend() noexcept {
    return std::experimental::suspend_never{};
  }
  void set_value() {
    state->set_value();
  }
  auto get_future() noexcept {
    return future<void>{state};
  }
  auto return_void() {
    state->set_value();
    return std::experimental::suspend_never{};
  }
  auto final_suspend() noexcept {
    return std::experimental::suspend_never{};
  }
  void unhandled_exception() {
    assert(false);
  }
};

}


