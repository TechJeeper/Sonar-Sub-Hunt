#pragma once

#include <pebble.h>

// Keep in sync with src/pkjs/index.js
typedef enum {
  SonarFeedback_Both = 0,
  SonarFeedback_Sound = 1,
  SonarFeedback_Vibrate = 2,
} SonarFeedbackMode;

// strength: 0 (nothing) … 100 (dead lock on target)
void sonar_init(void);
void sonar_deinit(void);
void sonar_set_strength(uint8_t strength);
void sonar_set_feedback_mode(SonarFeedbackMode mode);
SonarFeedbackMode sonar_get_feedback_mode(void);
void sonar_ping_now(void);           // immediate ping at current strength
void sonar_fire_fx(bool hit);        // shot / hit / miss feedback
void sonar_tick(void);               // call ~every 50ms from game timer
