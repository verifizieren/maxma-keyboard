// Copyright 2026 Max Mendes (@verifizieren)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "socd_cleaner.h"

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

// RESET_CFG timing. The warning fires 500ms BEFORE the wipe threshold, and
// that gap is the escape hatch: see red, let go, land in the dead band.
#define RESET_TAP_MS  500   // below this, a tap: reset lighting
#define RESET_WARN_MS 1500  // from here, flash red
#define RESET_WIPE_MS 2000  // at or above this, release wipes EEPROM

static uint32_t reset_timer = 0;
static bool     reset_held  = false;

// Tracks whether housekeeping_task_user is the one currently holding the RGB
// matrix on. File-scope (not local to housekeeping_task_user) so
// process_record_user can clear it the instant the user presses the real
// toggle — see the RM_TOGG case below for why that matters.
static bool rgb_forced_on = false;

// SOCD cleaning for the two WASD axes. LAST resolution: the most recently
// pressed key wins, and releasing it reactivates the still-held opposite —
// so a strafe reversal has no dead moment.
//
// PROHIBITED IN CS2 AND VALORANT. Both moved against this class of feature
// in 2024. That is why it boots disarmed and why the right light bar warns
// while it is live. Do not change either.
socd_cleaner_t socd_opposing_pairs[] = {
    {{KC_A, KC_D}, SOCD_CLEANER_LAST},  // strafe left / right
    {{KC_W, KC_S}, SOCD_CLEANER_LAST},  // forward / back
};

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
        _______, _______, SOCDTOG, _______, _______, _______, _______, _______, NK_TOGG, _______, _______, _______,          _______,          KC_PGDN,
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
        // Bare arrow so the physically-held Ctrl applies itself, but strip
        // any other mods: Ctrl+Alt+Arrow is a Windows shortcut, not word jump.
        const uint8_t others = mods & ~MOD_MASK_CTRL;
        if (others) unregister_mods(others);
        tap_code(clockwise ? KC_RGHT : KC_LEFT);
        if (others) register_mods(others);
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

void keyboard_post_init_user(void) {
    // Always come up disarmed, whatever was last set. A power cycle is the
    // guaranteed way back to a safe state.
    socd_cleaner_enabled = false;

    // FORCE_NKRO was removed from QMK, so force it by hand. Set the live
    // flag only — no eeconfig write — so Fn+K toggles for this session and
    // a replug always comes back to NKRO on.
    keymap_config.nkro = true;
}

// Both safety warnings live in rgb_matrix_indicators_advanced_user, which
// QMK skips entirely while the matrix is disabled. Neither warning may be
// silently suppressible, so arming SOCD or crossing the reset warning
// threshold forces the lighting back on. Once the warning clears, undo only
// what we ourselves forced — rgb_forced_on is the latch that makes that
// possible, and the RM_TOGG case in process_record_user is what keeps it
// honest if the user reaches for Fn+knob while a warning is showing.
void housekeeping_task_user(void) {
    const bool needs_warning = socd_cleaner_enabled ||
                               (reset_held && timer_elapsed32(reset_timer) >= RESET_WARN_MS);

    if (needs_warning && !rgb_matrix_is_enabled()) {
        rgb_matrix_enable_noeeprom();  // noeeprom: don't clobber the saved preference
        rgb_forced_on = true;
    } else if (!needs_warning && rgb_forced_on) {
        rgb_matrix_disable_noeeprom();  // only ever undo our own override
        rgb_forced_on = false;
    }
}

// Indicators.
//
// No LED index is hard-coded. Key LEDs are found by asking the keymap what
// a position does, so the highlight follows VIA remaps instead of drifting
// out of sync with them. Side-bar LEDs are found by their underglow flag.
//
// Called once per frame with a slice of the LED range, so everything is
// clipped to [led_min, led_max).

// The two side bars are underglow LEDs at the extreme left and right of the
// matrix. bar_x is 0 for the left bar, 224 for the right.
static void set_bar(uint8_t bar_x, uint8_t led_min, uint8_t led_max, uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = led_min; i < led_max; i++) {
        if ((g_led_config.flags[i] & LED_FLAG_UNDERGLOW) && g_led_config.point[i].x == bar_x) {
            rgb_matrix_set_color(i, r, g, b);
        }
    }
}

