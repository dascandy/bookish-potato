#pragma once

#include <stddef.h>
#include <stdint.h>

extern "C" size_t strnlen(const char* str, size_t maxN);
extern "C" size_t strlen(const char *str);
extern "C" int strcmp(const char *a, const char *b);
extern "C" char *strcpy(char *dst, const char *src);
extern "C" char *strstr(const char *haystack, const char *needle);
extern "C" void memcpy(void *dst, const void *src, size_t count);
extern "C" void memmove(void *dst, const void *src, size_t count);
extern "C" void memset(void *target, uint8_t c, size_t n);
extern "C" int memcmp(const void *s1, const void *s2, size_t n);
extern "C" char *strdup(const char *s);
extern "C" char *strrchr(char *s, char c);
extern "C" char *strchr(char *s, int c);

