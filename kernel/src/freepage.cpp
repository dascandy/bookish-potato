#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "map.h"

struct free_page_list {
  uint64_t address;
  uint64_t pagecount;
  free_page_list* next;
};

static free_page_list* head = nullptr;

uint64_t freepage_get() {
  free_page_list* c = head;
  uint64_t page = c->address + 4096 * c->pagecount;
  c->pagecount--;
  if (!c->pagecount) {
    head = c->next;
    delete c;
  }
  return page;
}

void freepage_add_region(uint64_t start, size_t length) {
  free_page_list* c = new free_page_list;
  length -= (4096 - (start & 0xFFF));
  start = (start + 4095) & ~((uint64_t)0xFFF);
  c->address = start;
  c->pagecount = length / 4096;
  c->next = head;
  head = c;
}

uint64_t freepage_get_zeroed() {
  uint64_t freepage = freepage_get();
  mapping m(freepage, 0x1000, DeviceMemory); // Using device memory to make sure the writes are not cached
  memset(m.get(), 0, 0x1000);
  return freepage;
}


