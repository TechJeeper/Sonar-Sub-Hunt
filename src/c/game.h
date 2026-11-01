#pragma once

#include <pebble.h>
#include "aim.h"

#define GAME_MAX_SUBS 6
#define GAME_TICK_MS  50

typedef enum {
  GameState_Title = 0,
  GameState_Playing,
  GameState_Feedback,
  GameState_WaveClear,
  GameState_GameOver,
} GameState;

typedef struct {
  int16_t heading_deg;
  int16_t elev_deg;
  bool alive;
} Sub;

typedef struct {
  GameState state;
  Sub subs[GAME_MAX_SUBS];
  uint8_t sub_count;
  uint8_t alive_count;
  uint8_t wave;
  uint8_t torpedoes;
  uint16_t score;
  uint8_t strength;       // 0–100 proximity to nearest sub
  int16_t nearest_bearing; // signed heading delta to nearest (−180…180)
  int16_t nearest_elev_delta;
  char banner[32];
  uint16_t state_ms;      // time spent in current transient state
  bool last_shot_hit;
} Game;

void game_init(void);
void game_deinit(void);
void game_start(void);
void game_fire(void);
void game_tick(void);
const Game *game_get(void);
