#pragma once

#include <cstdint>
#include <cstddef>
#include <string_view>

void debug_init();
void debug(const void* value, std::string_view spec);
void debug(uint64_t value, std::string_view spec);
void debug(int64_t value, std::string_view spec);
void debug(std::string_view text, std::string_view spec = "");
template <typename T, typename... Ts>
void debug(std::string_view text, T&& t, Ts&&... ts) {
  const char* p = text.begin(), *e = text.end();
  const char* s = p;
  while (p != e && *p != '{') p++;
  debug(std::string_view(s, p));
  if (p != e) {
    ++p;
    const char* pp = p;
    if (p == e) return; // bug
    while (p != e && *p != '}') p++;
    debug(t, std::string_view(pp, p));
    ++p;
    if (p == e) return;
    debug(std::string_view(p, e), ts...);
  }
}


