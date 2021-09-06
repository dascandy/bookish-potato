#pragma once

#include <future.h>

struct Disk;

s2::future<bool> ParseMbrPartitions(Disk* disk);


