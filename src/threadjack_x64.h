#pragma once

namespace threadjack {
bool init();

bool interrupt(HANDLE thread, PFIBER_START_ROUTINE fn, void* arg);

}

