// 13519214
// Engine configuration
// WARNING : Only allowed for engine.cpp to reduce coupling
// Common preprocessor
#ifndef _CONFIG
#define _CONFIG

// Target macro
#define CLEAR_SCREEN_CMD "cls"

// Common configuration
#define INPUT_BUFFER_COUNT 4
#define INPUT_DELAY_MS 100

// Map
#define SEA_STARTING_X 35
#define SEA_STARTING_Y 15

// Render configuration
// Use 2:1, x:y ratio for square-y map
#define MAP_MAX_X 50
#define MAP_MAX_Y 25

#define MSG_MAX_X 40
#define MAX_MESSAGE 5 // TODO : Maybe not needed

// NOTE : Render coordinate (0, 0) starting from upper left corner
#define MAP_OFFSET_X 1
#define MAP_OFFSET_Y 1
#define MESSAGE_OFFSET_X MAP_OFFSET_X+MAP_MAX_X+5
#define MESSAGE_OFFSET_Y MAP_OFFSET_Y+1

#define CURSOR_REST_X 60
#define CURSOR_REST_Y 20

// skillDatabse
#define MAX_SKILL_LIST 100
#endif
