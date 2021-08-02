#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>
#include <type_traits>

#ifdef __x86_64__
void debug_init();
#else
void debug_init(uintptr_t mmiobase);
#endif
void debug_char(char c);
void debug_field(void* value, s2::string_view spec);

void debug_field(uint64_t value, s2::string_view spec);
void debug_field(bool value, s2::string_view spec);
inline void debug_field(uint32_t value, s2::string_view spec) { debug_field((uint64_t)value, spec); }
inline void debug_field(uint16_t value, s2::string_view spec) { debug_field((uint64_t)value, spec); }
inline void debug_field(uint8_t value, s2::string_view spec) { debug_field((uint64_t)value, spec); }
void debug_field(int64_t value, s2::string_view spec);
void debug_field(s2::string_view text, s2::string_view spec = "");

template <typename T>
inline void debug_field(T* value, s2::string_view spec) {
  if constexpr (s2::is_convertible<T*, s2::string_view>::value) {
    if (!spec.empty() && spec[0] == 's') {
      debug_field(s2::string_view(value), spec);
    } else {
      debug_field((void*)value, spec); 
    }
  } else {
    debug_field((void*)value, spec); 
  }
}

inline void debug(s2::string_view text) {
  debug_field(text, "");
}

template <typename T, typename... Ts>
void debug(s2::string_view text, T&& t, Ts&&... ts) {
  const char* p = text.begin(), *e = text.end();
  const char* s = p;
  while (p != e && *p != '{') p++;
  debug_field(s2::string_view(s, p));
  if (p != e) {
    ++p;
    const char* pp = p;
    if (p == e) return; // bug
    while (p != e && *p != '}') p++;
    debug_field(t, s2::string_view(pp, p));
    ++p;
    if (p == e) return;
      debug(s2::string_view(p, e), ts...);
  }
}