#define LEFT_BAR_X 0
#define RIGHT_BAR_X 224

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // Past the warning threshold the board blinks red. Let go now and the
    // release lands under RESET_WIPE_MS; keep holding and it wipes.
    if (reset_held && timer_elapsed32(reset_timer) >= RESET_WARN_MS) {
        const bool on = (timer_read() / 150) % 2;
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_matrix_set_color(i, on ? 0xFF : 0x00, 0, 0);
        }
        return false;
    }

    const led_t leds    = host_keyboard_led_state();
    const bool  fn_held = layer_state_is(_FN);

    if (fn_held) {
        // Black out the range first, then light only what the layer defines.
        // Recognition instead of recall — the whole point of this indicator.
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            const uint8_t idx = g_led_config.matrix_co[row][col];
            if (idx == NO_LED || idx < led_min || idx >= led_max) {
                continue;
            }
            const keypos_t pos = (keypos_t){.col = col, .row = row};

            if (fn_held) {
                const uint16_t fn_kc = keymap_key_to_keycode(_FN, pos);
                if (fn_kc != KC_TRNS && fn_kc != KC_NO) {
                    rgb_matrix_set_color(idx, RGB_WHITE);
                }
                // Amber on the NKRO key while NKRO is off, so the warning is
                // only present when you are already looking at the layer.
                if (fn_kc == NK_TOGG && !keymap_config.nkro) {
                    rgb_matrix_set_color(idx, RGB_ORANGE);
                }
            }

            if (leds.caps_lock && keymap_key_to_keycode(_BASE, pos) == KC_CAPS) {
                rgb_matrix_set_color(idx, RGB_GREEN);
            }
        }
    }

    // Left bar: one bar, three claimants, so they are mutually exclusive.
    if (leds.caps_lock) {
        set_bar(LEFT_BAR_X, led_min, led_max, RGB_GREEN);
    } else if (leds.scroll_lock) {
        set_bar(LEFT_BAR_X, led_min, led_max, RGB_RED);
    } else if (leds.num_lock) {
        set_bar(LEFT_BAR_X, led_min, led_max, RGB_BLUE);
    }

    // Right bar: the SOCD safety warning, and nothing else. It gets its own
    // bar precisely so a lock state can never hide it.
    if (socd_cleaner_enabled) {
        set_bar(RIGHT_BAR_X, led_min, led_max, RGB_MAGENTA);
    }

    return false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case NK_TOGG:
            // Handle NKRO ourselves rather than letting process_magic have it.
            // process_magic re-reads keymap_config from EEPROM before toggling
            // and writes it back, which would both desync it from the RAM flag
            // keyboard_post_init_user sets AND make the toggle persist. The
            // user wants NKRO forced on at every boot, with this key toggling
            // for the current session only.
            if (record->event.pressed) {
                keymap_config.nkro = !keymap_config.nkro;
            }
            return false;
        case RM_TOGG:
            // The user might reach for the real toggle while a safety
            // warning is holding the matrix on. Don't try to guess what
            // they landed on — just drop our "we forced this" claim and let
            // the very next housekeeping tick re-derive it from the actual
            // enabled state. If the warning is still live and the matrix is
            // now off, housekeeping re-forces it and re-claims ownership
            // (the warning is intentionally not user-suppressible). If the
            // user's press left it genuinely on, we won't wrongly own it,
            // so we won't turn it back off when the warning clears.
            rgb_forced_on = false;
            return true;
        case RESET_CFG:
            if (record->event.pressed) {
                reset_timer = timer_read32();
                reset_held  = true;
            } else {
                const uint32_t held = timer_elapsed32(reset_timer);
                reset_held = false;

                if (held < RESET_TAP_MS) {
                    // Tap: lighting back to the compiled defaults. These are
                    // the eeprom-saving variants, so the reset persists.
                    rgb_matrix_mode(RGB_MATRIX_DEFAULT_MODE);
                    rgb_matrix_sethsv(RGB_MATRIX_DEFAULT_HUE, RGB_MATRIX_DEFAULT_SAT, RGB_MATRIX_DEFAULT_VAL);
                    rgb_matrix_set_speed(RGB_MATRIX_DEFAULT_SPD);
                } else if (held >= RESET_WIPE_MS) {
                    // Hold: everything goes, VIA remaps included.
                    eeconfig_init();
                    soft_reset_keyboard();
                }
                // Between the two: deliberately nothing. A hesitant press is
                // a no-op rather than a wipe.
            }
            return false;
    }
    return true;
}
