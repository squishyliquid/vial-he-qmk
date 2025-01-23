#include "analog.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "matrix.h"
#include "quantum.h"
#include "hall_effect.h"

#ifdef SPLIT_KEYBOARD
#    include "split_common/split_util.h"
#    include "split_common/transactions.h"

#    define ROWS_PER_HAND (MATRIX_ROWS / 2)
uint8_t thisHand, thatHand;
#else
#    define ROWS_PER_HAND (MATRIX_ROWS)
#endif

static const pin_t row_pins[ROWS_PER_HAND] = MATRIX_ROW_PINS;
static const pin_t mux_pins[MUX_BITS] = ALL_MUX_PINS;

matrix_row_t matrix[MATRIX_ROWS];
analog_key_t keys[ROWS_PER_HAND][MATRIX_COLS];

#if defined(DEBUG_MATRIX_SCAN_RATE)
static uint32_t matrix_timer = 0;
#endif

#ifdef SPLIT_KEYBOARD
bool matrix_post_scan(void) {
    bool changed = false;
    if (is_keyboard_master()) {
        static bool  last_connected              = false;
        matrix_row_t slave_matrix[ROWS_PER_HAND] = {0};
        if (transport_master_if_connected(matrix + thisHand, slave_matrix)) {
            changed = memcmp(matrix + thatHand, slave_matrix, sizeof(slave_matrix)) != 0;

            last_connected = true;
        } else if (last_connected) {
            // reset other half when disconnected
            memset(slave_matrix, 0, sizeof(slave_matrix));
            changed = true;

            last_connected = false;
        }

        if (changed) memcpy(matrix + thatHand, slave_matrix, sizeof(slave_matrix));

        matrix_scan_kb();
    } else {
        transport_slave(matrix + thatHand, matrix + thisHand);

        matrix_slave_scan_kb();
    }

    return changed;
}

__attribute__((weak)) void matrix_slave_scan_kb(void) {
    matrix_slave_scan_user();
}
__attribute__((weak)) void matrix_slave_scan_user(void) {}
#endif

__attribute__((weak)) void matrix_init_kb(void) { matrix_init_user(); }

__attribute__((weak)) void matrix_scan_kb(void) { matrix_scan_user(); }

__attribute__((weak)) void matrix_init_user(void) {}

__attribute__((weak)) void matrix_scan_user(void) {}

matrix_row_t matrix_get_row(uint8_t row) {
    return matrix[row];
}

inline bool matrix_is_on(uint8_t row, uint8_t col) {
    return (matrix[row] & ((matrix_row_t)1 << col));
}

void matrix_print(void) {
    //
}

static inline void gpio_atomic_set_pin_output_low(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_output(pin);
        gpio_write_pin_low(pin);
    }
}

static inline void gpio_atomic_set_pin_output_high(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        gpio_set_pin_output(pin);
        gpio_write_pin_high(pin);
    }
}

