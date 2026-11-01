#include "sonar.h"

#define PERSIST_KEY_FEEDBACK 1

static uint8_t s_strength = 0;
static uint16_t s_accum_ms = 0;
static bool s_muted = false;
static SonarFeedbackMode s_mode = SonarFeedback_Both;

static bool want_sound(void) {
  return s_mode == SonarFeedback_Both || s_mode == SonarFeedback_Sound;
}

static bool want_vibe(void) {
  return s_mode == SonarFeedback_Both || s_mode == SonarFeedback_Vibrate;
}

static uint16_t interval_for_strength(uint8_t s) {
  if (s >= 90) return 120;
  if (s >= 70) return 220;
  if (s >= 50) return 400;
  if (s >= 30) return 700;
  if (s >= 10) return 1100;
  return 1800;
}

static uint16_t tone_hz_for_strength(uint8_t s) {
  return (uint16_t)(180 + ((int)s * 12));
}

static void play_tone(uint16_t hz, uint16_t ms, uint8_t vol) {
  if (!want_sound()) return;
#if defined(PBL_SPEAKER)
  speaker_play_tone(hz, ms, vol, SpeakerWaveformSine);
#else
  (void)hz;
  (void)ms;
  (void)vol;
#endif
}

static void vibe_for_strength(uint8_t s) {
  if (!want_vibe()) return;
  if (s >= 85) {
    vibes_short_pulse();
  } else if (s >= 55) {
    static const uint32_t seg[] = { 40, 40, 40 };
    VibePattern p = { .durations = seg, .num_segments = 3 };
    vibes_enqueue_custom_pattern(p);
  } else if (s >= 25) {
    static const uint32_t seg[] = { 30 };
    VibePattern p = { .durations = seg, .num_segments = 1 };
    vibes_enqueue_custom_pattern(p);
  } else if (s >= 5) {
    static const uint32_t seg[] = { 18 };
    VibePattern p = { .durations = seg, .num_segments = 1 };
    vibes_enqueue_custom_pattern(p);
  }
}

static void play_fire_sound(bool hit) {
  if (!want_sound()) return;
#if defined(PBL_SPEAKER)
  if (hit) {
    static const SpeakerNote notes[] = {
      { .midi_note = 72, .waveform = SpeakerWaveformSawtooth, .duration_ms = 80 },
      { .midi_note = 76, .waveform = SpeakerWaveformSawtooth, .duration_ms = 80 },
      { .midi_note = 84, .waveform = SpeakerWaveformSquare,   .duration_ms = 180 },
    };
    speaker_play_notes(notes, ARRAY_LENGTH(notes), 85);
  } else {
    static const SpeakerNote notes[] = {
      { .midi_note = 50, .waveform = SpeakerWaveformTriangle, .duration_ms = 120 },
      { .midi_note = 43, .waveform = SpeakerWaveformTriangle, .duration_ms = 200 },
    };
    speaker_play_notes(notes, ARRAY_LENGTH(notes), 70);
  }
#else
  (void)hit;
#endif
}

static void play_fire_vibe(bool hit) {
  if (!want_vibe()) return;
  if (hit) {
    vibes_double_pulse();
  } else {
    vibes_short_pulse();
  }
}

void sonar_init(void) {
  s_strength = 0;
  s_accum_ms = 0;
  s_muted = false;
  s_mode = SonarFeedback_Both;
  if (persist_exists(PERSIST_KEY_FEEDBACK)) {
    int32_t v = persist_read_int(PERSIST_KEY_FEEDBACK);
    if (v == SonarFeedback_Sound || v == SonarFeedback_Vibrate || v == SonarFeedback_Both) {
      s_mode = (SonarFeedbackMode)v;
    }
  }
#if defined(PBL_SPEAKER)
  speaker_set_volume(70);
#endif
}

void sonar_deinit(void) {
#if defined(PBL_SPEAKER)
  speaker_stop();
#endif
  vibes_cancel();
}

void sonar_set_feedback_mode(SonarFeedbackMode mode) {
  if (mode != SonarFeedback_Both && mode != SonarFeedback_Sound && mode != SonarFeedback_Vibrate) {
    mode = SonarFeedback_Both;
  }
  s_mode = mode;
  persist_write_int(PERSIST_KEY_FEEDBACK, (int32_t)mode);
}

SonarFeedbackMode sonar_get_feedback_mode(void) {
  return s_mode;
}

void sonar_set_strength(uint8_t strength) {
  if (strength > 100) strength = 100;
  s_strength = strength;
}

void sonar_ping_now(void) {
  if (s_muted) return;
  play_tone(tone_hz_for_strength(s_strength), 45, 65);
  vibe_for_strength(s_strength);
  s_accum_ms = 0;
}

void sonar_fire_fx(bool hit) {
  s_muted = true;
  vibes_cancel();
#if defined(PBL_SPEAKER)
  speaker_stop();
#endif
  play_fire_sound(hit);
  play_fire_vibe(hit);
  s_accum_ms = 0;
}

void sonar_tick(void) {
  const uint16_t step = 50;
  if (s_muted) {
    s_accum_ms += step;
    if (s_accum_ms >= 450) {
      s_muted = false;
      s_accum_ms = 0;
    }
    return;
  }

  s_accum_ms += step;
  if (s_accum_ms >= interval_for_strength(s_strength)) {
    sonar_ping_now();
  }
}
