#pragma once
#include <inttypes.h>
#include <stdbool.h>

#define MIN_MAX_BUFFER 5

#define RESET_POINT 10

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
    uint8_t mode;
    uint8_t sensitivity;
    uint16_t travel_distance;
    uint16_t actuation_point;
} hall_effect_t;
_Static_assert(sizeof(hall_effect_t) == 6, "Unexpected size of the hall_effect_t structure");

extern hall_effect_t key_settings;

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