void bootmagic_scan(void) {
    uint8_t row = BOOTMAGIC_ROW;
    uint8_t col = BOOTMAGIC_COLUMN;

    uint8_t row2 = BOOTMAGIC_ROW_2;
    uint8_t col2 = BOOTMAGIC_COLUMN_2;

#if defined(SPLIT_KEYBOARD) && defined(BOOTMAGIC_ROW_RIGHT) && defined(BOOTMAGIC_COLUMN_RIGHT)
    if (!is_keyboard_left()) {
        row = BOOTMAGIC_ROW_RIGHT - thisHand;
        col = BOOTMAGIC_COLUMN_RIGHT;

        row2 = BOOTMAGIC_ROW_RIGHT_2 - thisHand;
        col2 = BOOTMAGIC_COLUMN_RIGHT_2;
    }
#endif
    ( col       & 1) ? gpio_atomic_set_pin_output_high(mux_pins[0]) : gpio_atomic_set_pin_output_low(mux_pins[0]);
    ((col >> 1) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[1]) : gpio_atomic_set_pin_output_low(mux_pins[1]);
    ((col >> 2) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[2]) : gpio_atomic_set_pin_output_low(mux_pins[2]);
    ((col >> 3) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[3]) : gpio_atomic_set_pin_output_low(mux_pins[3]);
    wait_us(10);

    uint16_t pin_1 = analogReadPin(row_pins[row]);

    ( col2       & 1) ? gpio_atomic_set_pin_output_high(mux_pins[0]) : gpio_atomic_set_pin_output_low(mux_pins[0]);
    ((col2 >> 1) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[1]) : gpio_atomic_set_pin_output_low(mux_pins[1]);
    ((col2 >> 2) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[2]) : gpio_atomic_set_pin_output_low(mux_pins[2]);
    ((col2 >> 3) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[3]) : gpio_atomic_set_pin_output_low(mux_pins[3]);
    wait_us(10);

    uint16_t pin_2 = analogReadPin(row_pins[row2]);

    if (pin_1 + pin_2 < 2900) {
        // Jump to bootloader.
        bootloader_jump();
    }
}

void initialise_hall_sensors(void) {
    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            // Select MUX channel
            ( c       & 1) ? gpio_atomic_set_pin_output_high(mux_pins[0]) : gpio_atomic_set_pin_output_low(mux_pins[0]);
            ((c >> 1) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[1]) : gpio_atomic_set_pin_output_low(mux_pins[1]);
            ((c >> 2) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[2]) : gpio_atomic_set_pin_output_low(mux_pins[2]);
            ((c >> 3) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[3]) : gpio_atomic_set_pin_output_low(mux_pins[3]);
            wait_us(10);
            
            for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
                if (is_keyboard_left()) {
                    if ((r == 0 && (c == 0 || c == 7 || c == 12)) ||
                        (r == 1 && (c == 11 || c == 12)) ||
                        (r == 2 && c == 0)) {
                        continue;
                    }
                } else if (c == 12 && (r == 0 || r == 1)) {
                    continue;
                }

                uint16_t analogValue = analogReadPin(row_pins[r]);

                if (i == 2) {
                    keys[r][c].dynamic_actuation = false;
                    keys[r][c].curr_pos = 0;
                    keys[r][c].prev_pos = 0;
                    
                    uint16_t offset = analogValue / 200;
                    uint16_t offset_r = analogValue % 200;

                    if (offset_r > 100)
                        offset += 1;

                    //analogValue -= offset;
                    keys[r][c].max_value = analogValue - offset;

                    if (analogValue >= 2100) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 95 + analogValue % 100 * 95 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 92 + analogValue % 100 * 92 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 87 + analogValue % 100 * 87 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 81 + analogValue % 100 * 81 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 72 + analogValue % 100 * 72 / 100;
                        keys[r][c].min_value = analogValue / 100 * 72 + analogValue % 100 * 72 / 100 - analogValue / 200;

                    } else if (analogValue >= 2000) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 95 + analogValue % 100 * 95 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 91 + analogValue % 100 * 91 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 86 + analogValue % 100 * 86 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 78 + analogValue % 100 * 78 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 68 + analogValue % 100 * 68 / 100;
                        keys[r][c].min_value = analogValue / 100 * 68 + analogValue % 100 * 68 / 100 - analogValue / 200;

                    } else if (analogValue >= 1900) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 95 + analogValue % 100 * 95 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 90 + analogValue % 100 * 90 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 85 + analogValue % 100 * 85 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 77 + analogValue % 100 * 77 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 66 + analogValue % 100 * 66 / 100;
                        keys[r][c].min_value = analogValue / 100 * 66 + analogValue % 100 * 66 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1800) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 90 + analogValue % 100 * 90 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 84 + analogValue % 100 * 84 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 75 + analogValue % 100 * 75 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 64 + analogValue % 100 * 64 / 100;
                        keys[r][c].min_value = analogValue / 100 * 64 + analogValue % 100 * 64 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1700) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 84 + analogValue % 100 * 84 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 75 + analogValue % 100 * 75 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 66 + analogValue % 100 * 66 / 100;
                        keys[r][c].min_value = analogValue / 100 * 66 + analogValue % 100 * 66 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1650) {
                        keys[r][c].value_05 = analogValue / 100 * 97 + analogValue % 100 * 97 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 83 + analogValue % 100 * 83 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 74 + analogValue % 100 * 74 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 68 + analogValue % 100 * 68 / 100;
                        keys[r][c].min_value = analogValue / 100 * 68 + analogValue % 100 * 68 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1600) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 90 + analogValue % 100 * 90 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 83 + analogValue % 100 * 83 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 75 + analogValue % 100 * 75 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 70 + analogValue % 100 * 70 / 100;
                        keys[r][c].min_value = analogValue / 100 * 70 + analogValue % 100 * 70 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1550) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 82 + analogValue % 100 * 82 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 75 + analogValue % 100 * 75 / 100;
                        keys[r][c].value_30 = analogValue / 100 * 71 + analogValue % 100 * 71 / 100;
                        keys[r][c].min_value = analogValue / 100 * 71 + analogValue % 100 * 71 / 100 - analogValue / 200;
                        
                    } else if (analogValue >= 1500) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 82 + analogValue % 100 * 82 / 100;
                        keys[r][c].value_25 = analogValue / 100 * 75 + analogValue % 100 * 75 / 100;
                        keys[r][c].value_30 = 0;
                        keys[r][c].min_value = analogValue / 100 * 74 + analogValue % 100 * 74 / 100;
                        
                    } else if (analogValue >= 1400) {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = analogValue / 100 * 82 + analogValue % 100 * 82 / 100;
                        keys[r][c].value_25 = 0;
                        keys[r][c].value_30 = 0;
                        keys[r][c].min_value = analogValue / 100 * 78 + analogValue % 100 * 78 / 100;
                        
                    } else {
                        keys[r][c].value_05 = analogValue / 100 * 98 + analogValue % 100 * 98 / 100;
                        keys[r][c].value_10 = analogValue / 100 * 94 + analogValue % 100 * 94 / 100;
                        keys[r][c].value_15 = analogValue / 100 * 89 + analogValue % 100 * 89 / 100;
                        keys[r][c].value_20 = 0;
                        keys[r][c].value_25 = 0;
                        keys[r][c].value_30 = 0;
                        keys[r][c].min_value = analogValue / 100 * 85 + analogValue % 100 * 85 / 100;
                        
                    }
                }
            }
        }
        wait_ms(100);
    }
}

