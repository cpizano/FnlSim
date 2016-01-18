#pragma once

namespace sim {
const int num_cores = 4;

void init();
void core_start(int core_id, void* start);
void run();
void* kern_gs(void* new_gs);
void* user_gs(void* new_gs);
}