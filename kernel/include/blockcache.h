#pragma once

#include <cstdint>
#include <future.h>
#include <map.h>
#include <flatmap>
#include "debug.h"

struct Blockcache {
  struct Entry {
    uint64_t page;
  };

  static Blockcache& Instance() {
    static Blockcache cache;
    return cache;
  }

  s2::future<PageSGList> read(Disk* disk, uint64_t start, uint64_t count) {
    // TODO: larger reads
    // TODO: somehow make this concurrency safe

    debug("[BC] {x} {}\n", start, count);
    PageSGList full;
    auto& diskcache = entries[disk];
    for (size_t n = 0; n < count; n++) {
      Entry& ent = diskcache[start + n];
      if (ent.page == 0) {
        debug("[BC] read {}\n", start + n);
        PageSGList mem = co_await disk->read(start + n, 1);
        ent.page = mem.pages[0];
      }
      full.pages.push_back(ent.page);
    }
    co_return full;
  }

  s2::flatmap<Disk*, s2::flatmap<uint64_t, Entry>> entries;
};


