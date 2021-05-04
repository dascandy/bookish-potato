#pragma once

#include "event.h"
#include <cstddef>
#include <cstdint>

// initialize timer logic
void timer_init(uintptr_t mmio_base);

// Trigger checks on all timers. To be called from interrupts normally.
void timer_check(); 

// Get current timer value (microseconds since system startup)
uint64_t get_timer_value();

// Get current time (microseconds since Jan 1st 1970)
uint64_t get_current_time();

// Set UTC offset between current timer value and current time (needed to make get_current_time work)
void set_utc_offset(uint64_t utc_offset);

// Return an awaitable event that triggers at the specified time point
event wait_until(uint64_t count);

// Return an awaitable event that triggers after the specified number of microseconds
event wait_for_us(uint64_t count);

// Delay operation for a specified number of nanoseconds. Not meant for longer delays; this is purely for hardware IO
void delay_ns(uint64_t count);

// Internal functions
// UTC offset to the timer value. Set using set_utc_offset.
extern uint64_t utc_offset;

// Sets the underlying timer interrupt moment to the next moment we want to check our timers
void timer_set_interrupt(uint64_t nextTimeout);

