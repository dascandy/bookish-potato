#pragma once

#include <experimental/coroutine>
#include <utility>

struct event {
    std::experimental::coroutine_handle<> awaiting = {};
    event(event*& s) {
        s = this;
    }
    event() {
    }
    event(event&& o) = delete;
    const event& operator=(event&& o) = delete;
    ~event() {
    }
    bool await_ready() {
        return false;
    }
    void await_suspend(std::experimental::coroutine_handle<> awaiting) {
        this->awaiting = awaiting;
    }
    void await_resume() {
    }
    void signal() {
        s2::move(awaiting).resume();
    }
};

