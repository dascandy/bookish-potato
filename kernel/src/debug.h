#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

void debug_init();
void debug_char(char c);
void debug_field(void* value, std::string_view spec);

template <typename T>
inline void debug_field(T* value, std::string_view spec) { debug_field((void*)value, spec); }
void debug_field(uint64_t value, std::string_view spec);
inline void debug_field(uint32_t value, std::string_view spec) { debug_field((uint64_t)value, spec); }
inline void debug_field(uint16_t value, std::string_view spec) { debug_field((uint64_t)value, spec); }
inline void debug_field(uint8_t value, std::string_view spec) { debug_field((uint64_t)value, spec); }
void debug_field(int64_t value, std::string_view spec);
void debug_field(std::string_view text, std::string_view spec = "");
inline void debug_field(const char* value, std::string_view spec) {
  debug_field(std::string_view(value), spec);
}

inline void debug(std::string_view text) {
  debug_field(text, "");
}

template <typename T, typename... Ts>
void debug(std::string_view text, T&& t, Ts&&... ts) {
  const char* p = text.begin(), *e = text.end();
  const char* s = p;
  while (p != e && *p != '{') p++;
  debug_field(std::string_view(s, p));
  if (p != e) {
    ++p;
    const char* pp = p;
    if (p == e) return; // bug
    while (p != e && *p != '}') p++;
    debug_field(t, std::string_view(pp, p));
    ++p;
    if (p == e) return;
      debug(std::string_view(p, e), ts...);
  }
}

