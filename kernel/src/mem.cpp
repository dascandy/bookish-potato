#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "debug.h"

struct malloc_entry {
  malloc_entry *next, *prev;
  size_t length;
  size_t free;
};

struct {
  malloc_entry ent;
  char reserve[1024*1024*14];
} malloc_reserve;

malloc_entry* last_used = &malloc_reserve.ent;

void *malloc(size_t size) {
  malloc_entry *cur = last_used;
  if (cur->prev == nullptr) {
    cur->prev = cur->next = cur;
    cur->length = sizeof(malloc_reserve.reserve);
    cur->free = true;
  }
  //debug("lu = %p %s %d\n", last_used, last_used->free ? "free" : "used", last_used->length);
  // TODO: handle memory-is-out case
  while (!cur->free || cur->length < size)
    cur = cur->next;
  cur->free = false;
  if (cur->length >= size + sizeof(malloc_entry)) {
    malloc_entry *next = reinterpret_cast<malloc_entry*>(reinterpret_cast<char*>(cur) + ((size + 15) & 0xFFFFFFF0) + sizeof(malloc_entry));
    next->free = true;
    next->length = cur->length - ((size_t)next - (size_t)cur);
    cur->length = (size_t)next - (size_t)cur - sizeof(malloc_entry);
    next->prev = cur;
    next->next = cur->next;
    next->next->prev = next;
    next->prev->next = next;
    last_used = next;
  }
  void *rv = (void*)(cur + 1);
  //debug("[MEM] malloc %08X (%p = %d)\n", rv, cur, size);
  return rv;
}

void free(void *p) {
  if (!p) return;
  malloc_entry *cur = reinterpret_cast<malloc_entry*>(reinterpret_cast<char*>(p) - sizeof(malloc_entry));
  if (cur->free > 1) {
    debug("MEMORY CORRUPTION\n");
    while(1) {}
  } else if (cur->free) {
    debug("DOUBLE FREE!\n");
    while(1) {}
  }
  //debug("[MEM] free %08X (%p = %d)\n", p, cur, cur->length);
  //debug("[MEM] ents %08X .. %08X .. %08X\n", cur->prev, cur, cur->next);
  //debug("[MEM] size %08X -- %08X -- %08X\n", cur->prev->length, cur->length, cur->next->length);
  cur->free = true;
  //debug("[MEM] prev? %08X with %08X\n", (char*)cur->prev + cur->prev->length + sizeof(malloc_entry), cur);
  if (cur->prev->free &&
      (uintptr_t)cur->prev + cur->prev->length + sizeof(malloc_entry) == (uintptr_t)cur) {
    //debug("[MEM] merging prev %08X with %08X\n", cur->prev, cur);
    cur->prev->length += cur->length + sizeof(malloc_entry);
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    cur = cur->prev;
  }
  //debug("[MEM] next? %08X with %08X\n", (char*)cur + cur->length + sizeof(malloc_entry), cur->next);
  if (cur->next->free &&
      (uintptr_t) cur + cur->length + sizeof(malloc_entry) == (uintptr_t)cur->next) {
    //debug("[MEM] merging next %08X with %08X\n", cur, cur->next);
    cur->length += cur->next->length + sizeof(malloc_entry);
    cur->next = cur->next->next;
    cur->next->prev = cur;
  }
  last_used = cur;
}

void *realloc(void* p, size_t size) {
  debug("[MEM] realloc {p}\n", p);
  malloc_entry *cur = reinterpret_cast<malloc_entry*>(reinterpret_cast<char*>(p) - sizeof(malloc_entry));
  if (p && cur->length > size) {
    debug("[MEM] realloc => {p}\n", p);
    return p;
  }
  void *newp = malloc(size);
  if (p) {
    memcpy(newp, p, cur->length);
    free(p);
  }
  debug("[MEM] realloc => {p}\n", newp);
  return newp;
}

void *calloc(size_t nmemb, size_t size) {
  void *p = malloc(nmemb * size);
  memset(p, 0, nmemb * size);
  return p;
}

void *operator new(size_t size) {
  return malloc(size);
}

void operator delete(void *p) {
  free(p);
}

void operator delete(void *p, size_t) {
  free(p);
}

void *operator new[](size_t size) {
  return malloc(size);
}

void operator delete[](void *p) {
  free(p);
}

void operator delete[](void *p, size_t) {
  free(p);
}


