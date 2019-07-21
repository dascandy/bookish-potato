#pragma once

#include <cstddef>
#include <cstdint>

void malloc_add_region(uint64_t start, size_t length);

void *malloc(size_t size);
void free(void *p);
void *realloc(void* p, size_t size);
void abort();
int atexit(void (*func)());
int atoi(const char *nptr);
char *getenv(const char *name);

long int strtol(const char *nptr, char **endptr, int base);


