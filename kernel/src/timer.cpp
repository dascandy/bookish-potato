#include "timer.h"
#include <cstdint>
#include "io.h"
#include <cstring>
#include "interrupt.h"
#include "debug.h"

uint64_t utc_offset;

struct timer_entry {
  event* p;
  uint64_t expire;
  inline bool isExpired(uint64_t currentTimer) const {
    if (expire == 0) {
      // unused entry
      return false;
    } else if (expire & 0x8000000000000000ull) {
      // UTC based timeout
      return (expire & 0x7FFFFFFFFFFFFFFFULL) <= currentTimer + utc_offset;
    } else {
      // delta based timeout
      return expire <= currentTimer;
    }
  }
  uint64_t getTimeout() const {
    if (expire & 0x8000000000000000ull) {
      return (expire & 0x7FFFFFFFFFFFFFFFULL) - utc_offset;
    } else {
      return expire;
    }
  }
};

timer_entry entries[20];

void timer_check() {
  uint64_t firstTimeout = 0;
  uint64_t currentTime = get_timer_value();
  for (auto& timer : entries) {
    if (timer.isExpired(currentTime)) {
      timer.expire = 0;
      timer.p->signal();
    } else {
      if ((timer.getTimeout() != 0 && timer.getTimeout() < firstTimeout) || firstTimeout == 0) 
        firstTimeout = timer.getTimeout();
    }
  }
  if (firstTimeout != 0) {
    timer_set_interrupt(firstTimeout);
  }
}

uint64_t get_current_time() {
  return get_timer_value() + utc_offset;
}

void set_utc_offset(uint64_t utc_offset) {
  ::utc_offset = utc_offset;
}

event wait_until(uint64_t count) {
  for (auto& e : entries) {
    if (e.expire == 0) {
      e.expire = 0x8000000000000000ull | count;
      timer_check();
      return e.p;
    }
  }
  return {}; // TODO: fix this
}

event wait_for_us(uint64_t count) {
  for (auto& e : entries) {
    if (e.expire == 0) {
      e.expire = count + get_timer_value();
      timer_check();
      return e.p;
    }
  }
  return {}; // TODO: fix this
}

