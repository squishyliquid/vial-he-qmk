#pragma once
#include <inttypes.h>
#include <stdbool.h>

#define MIN_MAX_BUFFER 5

// #define RESET_POINT 0

#ifndef RAPID_TRIGGER_MODE
#   define RAPID_TRIGGER_MODE 1
#endif

#ifndef TRAVEL_DISTANCE
#   define TRAVEL_DISTANCE 350
#endif

#ifndef ACTUATION_POINT
#   define ACTUATION_POINT 150
#endif

#ifndef SENSITIVITY
#   define SENSITIVITY 20
#endif

typedef struct {
    uint16_t actuation_point;
    uint8_t mode; //0 = no rapid trigger, 1 = rapid trigger, 2 = continuous rapid trigger
} key_config_t;

typedef struct {
    uint16_t travel_distance;
    uint16_t sensitivity;
    key_config_t key_config[MATRIX_ROWS][MATRIX_COLS];
} user_config_t;

extern user_config_t user_config;

typedef struct {
    bool dynamic_actuation;
    uint16_t curr_pos;
    uint16_t prev_pos;
    uint16_t max_value;
    uint16_t min_value;
    uint16_t value_05;
    uint16_t value_10;
    uint16_t value_15;
    uint16_t value_20;
    uint16_t value_25;
    uint16_t value_30;
    #if defined(DEBUG_MATRIX_SCAN_RATE)
    uint16_t test_value;
    #endif
} analog_key_t;