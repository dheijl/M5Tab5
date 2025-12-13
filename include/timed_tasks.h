
#include "mpdcli.h"
#include "p1dongle.h"

typedef struct touch_params {
    MPD_PLAYER* mpd_pl;
    P1_DATA* dongle_data;
} TOUCH_PARAMS;

void CheckPowerOff(void* params);
void CheckTouch(void* params);