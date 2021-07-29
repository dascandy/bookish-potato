#include <string>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <cassert>

size_t strlen(const char *str) {
  size_t n = 0;
  while (str[n]) n++;
  return n;
}

int strcmp(const char *a, const char *b) {
  while (*a && *a == *b) { 
    a++; 
    b++; 
  }
  return *a - *b;
}

char *strstr(const char *haystack, const char *needle) {
  if (strlen(needle) > strlen(haystack)) return nullptr;
  size_t lim = strlen(haystack) - strlen(needle);
  for (size_t offset = 0; offset < lim; offset++) {
    if (memcmp(haystack+offset, needle, strlen(needle)) == 0)
      return const_cast<char*>(haystack + offset);
  }
  return nullptr;
}

char *strdup(const char *str) {
  size_t len = strlen(str);
  char *c = reinterpret_cast<char*>(malloc(len + 1));
  memcpy(c, str, len+1);
  return c;
}

char *strrchr(char *str, char v) {
  char *p = str + strlen(str) - 1;
  while (*p != v && str != p) {
    p--;
  }
  if (*p == v) return p;
  return nullptr;
}

char *strchr(char *s, int c) {
  while (*s && *s != c) s++;
  return *s ? s : nullptr;
}

char *strcpy(char *dst, const char *src) {
  char *dest = dst;
  while (*src) 
    *dst++ = *src++;
  *dst = 0;
  return dest;
}

void memmove(void *dst, const void *src, size_t count) {
  char *d = reinterpret_cast<char*>(dst);
  const char *s = reinterpret_cast<const char*>(src);
  if (d < s) {
    for (size_t n = 0; n < count; n++) {
      d[n] = s[n];
    }
  } else {
    for (size_t n = count; n > 0; n--) {
      d[n-1] = s[n-1];
    }
  }
}

void memcpy(void *dst, const void *src, size_t count) {
  memmove(dst, src, count);
}

void memset(void *target, uint8_t c, size_t n) {
  uint8_t *t = reinterpret_cast<uint8_t *>(target);
  for (size_t i = 0; i < n; i++) {
    t[i] = c;
  }
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *t1 = (const uint8_t*)s1,
                *t2 = (const uint8_t*)s2;
  for (size_t i = 0; i < n; ++i) {
    if (t1[i] != t2[i]) return t2[i] - t1[i];
  }
  return 0;
}

namespace s2 {

static char32_t cp437_table[256] = {
  0x0000, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
  0x25ba, 0x25c4, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc,
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
  0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
  0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192, 
  0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb, 
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510, 
  0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567, 
  0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580, 
  0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229, 
  0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0, 
};

string string::from_cp437(span<const uint8_t> data) {
  string s;
  for (auto& v : data) {
    s.push_back(cp437_table[v]);
  }
  return s;
}

static inline uint16_t getValue(const uint8_t*& ptr, bool littleEndian) {
  uint16_t value;
  if (littleEndian) {
    value = (ptr[1] << 8) | ptr[0];
  } else {
    value = (ptr[0] << 8) | ptr[1];
  }
  ptr += 2;
  return value;
}

static uint32_t getUtf16Char(const uint8_t *&ptr, bool littleEndian, bool lastEntry) {
  uint16_t v1 = getValue(ptr, littleEndian);
  if (v1 >= 0xD800 && v1 <= 0xDBFF) {
    if (lastEntry) {
      return 0xFFFD;
    }
    uint32_t val = (v1 << 10) & 0xFFC00;
    uint16_t v2 = getValue(ptr, littleEndian);
    if (v2 >= 0xDC00 && v2 <= 0xDFFF) {
      val |= (v2 & 0x3FF);
      val += 0x10000;
      return val;
    } else {
      // Missing surrogate; leave second half here
      ptr -= 2;
      return 0xFFFD;
    }
  } else if (v1 >= 0xDC00 && v1 <= 0xDFFF) {
    return 0xFFFD;
  } else {
    return v1;
  }
}

string string::from_utf16(span<const uint8_t> data, bool littleEndian) {
  assert(data.size() % 2 == 0);
  string s;
  const uint8_t* p = data.data(), *e = data.data() + data.size();
  while (p != e) {
    s.push_back(getUtf16Char(p, littleEndian, (p == e-2)));
  }
  return s;
}

string string::from_utf32(span<const uint8_t> data, bool littleEndian) {
  assert(data.size() % 4 == 0);

  string s;
  for (size_t index = 0; index < data.size(); index += 4) {
    uint32_t value = littleEndian 
       ? (data[index]) | (data[index+1] << 8) | (data[index+2] << 16) | (data[index+3] << 24)
       : (data[index+3]) | (data[index+2] << 8) | (data[index+1] << 16) | (data[index] << 24);
    if ((value >= 0xD800 && value < 0xE000) || value > 0x10FFFF) 
      s.push_back(0xFFFD);
    else
      s.push_back(value);
  }
  return s;
}

}

