#include QMK_KEYBOARD_H

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE,
    _FN
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┐
     * │ N │ I │ C │ E │
     * └───┴───┴───┴───┘
     */
    /* Base */
    [_BASE] = LAYOUT(
        KC_N, KC_I, KC_C, KC_E
    ),

    [_FN] = LAYOUT(
        KC_N, KC_I, KC_C, KC_E
    ),
    /*
    [0] = LAYOUT_ortho_4x4(
        KC_P7,   KC_P8,   KC_P9,   KC_PSLS,
        KC_P4,   KC_P5,   KC_P6,   KC_PAST,
        KC_P1,   KC_P2,   KC_P3,   KC_PMNS,
        KC_P0,   KC_PDOT, KC_PENT, KC_PPLS
    )
    */
};