void matrix_init(void) {

#ifdef SPLIT_KEYBOARD
    thisHand = isLeftHand ? 0 : (ROWS_PER_HAND);
    thatHand = ROWS_PER_HAND - thisHand;
#endif

    memset(matrix, 0, sizeof(matrix));
    initialise_hall_sensors();

    // This *must* be called for correct keyboard behavior
    matrix_init_kb();
}

uint8_t matrix_scan(void) {

    matrix_row_t curr_matrix[ROWS_PER_HAND] = {0};

    #ifdef SPLIT_KEYBOARD
    memcpy(curr_matrix, matrix + thisHand, sizeof(matrix_row_t) * ROWS_PER_HAND);
    #else
    memcpy(curr_matrix, matrix, sizeof(matrix));
    #endif
    
    for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
        // Select MUX line for each col
        ( col_index       & 1) ? gpio_atomic_set_pin_output_high(mux_pins[0]) : gpio_atomic_set_pin_output_low(mux_pins[0]);
        ((col_index >> 1) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[1]) : gpio_atomic_set_pin_output_low(mux_pins[1]);
        ((col_index >> 2) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[2]) : gpio_atomic_set_pin_output_low(mux_pins[2]);
        ((col_index >> 3) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[3]) : gpio_atomic_set_pin_output_low(mux_pins[3]);
        wait_us(10);
        
        for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
            if (is_keyboard_left()) {
                if ((row_index == 0 && (col_index == 0 || col_index == 7 || col_index == 12)) ||
                    (row_index == 1 && (col_index == 11 || col_index == 12)) ||
                    (row_index == 2 && col_index == 0)) {
                    continue;
                }
            } else if (col_index == 12 && (row_index == 0 || row_index == 1)) {
                continue;
            }

            analog_key_t *key = &keys[row_index][col_index];
            uint16_t analogValue = analogReadPin(row_pins[row_index]);

            key_config_t *config = &user_config.key_config[row_index + thisHand][col_index]; 
            
            #if defined(DEBUG_MATRIX_SCAN_RATE)
            key->test_value = analogValue;
            #endif

            if (analogValue < key->min_value - MIN_MAX_BUFFER)
                key->min_value = analogValue;
            else
                analogValue = MIN(MAX(key->min_value, analogValue), key->max_value);

            // Calculate current position
            uint16_t upper_limit;
            uint16_t lower_limit;
            uint16_t curr_value;
            uint16_t travel_offset = 0;
            uint16_t travel_range = 50;

            if (key->max_value >= 1550) {
                if (analogValue > key->value_05) {
                    curr_value = key->max_value - analogValue;
                    upper_limit = key->max_value;
                    lower_limit = key->value_05;
                } else if (analogValue > key->value_10) {
                    curr_value = key->value_05 - analogValue;
                    upper_limit = key->value_05;
                    lower_limit = key->value_10;
                    travel_offset = 50;
                } else if (analogValue > key->value_15) {
                    curr_value = key->value_10 - analogValue;
                    upper_limit = key->value_10;
                    lower_limit = key->value_15;
                    travel_offset = 100;
                } else if (analogValue > key->value_20) {
                    curr_value = key->value_15 - analogValue;
                    upper_limit = key->value_15;
                    lower_limit = key->value_20;
                    travel_offset = 150;
                } else if (analogValue > key->value_25) {
                    curr_value = key->value_20 - analogValue;
                    upper_limit = key->value_20;
                    lower_limit = key->value_25;
                    travel_offset = 200;
                } else if (analogValue > key->value_30) {
                    curr_value = key->value_25 - analogValue;
                    upper_limit = key->value_25;
                    lower_limit = key->value_30;
                    travel_offset = 250;
                } else {
                    curr_value = key->value_30 - analogValue;
                    upper_limit = key->value_30;
                    lower_limit = key->min_value;
                    travel_offset = 300;
                }
            } else if (key->max_value >= 1500) {
                if (analogValue > key->value_05) {
                    curr_value = key->max_value - analogValue;
                    upper_limit = key->max_value;
                    lower_limit = key->value_05;
                } else if (analogValue > key->value_10) {
                    curr_value = key->value_05 - analogValue;
                    upper_limit = key->value_05;
                    lower_limit = key->value_10;
                    travel_offset = 50;
                } else if (analogValue > key->value_15) {
                    curr_value = key->value_10 - analogValue;
                    upper_limit = key->value_10;
                    lower_limit = key->value_15;
                    travel_offset = 100;
                } else if (analogValue > key->value_20) {
                    curr_value = key->value_15 - analogValue;
                    upper_limit = key->value_15;
                    lower_limit = key->value_20;
                    travel_offset = 150;
                } else if (analogValue > key->value_25) {
                    curr_value = key->value_20 - analogValue;
                    upper_limit = key->value_20;
                    lower_limit = key->value_25;
                    travel_offset = 200;
                } else {
                    curr_value = key->value_25 - analogValue;
                    upper_limit = key->value_25;
                    lower_limit = key->min_value;
                    travel_offset = 250;
                    travel_range = 100;
                }
            } else if (key->max_value >= 1400) {
                if (analogValue > key->value_05) {
                    curr_value = key->max_value - analogValue;
                    upper_limit = key->max_value;
                    lower_limit = key->value_05;
                } else if (analogValue > key->value_10) {
                    curr_value = key->value_05 - analogValue;
                    upper_limit = key->value_05;
                    lower_limit = key->value_10;
                    travel_offset = 50;
                } else if (analogValue > key->value_15) {
                    curr_value = key->value_10 - analogValue;
                    upper_limit = key->value_10;
                    lower_limit = key->value_15;
                    travel_offset = 100;
                } else if (analogValue > key->value_20) {
                    curr_value = key->value_15 - analogValue;
                    upper_limit = key->value_15;
                    lower_limit = key->value_20;
                    travel_offset = 150;
                } else {
                    curr_value = key->value_20 - analogValue;
                    upper_limit = key->value_20;
                    lower_limit = key->min_value;
                    travel_offset = 200;
                    travel_range = 150;
                }
            } else {
                if (analogValue > key->value_05) {
                    curr_value = key->max_value - analogValue;
                    upper_limit = key->max_value;
                    lower_limit = key->value_05;
                } else if (analogValue > key->value_10) {
                    curr_value = key->value_05 - analogValue;
                    upper_limit = key->value_05;
                    lower_limit = key->value_10;
                    travel_offset = 50;
                } else if (analogValue > key->value_15) {
                    curr_value = key->value_10 - analogValue;
                    upper_limit = key->value_10;
                    lower_limit = key->value_15;
                    travel_offset = 100;
                } else {
                    curr_value = key->value_15 - analogValue;
                    upper_limit = key->value_15;
                    lower_limit = key->min_value;
                    travel_offset = 150;
                    travel_range = 200;
                }
            }

            key->curr_pos = travel_range * (curr_value) / (upper_limit - lower_limit) + travel_offset;
            uint16_t curr_pos_r = travel_range * (curr_value) % (upper_limit - lower_limit);

            if (curr_pos_r > (upper_limit - lower_limit) >> 1)
                key->curr_pos += 1;

            uint16_t reset_point = config->actuation_point - 10;
            if (config->mode == 2) {
                reset_point = 10;
                if (config->actuation_point == 10) {
                    reset_point = 0;
                }
            }

            // Update key states
            if (config->mode != 0) {
                // Rapid Trigger mode enabled
                if (key->dynamic_actuation) {
                    if (curr_matrix[row_index] & (1 << col_index)) {
                        // Key is 'pressed'
                        if (key->curr_pos > key->prev_pos) {
                            key->prev_pos = key->curr_pos;
                        } else if (key->curr_pos < key->prev_pos - user_config.sensitivity) {
                            curr_matrix[row_index] &= ~(1 << col_index);
                            key->prev_pos = key->curr_pos;
                        }
                    } else {
                        // Key is 'not pressed'
                        if (key->curr_pos < key->prev_pos) {
                            key->prev_pos = key->curr_pos;
                        } else if (key->curr_pos > key->prev_pos + user_config.sensitivity) {
                            curr_matrix[row_index] |= (1 << col_index);
                            key->prev_pos = key->curr_pos;
                        }
                    }
                    if (key->curr_pos <= reset_point) {
                        // Key is above reset point
                        curr_matrix[row_index] &= ~(1 << col_index);
                        key->prev_pos = key->curr_pos;
                        key->dynamic_actuation = false;
                    } 
                } else if (key->curr_pos > config->actuation_point) {
                    curr_matrix[row_index] |= (1 << col_index);
                    key->prev_pos = key->curr_pos;
                    key->dynamic_actuation = true;
                }
            } else {
                // Rapid Trigger mode disabled
                if (curr_matrix[row_index] & (1 << col_index)) {
                    // Key is 'pressed'
                    if (key->curr_pos < config->actuation_point - 10) {
                        curr_matrix[row_index] &= ~(1 << col_index);
                    }
                } else {
                    // Key is 'not pressed'
                    if (key->curr_pos > config->actuation_point) {
                        curr_matrix[row_index] |= (1 << col_index);
                    }
                }
            }
        }
    }
    
    #if defined(DEBUG_MATRIX_SCAN_RATE)
    uint32_t timer_now = timer_read32();
    
    if (TIMER_DIFF_32(timer_now, matrix_timer) >= 500) {
        
        uprintf("matrix scan rate: %lu\n", get_matrix_scan_rate());
        if (is_keyboard_left()) {
            uprintf("(%u, %u) ", keys[0][3].max_value, keys[0][3].test_value);
            uprintf("(%u, %u) ", keys[0][2].max_value, keys[0][2].test_value);
            uprintf("(%u, %u) ", keys[0][1].max_value, keys[0][1].test_value);
            uprintf("(%u, %u) ", keys[0][11].max_value, keys[0][11].test_value);
            uprintf("(%u, %u) ", keys[0][10].max_value, keys[0][10].test_value);
            uprintf("(%u, %u) ", keys[0][9].max_value, keys[0][9].test_value);
            uprintf("(%u, %u)\n", keys[0][8].max_value, keys[0][8].test_value);

            uprintf("(%u, %u) ", keys[0][4].max_value, keys[0][4].test_value);
            uprintf("(%u, %u) ", keys[0][5].max_value, keys[0][5].test_value);
            uprintf("(%u, %u) ", keys[0][6].max_value, keys[0][6].test_value);
            uprintf("(%u, %u) ", keys[1][7].max_value, keys[1][7].test_value);
            uprintf("(%u, %u) ", keys[1][6].max_value, keys[1][6].test_value);
            uprintf("(%u, %u) ", keys[1][5].max_value, keys[1][5].test_value);
            uprintf("(%u, %u)\n", keys[1][4].max_value, keys[1][4].test_value);

            uprintf("(%u, %u) ", keys[1][8].max_value, keys[1][8].test_value);
            uprintf("(%u, %u) ", keys[1][9].max_value, keys[1][9].test_value);
            uprintf("(%u, %u) ", keys[1][10].max_value, keys[1][10].test_value);
            uprintf("(%u, %u) ", keys[1][0].max_value, keys[1][0].test_value);
            uprintf("(%u, %u) ", keys[1][1].max_value, keys[1][1].test_value);
            uprintf("(%u, %u) ", keys[1][2].max_value, keys[1][2].test_value);
            uprintf("(%u, %u)\n", keys[1][3].max_value, keys[1][3].test_value);

            uprintf("(%u, %u) ", keys[2][10].max_value, keys[2][10].test_value);
            uprintf("(%u, %u) ", keys[2][9].max_value, keys[2][9].test_value);
            uprintf("(%u, %u) ", keys[2][8].max_value, keys[2][8].test_value);
            uprintf("(%u, %u) ", keys[2][7].max_value, keys[2][7].test_value);
            uprintf("(%u, %u) ", keys[2][6].max_value, keys[2][6].test_value);
            uprintf("(%u, %u) ", keys[2][5].max_value, keys[2][5].test_value);
            uprintf("(%u, %u)\n", keys[2][4].max_value, keys[2][4].test_value);

            uprintf("(%u, %u) ", keys[2][11].max_value, keys[2][11].test_value);
            uprintf("(%u, %u) ", keys[2][12].max_value, keys[2][12].test_value);
            uprintf("(%u, %u) ", keys[2][1].max_value, keys[2][1].test_value);
            uprintf("(%u, %u) ", keys[2][2].max_value, keys[2][2].test_value);
            uprintf("(%u, %u)\n\n", keys[2][3].max_value, keys[2][3].test_value);
        } else {
            uprintf("(%u, %u) ", keys[0][3].max_value, keys[0][3].test_value);
            uprintf("(%u, %u) ", keys[0][2].max_value, keys[0][2].test_value);
            uprintf("(%u, %u) ", keys[0][1].max_value, keys[0][1].test_value);
            uprintf("(%u, %u) ", keys[0][0].max_value, keys[0][0].test_value);
            uprintf("(%u, %u) ", keys[0][11].max_value, keys[0][11].test_value);
            uprintf("(%u, %u) ", keys[0][10].max_value, keys[0][10].test_value);
            uprintf("(%u, %u) ", keys[0][9].max_value, keys[0][9].test_value);
            uprintf("(%u, %u)\n", keys[0][8].max_value, keys[0][8].test_value);

            uprintf("(%u, %u) ", keys[0][4].max_value, keys[0][4].test_value);
            uprintf("(%u, %u) ", keys[0][5].max_value, keys[0][5].test_value);
            uprintf("(%u, %u) ", keys[0][6].max_value, keys[0][6].test_value);
            uprintf("(%u, %u) ", keys[0][7].max_value, keys[0][7].test_value);
            uprintf("(%u, %u) ", keys[1][7].max_value, keys[1][7].test_value);
            uprintf("(%u, %u) ", keys[1][6].max_value, keys[1][6].test_value);
            uprintf("(%u, %u) ", keys[1][5].max_value, keys[1][5].test_value);
            uprintf("(%u, %u)\n", keys[1][4].max_value, keys[1][4].test_value);

            uprintf("(%u, %u) ", keys[1][8].max_value, keys[1][8].test_value);
            uprintf("(%u, %u) ", keys[1][10].max_value, keys[1][10].test_value);
            uprintf("(%u, %u) ", keys[1][11].max_value, keys[1][11].test_value);
            uprintf("(%u, %u) ", keys[1][0].max_value, keys[1][0].test_value);
            uprintf("(%u, %u) ", keys[1][1].max_value, keys[1][1].test_value);
            uprintf("(%u, %u) ", keys[1][2].max_value, keys[1][2].test_value);
            uprintf("(%u, %u)\n", keys[1][3].max_value, keys[1][3].test_value);

            uprintf("(%u, %u) ", keys[1][9].max_value, keys[1][9].test_value);
            uprintf("(%u, %u) ", keys[2][3].max_value, keys[2][3].test_value);
            uprintf("(%u, %u) ", keys[2][2].max_value, keys[2][2].test_value);
            uprintf("(%u, %u) ", keys[2][1].max_value, keys[2][1].test_value);
            uprintf("(%u, %u) ", keys[2][0].max_value, keys[2][0].test_value);
            uprintf("(%u, %u) ", keys[2][12].max_value, keys[2][12].test_value);
            uprintf("(%u, %u)\n", keys[2][11].max_value, keys[2][11].test_value);

            uprintf("(%u, %u) ", keys[2][4].max_value, keys[2][4].test_value);
            uprintf("(%u, %u) ", keys[2][5].max_value, keys[2][5].test_value);
            uprintf("(%u, %u) ", keys[2][6].max_value, keys[2][6].test_value);
            uprintf("(%u, %u) ", keys[2][7].max_value, keys[2][7].test_value);
            uprintf("(%u, %u) ", keys[2][8].max_value, keys[2][8].test_value);
            uprintf("(%u, %u) ", keys[2][9].max_value, keys[2][9].test_value);
            uprintf("(%u, %u)\n\n", keys[2][10].max_value, keys[2][10].test_value);
        }

        matrix_timer = timer_now;
    }
    #endif

    bool changed;

#ifdef SPLIT_KEYBOARD
    changed = memcmp(matrix + thisHand, curr_matrix, sizeof(curr_matrix)) != 0;
    if (changed) memcpy(matrix + thisHand, curr_matrix, sizeof(curr_matrix));
    matrix_post_scan();
#else
    changed = memcmp(matrix, curr_matrix, sizeof(curr_matrix)) != 0;
    if (changed) memcpy(matrix, curr_matrix, sizeof(curr_matrix));
    // This *must* be called for correct keyboard behavior
    matrix_scan_kb();
#endif

    return changed;
}