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

// static uint8_t filter_index = 0;

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
    for (uint8_t a = 0; a < 4; a++) {
        for (uint8_t i = 0; i < FILTER_SIZE; i++) {
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                // Select MUX channel
                ( c       & 1) ? gpio_atomic_set_pin_output_high(mux_pins[0]) : gpio_atomic_set_pin_output_low(mux_pins[0]);
                ((c >> 1) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[1]) : gpio_atomic_set_pin_output_low(mux_pins[1]);
                ((c >> 2) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[2]) : gpio_atomic_set_pin_output_low(mux_pins[2]);
                ((c >> 3) & 1) ? gpio_atomic_set_pin_output_high(mux_pins[3]) : gpio_atomic_set_pin_output_low(mux_pins[3]);
                wait_us(10);
                
                for (uint8_t r = 0; r < ROWS_PER_HAND; r++) {
                    uint16_t analogValue = analogReadPin(row_pins[r]);

                    if (keys[r][c].sma_filter[i] == 0) {
                        keys[r][c].sma_sum += analogValue;
                        keys[r][c].sma_filter[i] = analogValue;
                    } else {
                        keys[r][c].sma_sum -= keys[r][c].sma_filter[i];
                        keys[r][c].sma_filter[i] = analogValue;
                        keys[r][c].sma_sum += analogValue; 

                        uint16_t analog_r = keys[r][c].sma_sum % FILTER_SIZE;
                        if (analog_r > FILTER_SIZE >> 1) {
                            analogValue = (keys[r][c].sma_sum + (FILTER_SIZE >> 1)) >> FILTER_BITS;
                        } else {
                            analogValue = (keys[r][c].sma_sum) >> FILTER_BITS;
                        }
                    }

                    keys[r][c].dynamic_actuation = false;
                    keys[r][c].curr_pos = 0;
                    keys[r][c].prev_pos = 0;
                    
                    uint16_t offset = analogValue / 100;
                    uint16_t offset_r = analogValue % 100;

                    if (offset_r > 50)
                        offset += 1;

                    offset = offset * 15;

                    keys[r][c].max_value = analogValue + offset;
                    keys[r][c].min_value = analogValue + 3;

                }
            }
            wait_ms(5);
        }
        wait_ms(5);
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
            analog_key_t *key = &keys[row_index][col_index];
            uint16_t analogValue = analogReadPin(row_pins[row_index]);

            key_config_t *config = &user_config.key_config[row_index + thisHand][col_index]; 
            
            // key->sma_sum -= key->sma_filter[filter_index];
            // key->sma_filter[filter_index] = analogValue;
            // key->sma_sum += analogValue;

            // uint16_t analog_r = key->sma_sum % FILTER_SIZE;
            // if (analog_r > FILTER_SIZE >> 1) {
            //     analogValue = (key->sma_sum + (FILTER_SIZE >> 1)) >> FILTER_BITS;
            // } else {
            //     analogValue = (key->sma_sum) >> FILTER_BITS;
            // }

            #if defined(DEBUG_MATRIX_SCAN_RATE)
            key->test_value = analogValue;
            #endif

            if (analogValue > key->max_value + MIN_MAX_BUFFER)
                key->max_value = analogValue;
            else
                analogValue = MIN(MAX(key->min_value, analogValue), key->max_value);

            // Calculate current position

            uint16_t curr_value;

            curr_value = analogValue - key->min_value;
            
            uint16_t range = key->max_value - key->min_value;

            uint16_t norm_value;
            uint16_t norm_value_r = (uint32_t)(curr_value << 10) % range;

            if (norm_value_r > range >> 1) {
                norm_value = ((uint32_t)(curr_value << 10) + (range >> 1)) / range;
            } else {
                norm_value = (uint32_t)(curr_value << 10) / range;
            }

            switch(user_config.travel_distance) {
                case 340:
                    key->curr_pos = geon_raw_he[norm_value];
                    break;
                case 350:
                    key->curr_pos = gateron_magnetic_jade[norm_value];
                    break;
            }
            
            if (row_index == 0 || row_index == 2) {
                key->curr_pos = 0;
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
                    uint16_t reset_point = config->actuation_point - 10;
                    if (config->mode == 2) {
                        reset_point = 0;
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

    // filter_index = (filter_index + 1) >= FILTER_SIZE ? 0 : filter_index + 1;
    
    #if defined(DEBUG_MATRIX_SCAN_RATE)
    uint32_t timer_now = timer_read32();
    
    if (TIMER_DIFF_32(timer_now, matrix_timer) >= 500) {
        /*
        uprintf("matrix scan rate: %lu\n", get_matrix_scan_rate());
        
        for (uint8_t row_index = 0; row_index < ROWS_PER_HAND; row_index++) {
            // for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
            //     uprintf("[(%u,%u): %u] ", row_index, col_index, keys[row_index][col_index].test_value);
            // }

            uprintf("[");
            for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
                uprintf("(%u, %u) ", keys[row_index][col_index].max_value, keys[row_index][col_index].test_value);
            }
            uprintf("]");
            
            // uprintf("[");
            // for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
            //     uprintf("(%u, %u, %u) ", keys[row_index][col_index].max_value, keys[row_index][col_index].test_value, keys[row_index][col_index].curr_pos);
            // }
            // uprintf("]");

            // uprintf("[");
            // for (uint8_t col_index = 0; col_index < MATRIX_COLS; col_index++) {
            //     uprintf("%u ", keys[row_index][col_index].curr_pos);
            // }
            // uprintf("]");
            
            uprintf("\n");
        }

        // uprintf("mode: %u\n", key_settings.mode);
        // uprintf("sensitivity: %u\n", key_settings.sensitivity);
        // uprintf("travel distance: %u\n", key_settings.travel_distance);
        // uprintf("actuation point: %u\n", key_settings.actuation_point);

        // uprintf("size: %u\n", sizeof(hall_effect_t));
        

        uprintf("(%u, %u) ", keys[0][8].max_value, keys[0][8].test_value);
        uprintf("(%u, %u) ", keys[0][7].max_value, keys[0][7].test_value);
        uprintf("(%u, %u) ", keys[0][6].max_value, keys[0][6].test_value);
        uprintf("(%u, %u) ", keys[0][5].max_value, keys[0][5].test_value);
        uprintf("(%u, %u) ", keys[0][4].max_value, keys[0][4].test_value);
        uprintf("(%u, %u)\n", keys[0][3].max_value, keys[0][3].test_value);

        uprintf("(%u, %u) ", keys[0][9].max_value, keys[0][9].test_value);
        uprintf("(%u, %u) ", keys[0][0].max_value, keys[0][0].test_value);
        uprintf("(%u, %u) ", keys[0][1].max_value, keys[0][1].test_value);
        uprintf("(%u, %u) ", keys[0][2].max_value, keys[0][2].test_value);
        uprintf("(%u, %u) ", keys[1][4].max_value, keys[1][4].test_value);
        uprintf("(%u, %u)\n", keys[1][2].max_value, keys[1][2].test_value);

        uprintf("(%u, %u) ", keys[1][8].max_value, keys[1][8].test_value);
        uprintf("(%u, %u) ", keys[1][7].max_value, keys[1][7].test_value);
        uprintf("(%u, %u) ", keys[1][6].max_value, keys[1][6].test_value);
        uprintf("(%u, %u) ", keys[1][5].max_value, keys[1][5].test_value);
        uprintf("(%u, %u) ", keys[1][3].max_value, keys[1][3].test_value);
        uprintf("(%u, %u)\n", keys[1][1].max_value, keys[1][1].test_value);

        uprintf("(%u, %u) ", keys[1][9].max_value, keys[1][9].test_value);
        uprintf("(%u, %u) ", keys[1][0].max_value, keys[1][0].test_value);
        uprintf("(%u, %u) ", keys[2][7].max_value, keys[2][7].test_value);
        uprintf("(%u, %u) ", keys[2][6].max_value, keys[2][6].test_value);
        uprintf("(%u, %u) ", keys[2][5].max_value, keys[2][5].test_value);
        uprintf("(%u, %u)\n", keys[2][4].max_value, keys[2][4].test_value);

        uprintf("(%u, %u) ", keys[2][8].max_value, keys[2][8].test_value);
        uprintf("(%u, %u) ", keys[2][9].max_value, keys[2][9].test_value);
        uprintf("(%u, %u) ", keys[2][0].max_value, keys[2][0].test_value);
        uprintf("(%u, %u) ", keys[2][1].max_value, keys[2][1].test_value);
        uprintf("(%u, %u) ", keys[2][2].max_value, keys[2][2].test_value);
        uprintf("(%u, %u)\n\n", keys[2][3].max_value, keys[2][3].test_value);
        */
        uprintf("matrix scan rate: %lu\n", get_matrix_scan_rate());
        // uprintf("(%u, %u, %u)\n", keys[1][4].curr_pos, keys[1][4].test_value, keys[1][4].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][2].curr_pos, keys[1][2].test_value, keys[1][2].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][8].curr_pos, keys[1][8].test_value, keys[1][8].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][7].curr_pos, keys[1][7].test_value, keys[1][7].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][6].curr_pos, keys[1][6].test_value, keys[1][6].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][5].curr_pos, keys[1][5].test_value, keys[1][5].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][3].curr_pos, keys[1][3].test_value, keys[1][3].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][1].curr_pos, keys[1][1].test_value, keys[1][1].min_value);
        // uprintf("(%u, %u, %u)\n", keys[1][9].curr_pos, keys[1][9].test_value, keys[1][9].min_value);
        // uprintf("(%u, %u, %u)\n\n", keys[1][0].curr_pos, keys[1][0].test_value, keys[1][0].min_value);

        uprintf("(%u, %u)\n", user_config.travel_distance, user_config.sensitivity);

        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][4].actuation_point, user_config.key_config[1 + thisHand][4].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][2].actuation_point, user_config.key_config[1 + thisHand][2].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][8].actuation_point, user_config.key_config[1 + thisHand][8].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][7].actuation_point, user_config.key_config[1 + thisHand][7].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][6].actuation_point, user_config.key_config[1 + thisHand][6].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][5].actuation_point, user_config.key_config[1 + thisHand][5].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][3].actuation_point, user_config.key_config[1 + thisHand][3].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][1].actuation_point, user_config.key_config[1 + thisHand][1].mode);
        uprintf("(%u, %u)\n", user_config.key_config[1 + thisHand][9].actuation_point, user_config.key_config[1 + thisHand][9].mode);
        uprintf("(%u, %u)\n\n", user_config.key_config[1 + thisHand][0].actuation_point, user_config.key_config[1 + thisHand][0].mode);

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