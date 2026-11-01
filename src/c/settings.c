#include "settings.h"
#include "sonar.h"

#include <stdlib.h>

static int32_t tuple_to_mode(Tuple *t) {
  if (!t) return -1;
  if (t->type == TUPLE_INT) return t->value->int32;
  if (t->type == TUPLE_UINT) return (int32_t)t->value->uint32;
  if (t->type == TUPLE_CSTRING) {
    return (int32_t)atoi(t->value->cstring);
  }
  return -1;
}

static void inbox_received(DictionaryIterator *it, void *context) {
  (void)context;
  Tuple *t = dict_find(it, MESSAGE_KEY_FeedbackMode);
  if (!t) return;

  int32_t mode = tuple_to_mode(t);
  if (mode < 0) return;
  sonar_set_feedback_mode((SonarFeedbackMode)mode);
}

void settings_init(void) {
  app_message_register_inbox_received(inbox_received);
  app_message_open(128, 64);
}

void settings_deinit(void) {
}
