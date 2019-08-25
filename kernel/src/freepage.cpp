#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>

struct free_page_list {
  uint64_t address;
  uint64_t pagecount;
  free_page_list* next;
};

static free_page_list* head = nullptr;

uint64_t freepage_get() {
  free_page_list* c = head;
  uint64_t page = c->address;
  c->address += 4096;
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


