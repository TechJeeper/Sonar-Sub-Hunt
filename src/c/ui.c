#include "ui.h"
#include "game.h"
#include "aim.h"

static Window *s_window;
static Layer *s_canvas;
static AppTimer *s_redraw = NULL;
static GBitmap *s_bmp_sub = NULL;
static GBitmap *s_bmp_reticle = NULL;
static GBitmap *s_bmp_title = NULL;

#if defined(PBL_COLOR)
#define C_BG      GColorBlack
#define C_WATER   GColorDukeBlue
#define C_RING    GColorCyan
#define C_SWEEP   GColorBrightGreen
#define C_LOCK    GColorRed
#define C_TEXT    GColorWhite
#define C_DIM     GColorLightGray
#define C_SUBHINT GColorYellow
#else
#define C_BG      GColorBlack
#define C_WATER   GColorBlack
#define C_RING    GColorWhite
#define C_SWEEP   GColorWhite
#define C_LOCK    GColorWhite
#define C_TEXT    GColorWhite
#define C_DIM     GColorWhite
#define C_SUBHINT GColorWhite
#endif

static void redraw_cb(void *ctx) {
  (void)ctx;
  s_redraw = NULL;
  if (s_canvas) layer_mark_dirty(s_canvas);
  s_redraw = app_timer_register(100, redraw_cb, NULL);
}

static void draw_bitmap_centered(GContext *ctx, GBitmap *bmp, GPoint center) {
  if (!bmp) return;
  GRect b = gbitmap_get_bounds(bmp);
  GRect dest = GRect(center.x - b.size.w / 2, center.y - b.size.h / 2, b.size.w, b.size.h);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bmp, dest);
}

static void draw_radar(GContext *ctx, GRect bounds, const Game *g, const AimState *aim) {
  const int16_t cx = bounds.origin.x + bounds.size.w / 2;
  const int16_t cy = bounds.origin.y + bounds.size.h / 2 + 6;
  int16_t radius = bounds.size.w / 2 - 6;
  if (bounds.size.h / 2 - 18 < radius) radius = bounds.size.h / 2 - 18;

  graphics_context_set_fill_color(ctx, C_WATER);
  graphics_fill_circle(ctx, GPoint(cx, cy), radius);

  graphics_context_set_stroke_color(ctx, C_RING);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(cx, cy), radius);
  graphics_draw_circle(ctx, GPoint(cx, cy), radius * 2 / 3);
  graphics_draw_circle(ctx, GPoint(cx, cy), radius / 3);

  graphics_draw_line(ctx, GPoint(cx - radius, cy), GPoint(cx + radius, cy));
  graphics_draw_line(ctx, GPoint(cx, cy - radius), GPoint(cx, cy + radius));

  if (g->strength > 0) {
    int16_t sr = radius / 3 + (radius * 2 / 3) * (100 - g->strength) / 100;
    graphics_context_set_stroke_color(ctx, g->strength >= 85 ? C_LOCK : C_SWEEP);
    graphics_context_set_stroke_width(ctx, g->strength >= 70 ? 3 : 1);
    graphics_draw_circle(ctx, GPoint(cx, cy), sr);
  }

  // Contact: sub sprite along bearing when we have a ping lock
  if (g->state == GameState_Playing && g->strength > 0) {
    int32_t trig = DEG_TO_TRIGANGLE(g->nearest_bearing);
    int16_t reach = radius - 10;
    if (g->strength < 85) {
      // Keep contact near the outer ring until lock
      reach = radius - 8;
    } else {
      reach = radius / 3;
    }
    int16_t nx = (int16_t)(cx + (sin_lookup(trig) * reach / TRIG_MAX_RATIO));
    int16_t ny = (int16_t)(cy - (cos_lookup(trig) * reach / TRIG_MAX_RATIO));

    if (s_bmp_sub) {
      draw_bitmap_centered(ctx, s_bmp_sub, GPoint(nx, ny));
    } else {
      graphics_context_set_fill_color(ctx, C_SUBHINT);
      graphics_fill_circle(ctx, GPoint(nx, ny), 3);
    }
  }

  // Center reticle (lock glow uses red stroke behind it)
  if (g->strength >= 85) {
    graphics_context_set_stroke_color(ctx, C_LOCK);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, GPoint(cx, cy), 18);
  }
  if (s_bmp_reticle) {
    draw_bitmap_centered(ctx, s_bmp_reticle, GPoint(cx, cy));
  } else {
    graphics_context_set_fill_color(ctx, g->strength >= 85 ? C_LOCK : C_SWEEP);
    graphics_fill_circle(ctx, GPoint(cx, cy), g->strength >= 85 ? 4 : 2);
  }

  (void)aim;
}

