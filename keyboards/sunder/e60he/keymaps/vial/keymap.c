#include QMK_KEYBOARD_H

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE,
    _FN
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_TAB,  KC_Q,    KC_W,    KC_E,    \
        KC_R,    KC_T,    KC_ESC,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_LSFT, KC_Z,    \
        KC_X,    KC_C,    KC_V,    KC_B,    KC_LCTL, KC_LGUI, MO(_FN), KC_LALT, MO(_FN), KC_SPC,  \
        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSPC, KC_Y,    KC_U,    KC_I,    KC_O,    \
        KC_P,    KC_DEL,  KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_N,    KC_M,    \
        KC_COMM, KC_DOT,  KC_SLSH, KC_ENT,  KC_SPC,  MO(_FN), KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT  \
    ),

    [_FN] = LAYOUT(
        KC_NO,   KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_NO,   KC_F11,  KC_F12,  KC_NO,   \
        KC_NO,   KC_NO,   KC_CAPS, KC_NO,   KC_NO,   KC_SLSH, KC_LCBR, KC_LBRC, KC_LSFT, KC_NO,   \
        KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_TRNS, KC_NO,   KC_TRNS, KC_SPC,  \
        KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_UNDS, \
        KC_PLUS, KC_NO,   KC_RBRC, KC_RCBR, KC_BSLS, KC_MINS, KC_EQL,  KC_PIPE, KC_NO,   KC_NO,   \
        KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_SPC,  KC_TRNS, KC_NO,   KC_NO,   KC_NO,   KC_NO    \
    ),
};
