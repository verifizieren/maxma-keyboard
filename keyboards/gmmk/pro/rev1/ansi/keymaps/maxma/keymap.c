// Copyright 2026 Max Mendes (@verifizieren)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _FN,
    _L2,  // spare, for VIA
    _L3,  // spare, for VIA
};

enum custom_keycodes {
    RESET_CFG = QK_KB_0,  // tap: reset lighting. hold 2s: wipe EEPROM.
};

// No HID keycode mutes a microphone; the host has to. This sends a combo
// unlikely to collide, to be bound as the mute hotkey in Discord/OBS/etc.
#define MIC_MUTE C(A(S(KC_M)))

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    /* Base — stock ANSI, except the right column reads Del / Ins / Home / End.
     *
     * ,---------------------------------------------------------------.  ,------.
     * | Esc| F1| F2| F3| F4| F5| F6| F7| F8| F9|F10|F11|F12|   PrtSc   |  | Knob |
     * |---------------------------------------------------------------|  |------|
     * |  ` | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0 | - | = |   Bksp    |  | Del  |
     * |---------------------------------------------------------------|  |------|
     * | Tab | Q | W | E | R | T | Y | U | I | O | P | [ | ] |    \     |  | Ins  |
     * |---------------------------------------------------------------|  |------|
     * | Caps | A | S | D | F | G | H | J | K | L | ; | ' |   Enter     |  | Home |
     * |---------------------------------------------------------------|  |------|
     * | Shift  | Z | X | C | V | B | N | M | , | . | / | Shift  |  Up  |  | End  |
     * |---------------------------------------------------------------|  |------|
     * | Ctl | Win | Alt |        Space        | Alt | Fn | Ctl | L D R |
     * `---------------------------------------------------------------'
     */
    [_BASE] = LAYOUT(
        KC_ESC,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_PSCR,          KC_MUTE,
        KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,          KC_DEL,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_INS,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,           KC_HOME,
        KC_LSFT,          KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,          KC_RSFT, KC_UP,   KC_END,
        KC_LCTL, KC_LGUI, KC_LALT,                            KC_SPC,                             KC_RALT, MO(_FN), KC_RCTL, KC_LEFT, KC_DOWN, KC_RGHT
    ),

    /* FN — a short list of assignments, everything else transparent.
     * (Task 6 adds SOCDTOG on S; no count is written here so this comment
     * cannot go stale.)
     *
     * Esc      reset (tap: lighting, hold 2s: full wipe)
     * F1-F4    prev / play-pause / next / stop
     * F5-F7    mute / vol- / vol+
     * F8       mic mute (Ctrl+Alt+Shift+M)
     * F9,F10   Scroll Lock, Pause
     * PrtSc    bootloader
     * -,=      RGB brightness down/up
     * Bksp     Delete
     * K        NKRO toggle          N       Num Lock
     * arrows   PgUp/PgDn/Home/End   Ins,Home column   PgUp/PgDn
     * knob     RGB on/off
     */
    [_FN] = LAYOUT(
        RESET_CFG, KC_MPRV, KC_MPLY, KC_MNXT, KC_MSTP, KC_MUTE, KC_VOLD, KC_VOLU, MIC_MUTE, KC_SCRL, KC_PAUS, _______, _______, QK_BOOT,        RM_TOGG,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, RM_VALD, RM_VALU, KC_DEL,           _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          KC_PGUP,
        _______, _______, _______, _______, _______, _______, _______, _______, NK_TOGG, _______, _______, _______,          _______,          KC_PGDN,
        _______,          _______, _______, _______, _______, _______, KC_NUM,  _______, _______, _______, _______,          _______, KC_PGUP, _______,
        _______, _______, _______,                            _______,                            _______, _______, _______, KC_HOME, KC_PGDN, KC_END
    ),

    /* Spare layers. Transparent, but they MUST exist: VIA seeds its EEPROM
     * copy by reading keymaps[layer] for every dynamic layer, so a short
     * array gets read past its end and the spares fill with garbage. */
    [_L2] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,          _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______, _______, _______,
        _______, _______, _______,                            _______,                            _______, _______, _______, _______, _______, _______
    ),

    [_L3] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,          _______,
        _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______, _______, _______,
        _______, _______, _______,                            _______,                            _______, _______, _______, _______, _______, _______
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_FN]   = { ENCODER_CCW_CW(RM_VALD, RM_VALU) },
    [_L2]   = { ENCODER_CCW_CW(_______, _______) },
    [_L3]   = { ENCODER_CCW_CW(_______, _______) },
};
#endif
// clang-format on

// Knob dispatch.
//
// Returning true hands the event to encoder_map, which is the path VIA can
// remap — so the plain turn and the Fn+turn stay user-configurable. Only
// modifiers are intercepted here, because a modifier is not a layer and
// encoder_map cannot see it.
//
// Fn deliberately does NOT appear below: Fn is a momentary layer, so holding
// it already selects encoder_map[_FN].
bool encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t mods = get_mods();

    if (mods & MOD_MASK_CTRL) {
        // Bare arrow, not C(KC_RGHT). Ctrl is already physically held, so the
        // arrow picks it up on its own. Wrapping it would make QMK release the
        // held Ctrl at the end of the tap, desyncing it from the physical key.
        tap_code(clockwise ? KC_RGHT : KC_LEFT);
        return false;
    }

    if (mods & MOD_MASK_ALT) {
        // Opposite treatment: media keycodes should arrive unmodified, so
        // strip Alt around the tap and put it back.
        uint8_t held_alt = mods & MOD_MASK_ALT;
        unregister_mods(held_alt);
        tap_code(clockwise ? KC_MNXT : KC_MPRV);
        register_mods(held_alt);
        return false;
    }

    return true;
}
