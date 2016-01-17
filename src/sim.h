#pragma once

namespace sim {
const int num_cores = 4;

void init();
void core_start(int core_id, void* start);
void run();

}