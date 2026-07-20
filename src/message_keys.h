#pragma once
#include "platform_config.h"

// Command types (CMD key)
typedef enum {
  CMD_GET_PLAYER_STATE = 1,
  CMD_PLAY_PAUSE = 2,
  CMD_NEXT = 3,
  CMD_PREVIOUS = 4,
  CMD_SET_VOLUME = 5,
  CMD_SEARCH = 6,
  CMD_RANDOM = 7,
  CMD_ADD_TO_QUEUE = 8,
  CMD_GET_OUTPUTS = 9,
  CMD_SET_OUTPUT_EXCLUSIVE = 10,
  CMD_TOGGLE_OUTPUT = 11,
  CMD_SET_OUTPUT_VOLUME = 12,
  CMD_GET_FAVORITES = 13,
  CMD_PLAY = 14,
  CMD_PAUSE = 15,
  CMD_GET_QUEUE = 16,
  CMD_PLAY_QUEUE_ITEM = 17
} CommandType;

// Content types for search/random
typedef enum {
  CONTENT_TYPE_PLAYLIST = 0,
  CONTENT_TYPE_ARTIST = 1,
  CONTENT_TYPE_ALBUM = 2
} ContentType;

// Message keys (must match appinfo.json)
typedef enum {
  KEY_CMD = 0,
  KEY_TYPE = 1,
  KEY_QUERY = 2,
  KEY_URI = 3,
  KEY_VOLUME = 4,
  KEY_OUTPUT_ID = 5,
  
  KEY_RESULT_COUNT = 10,
  KEY_RESULT_TITLE_BASE = 20,
  KEY_RESULT_URI_BASE = 30,
  
  KEY_PLAYER_STATE = 40,
  KEY_PLAYER_TRACK = 41,
  KEY_PLAYER_ARTIST = 42,
  KEY_PLAYER_ALBUM = 43,
  KEY_PLAYER_VOLUME = 44,
  
  KEY_OUTPUT_COUNT = 50,
  KEY_OUTPUT_NAME_BASE = 60,
  KEY_OUTPUT_ID_BASE = 70,
  KEY_OUTPUT_VOLUME_BASE = 80,
  KEY_OUTPUT_ENABLED_BASE = 90,
  
  KEY_STATUS = 100,
  
  KEY_FAVORITE_COUNT = 110,
  KEY_FAVORITE_NAME_BASE = 120,
  KEY_FAVORITE_TYPE_BASE = 130,
  
  KEY_QUEUE_COUNT = 140,
  KEY_QUEUE_SELECTED = 141,
  KEY_QUEUE_TITLE_BASE = 150,
  KEY_QUEUE_ARTIST_BASE = 160,
  KEY_QUEUE_ITEM_ID_BASE = 170,
  
  KEY_QUEUE_ITEM_ID = 180,
  KEY_PLAYER_AUTO_CLOSE_TIMEOUT = 190,
  KEY_APP_AUTO_CLOSE_TIMEOUT = 191
  ,
  KEY_VIBRATION = 192
} MessageKey;

// Vibration settings sent from the config page (wire values)
// 1 = Default (lighter), 2 = Strong, 3 = Off
typedef enum {
  VIBRATION_DEFAULT = 1,
  VIBRATION_STRONG  = 2,
  VIBRATION_OFF     = 3
} VibrationSetting;

// Player states
typedef enum {
  PLAYER_STATE_STOPPED = 0,
  PLAYER_STATE_PLAYING = 1,
  PLAYER_STATE_PAUSED = 2
} PlayerState;

// Favorites key layout helpers.
// Wire values intentionally kept as-is to avoid breaking the JS bridge.
// Names 0-9  : KEY_FAVORITE_NAME_BASE + i          (120-129)
// Types 0-9  : KEY_FAVORITE_TYPE_BASE + i          (130-139)
// Names 10-29: KEY_FAVORITE_NAME_BASE + 20 + (i-10) (140-159)
// Types 10-29: KEY_FAVORITE_TYPE_BASE + 30 + (i-10) (160-179)
static inline int favorite_name_key(int i) {
  return (i < 10) ? (KEY_FAVORITE_NAME_BASE + i)
                  : (KEY_FAVORITE_NAME_BASE + 20 + (i - 10));
}
static inline int favorite_type_key(int i) {
  return (i < 10) ? (KEY_FAVORITE_TYPE_BASE + i)
                  : (KEY_FAVORITE_TYPE_BASE + 30 + (i - 10));
}
