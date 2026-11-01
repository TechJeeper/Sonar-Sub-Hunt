// Phone side: Clay settings → AppMessage FeedbackMode on the watch.

var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var messageKeys = require('message_keys');

var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

function feedbackInt(value) {
  var n = parseInt(value, 10);
  if (n === 1 || n === 2) return n;
  return 0; // Both
}

function pushSettings(settings) {
  settings = settings || clay.getSettings();
  var raw = settings[messageKeys.FeedbackMode];
  if (raw === undefined) raw = settings.FeedbackMode;
  Pebble.sendAppMessage({ FeedbackMode: feedbackInt(raw) });
}

Pebble.addEventListener('ready', function () {
  pushSettings();
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  pushSettings(clay.getSettings(e.response));
});
