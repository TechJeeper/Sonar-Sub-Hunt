#include "game.h"
#include "sonar.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static Game s_game;
static AppTimer *s_timer = NULL;

static int16_t wrap_deg(int16_t d) {
  while (d < 0) d += 360;
  while (d >= 360) d -= 360;
  return d;
}

static int16_t heading_delta(int16_t from, int16_t to) {
  int16_t d = (int16_t)(to - from);
  while (d > 180) d -= 360;
  while (d < -180) d += 360;
  return d;
}

static int16_t abs16(int16_t v) {
  return v < 0 ? (int16_t)(-v) : v;
}

static uint8_t strength_from_deltas(int16_t dh, int16_t de) {
  // Combined angular miss in degrees (approx).
  int32_t miss = abs16(dh) + (abs16(de) * 3) / 2;
  if (miss >= 90) return 0;
  if (miss <= 4) return 100;
  // Map 4…90 → 100…0
  int32_t s = 100 - ((miss - 4) * 100) / 86;
  if (s < 0) s = 0;
  if (s > 100) s = 100;
  return (uint8_t)s;
}

static void place_subs(uint8_t wave) {
  uint8_t count = (uint8_t)(2 + (wave / 2));
  if (count > GAME_MAX_SUBS) count = GAME_MAX_SUBS;
  s_game.sub_count = count;
  s_game.alive_count = count;

  AimState aim = aim_get();
  for (uint8_t i = 0; i < count; i++) {
    // Scatter around the player; keep at least ~25° apart-ish via offset slots.
    int16_t slot = (int16_t)((360 / count) * i + (rand() % 40) - 20);
    s_game.subs[i].heading_deg = wrap_deg((int16_t)(aim.heading_deg + 50 + slot));
    s_game.subs[i].elev_deg = (int16_t)((rand() % 81) - 40); // −40…+40
    s_game.subs[i].alive = true;
  }
}

static void set_state(GameState st, const char *banner) {
  s_game.state = st;
  s_game.state_ms = 0;
  if (banner) {
    strncpy(s_game.banner, banner, sizeof(s_game.banner) - 1);
    s_game.banner[sizeof(s_game.banner) - 1] = '\0';
  } else {
    s_game.banner[0] = '\0';
  }
}

static void update_proximity(void) {
  AimState aim = aim_get();
  uint8_t best = 0;
  int16_t best_dh = 0;
  int16_t best_de = 0;

  for (uint8_t i = 0; i < s_game.sub_count; i++) {
    if (!s_game.subs[i].alive) continue;
    int16_t dh = heading_delta(aim.heading_deg, s_game.subs[i].heading_deg);
    int16_t de = (int16_t)(s_game.subs[i].elev_deg - aim.elev_deg);
    uint8_t s = strength_from_deltas(dh, de);
    if (s >= best) {
      best = s;
      best_dh = dh;
      best_de = de;
    }
  }

  s_game.strength = best;
  s_game.nearest_bearing = best_dh;
  s_game.nearest_elev_delta = best_de;
  sonar_set_strength(best);
}

static void timer_cb(void *ctx);

static void schedule_tick(void) {
  s_timer = app_timer_register(GAME_TICK_MS, timer_cb, NULL);
}

static void timer_cb(void *ctx) {
  (void)ctx;
  s_timer = NULL;
  game_tick();
  schedule_tick();
}

void game_init(void) {
  memset(&s_game, 0, sizeof(s_game));
  s_game.torpedoes = 8;
  set_state(GameState_Title, "SONAR SUB HUNT");
  aim_init();
  sonar_init();
  schedule_tick();
}

void game_deinit(void) {
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  sonar_deinit();
  aim_deinit();
}

void game_start(void) {
  srand(time(NULL));
  s_game.wave = 1;
  s_game.score = 0;
  s_game.torpedoes = 8;
  place_subs(s_game.wave);
  set_state(GameState_Playing, "");
  sonar_set_strength(0);
}

void game_fire(void) {
  if (s_game.state == GameState_Title || s_game.state == GameState_GameOver) {
    game_start();
    return;
  }
  if (s_game.state == GameState_WaveClear) {
    s_game.wave++;
    place_subs(s_game.wave);
    if (s_game.torpedoes < 8) s_game.torpedoes++;
    set_state(GameState_Playing, "");
    return;
  }
  if (s_game.state != GameState_Playing) return;
  if (s_game.torpedoes == 0) return;

  s_game.torpedoes--;
  update_proximity();

  // Hit cone: tight lock required.
  bool hit = false;
  AimState aim = aim_get();
  for (uint8_t i = 0; i < s_game.sub_count; i++) {
    if (!s_game.subs[i].alive) continue;
    int16_t dh = abs16(heading_delta(aim.heading_deg, s_game.subs[i].heading_deg));
    int16_t de = abs16((int16_t)(s_game.subs[i].elev_deg - aim.elev_deg));
    if (dh <= 8 && de <= 10) {
      s_game.subs[i].alive = false;
      s_game.alive_count--;
      hit = true;
      s_game.score = (uint16_t)(s_game.score + 100 + (s_game.wave * 25));
      break;
    }
  }

  s_game.last_shot_hit = hit;
  sonar_fire_fx(hit);

  if (hit) {
    if (s_game.alive_count == 0) {
      set_state(GameState_WaveClear, "WAVE CLEAR");
    } else {
      set_state(GameState_Feedback, "HIT!");
    }
  } else {
    if (s_game.torpedoes == 0) {
      set_state(GameState_GameOver, "OUT OF TORPS");
    } else {
      set_state(GameState_Feedback, "MISS");
    }
  }
}

void game_tick(void) {
  s_game.state_ms = (uint16_t)(s_game.state_ms + GAME_TICK_MS);

  if (s_game.state == GameState_Playing) {
    update_proximity();
    sonar_tick();
  } else if (s_game.state == GameState_Feedback) {
    sonar_tick();  // still drains mute timer
    if (s_game.state_ms >= 700) {
      if (s_game.torpedoes == 0 && s_game.alive_count > 0) {
        set_state(GameState_GameOver, "OUT OF TORPS");
      } else {
        set_state(GameState_Playing, "");
      }
    }
  } else if (s_game.state == GameState_Title) {
    // Soft idle ping so the speaker/vibe path is discoverable.
    sonar_set_strength(5);
    sonar_tick();
  }
}

const Game *game_get(void) {
  return &s_game;
}
