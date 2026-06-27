#pragma once
#include <stdint.h>

// Enter light sleep; wakes on touch (GPIO23/TP_INT, active low).
// Handles display sleep/wake internally. Optionally also wakes on timer.
void enterLightSleepTouchWake(uint64_t timer_wake_us = 0);
