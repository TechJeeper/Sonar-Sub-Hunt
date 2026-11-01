#pragma once

#include <pebble.h>

// Player aim direction in degrees.
// heading: 0–359 (0 = north / forward)
// elevation: −90…+90 (negative = down, positive = up)
typedef struct {
  int16_t heading_deg;
  int16_t elev_deg;
  bool compass_ok;
} AimState;

void aim_init(void);
void aim_deinit(void);
AimState aim_get(void);
