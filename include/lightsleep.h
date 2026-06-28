#pragma once
#include <stdint.h>

// Enter light sleep; wakes on touch (GPIO23/TP_INT, active low).
// Handles display sleep/wake internally. Optionally also wakes on timer.
void lightSleepTouchWake(uint64_t sleep_us = 0);