static void canvas_update(Layer *layer, GContext *ctx) {
  const Game *g = game_get();
  AimState aim = aim_get();
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, C_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  char line[32];
  graphics_context_set_text_color(ctx, C_TEXT);

  if (g->state == GameState_Title) {
    if (s_bmp_title) {
      GRect tb = gbitmap_get_bounds(s_bmp_title);
      int16_t tw = tb.size.w;
      if (tw > bounds.size.w - 8) tw = bounds.size.w - 8;
      int16_t th = tb.size.h;
      GRect dest = GRect((bounds.size.w - tw) / 2, 8, tw, th);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, s_bmp_title, dest);

      graphics_draw_text(ctx, "SONAR SUB HUNT",
                         fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(2, dest.origin.y + dest.size.h + 2, bounds.size.w - 4, 22),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else {
      graphics_draw_text(ctx, "SONAR", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(0, 28, bounds.size.w, 32), GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentCenter, NULL);
      graphics_draw_text(ctx, "SUB HUNT", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(0, 56, bounds.size.w, 32), GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentCenter, NULL);
    }

    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "Sweep arm to hunt", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(4, bounds.size.h - 58, bounds.size.w - 8, 18),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
#if defined(PBL_TOUCH)
    const char *hint = touch_service_is_enabled() ? "Tap screen to start"
                                                  : "SELECT to start";
#else
    const char *hint = "SELECT to start";
#endif
    graphics_context_set_text_color(ctx, C_TEXT);
    graphics_draw_text(ctx, hint, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(4, bounds.size.h - 36, bounds.size.w - 8, 24),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

  snprintf(line, sizeof(line), "W%d  %dpts", (int)g->wave, (int)g->score);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(4, 0, bounds.size.w - 8, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  snprintf(line, sizeof(line), "T:%d", (int)g->torpedoes);
  graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(4, 0, bounds.size.w - 8, 16), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentRight, NULL);

  // Show sub banner art briefly on hit
  if (g->state == GameState_Feedback && g->last_shot_hit && s_bmp_sub) {
    draw_bitmap_centered(ctx, s_bmp_sub,
                         GPoint(bounds.size.w / 2, bounds.size.h / 2 - 8));
  }

  GRect radar = GRect(0, 18, bounds.size.w, bounds.size.h - 50);
  if (!(g->state == GameState_Feedback && g->last_shot_hit)) {
    draw_radar(ctx, radar, g, &aim);
  }

  graphics_context_set_text_color(ctx, C_TEXT);
  if (g->banner[0]) {
    graphics_draw_text(ctx, g->banner, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                       GRect(2, bounds.size.h - 42, bounds.size.w - 4, 22),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else if (g->state == GameState_Playing) {
    if (!aim.compass_ok) {
      graphics_draw_text(ctx, "Calibrate compass…", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(2, bounds.size.h - 40, bounds.size.w - 4, 18),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else if (g->strength >= 85) {
      graphics_context_set_text_color(ctx, C_LOCK);
      graphics_draw_text(ctx, "LOCK — FIRE!", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                         GRect(2, bounds.size.h - 42, bounds.size.w - 4, 22),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    } else {
      snprintf(line, sizeof(line), "Hdg %03d  El %+d",
               (int)aim.heading_deg, (int)aim.elev_deg);
      graphics_draw_text(ctx, line, fonts_get_system_font(FONT_KEY_GOTHIC_14),
                         GRect(2, bounds.size.h - 40, bounds.size.w - 4, 18),
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
  }

  if (g->state == GameState_GameOver) {
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "SELECT: again", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(2, bounds.size.h - 22, bounds.size.w - 4, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else if (g->state == GameState_WaveClear) {
    graphics_context_set_text_color(ctx, C_DIM);
    graphics_draw_text(ctx, "SELECT: next wave", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                       GRect(2, bounds.size.h - 22, bounds.size.w - 4, 18),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

static void do_fire(void) {
  game_fire();
  ui_mark_dirty();
}

static void select_click(ClickRecognizerRef recognizer, void *context) {
  (void)recognizer;
  (void)context;
  do_fire();
}

static void click_config(void *context) {
  (void)context;
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
  window_single_click_subscribe(BUTTON_ID_UP, select_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, select_click);
}

#if defined(PBL_TOUCH)
static void on_tap(const Recognizer *recognizer, RecognizerEvent event) {
  (void)recognizer;
  if (event == RecognizerEvent_Completed) {
    do_fire();
  }
}
#endif

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  s_bmp_sub = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUB);
  s_bmp_reticle = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RETICLE);
  s_bmp_title = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TITLE);

#if defined(PBL_TOUCH)
  window_set_touch_bridge_disabled(window, true);
  if (touch_service_is_enabled()) {
    Recognizer *tap = tap_recognizer_create(on_tap, NULL);
    window_attach_recognizer(window, tap);
  }
#endif

  s_redraw = app_timer_register(100, redraw_cb, NULL);
}

static void window_unload(Window *window) {
  (void)window;
  if (s_redraw) {
    app_timer_cancel(s_redraw);
    s_redraw = NULL;
  }
  if (s_bmp_sub) { gbitmap_destroy(s_bmp_sub); s_bmp_sub = NULL; }
  if (s_bmp_reticle) { gbitmap_destroy(s_bmp_reticle); s_bmp_reticle = NULL; }
  if (s_bmp_title) { gbitmap_destroy(s_bmp_title); s_bmp_title = NULL; }
  layer_destroy(s_canvas);
  s_canvas = NULL;
}

void ui_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, C_BG);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_set_click_config_provider(s_window, click_config);
  window_stack_push(s_window, true);
}

void ui_deinit(void) {
  window_destroy(s_window);
  s_window = NULL;
}

void ui_mark_dirty(void) {
  if (s_canvas) layer_mark_dirty(s_canvas);
}
