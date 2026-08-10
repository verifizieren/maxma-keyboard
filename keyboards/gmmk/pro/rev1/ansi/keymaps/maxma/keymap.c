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

// Turning the knob on the FN layer adjusts hue — but the FN layer also blacks
// the board out to highlight its own keys, so you'd be picking a colour you
// cannot see. Any FN-layer knob turn opens a short window during which the
// highlight steps aside and the real colour shows through.
#define RGB_PREVIEW_MS 5000
static uint32_t rgb_preview_timer = 0;
static bool     rgb_preview       = false;

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
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Z,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_INS,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,          KC_ENT,           KC_HOME,
        KC_LSFT,          KC_Y,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,          KC_RSFT, KC_UP,   KC_END,
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
    [_FN]   = { ENCODER_CCW_CW(RM_HUED, RM_HUEU) },
    [_L2]   = { ENCODER_CCW_CW(_______, _______) },
    [_L3]   = { ENCODER_CCW_CW(_______, _______) },
};
#endif
// clang-format on

// Knob dispatch.
//
// NOT in encoder_update_user(). With ENCODER_MAP_ENABLE, QMK's encoder_handle
// (quantum/encoder.c) sends turns straight to action_exec() and only calls
// encoder_update_kb/_user in the #else branch — so with the map enabled that
// hook is never invoked at all. An implementation there compiles, reviews
// clean, and silently does nothing.
//
// Encoder turns instead arrive here as keyrecords carrying ENCODER_CW_EVENT /
// ENCODER_CCW_EVENT, so the modifier handling lives in process_record_user.
// Returning true falls through to the encoder_map keycode, which is the path
// VIA can remap.
//
// Fn is deliberately not a case below: it is a momentary layer, so holding it
// already selects encoder_map[_FN].
static bool encoder_dispatch(keyrecord_t *record) {
    // Every turn fires a press and then a release. Act on the press; if we
    // consumed it, consume the matching release too, otherwise the encoder
    // map's keycode would be registered and never released.
    static bool consumed = false;

    if (!record->event.pressed) {
        if (consumed) {
            consumed = false;
            return false;
        }
        return true;
    }

    const bool    clockwise = (record->event.type == ENCODER_CW_EVENT);
    const uint8_t mods      = get_mods();

    if (mods & MOD_MASK_SHIFT) {
        // Shift+turn extends the selection a word at a time: Ctrl+Shift+Arrow.
        // Shift is already physically held, so only Ctrl needs adding — and
        // only when it isn't already down, otherwise the unregister below
        // would release the user's real Ctrl and desync it from the key.
        // Checked before the Ctrl branch so Ctrl+Shift+turn selects too,
        // rather than falling through to a plain word jump.
        const uint8_t others   = mods & ~(MOD_MASK_SHIFT | MOD_MASK_CTRL);
        const bool    add_ctrl = !(mods & MOD_MASK_CTRL);
        if (others) unregister_mods(others);
        if (add_ctrl) register_mods(MOD_BIT(KC_LCTL));
        tap_code(clockwise ? KC_RGHT : KC_LEFT);
        if (add_ctrl) unregister_mods(MOD_BIT(KC_LCTL));
        if (others) register_mods(others);
        consumed = true;
        return false;
    }

    if (mods & MOD_MASK_CTRL) {
        // Bare arrow so the physically-held Ctrl applies itself, but strip
        // any other mods: Ctrl+Alt+Arrow is a Windows shortcut, not word jump.
        const uint8_t others = mods & ~MOD_MASK_CTRL;
        if (others) unregister_mods(others);
        tap_code(clockwise ? KC_RGHT : KC_LEFT);
        if (others) register_mods(others);
        consumed = true;
        return false;
    }

    if (mods & MOD_MASK_ALT) {
        // No strip/restore needed here, unlike the Ctrl branch above.
        // KC_MNXT/KC_MPRV are Consumer Control usages: action.c routes them
        // through host_consumer_send() into their own HID report
        // (report_extra_t, report.h) that carries only a report ID and a
        // usage code — no modifier byte exists in that report for a held
        // Alt to ride along on. A register_mods(held_alt) here would instead
        // emit a fresh Alt-down in the *keyboard* report with no keypress
        // between it and the eventual physical release, which Windows reads
        // as an isolated Alt tap and pops the menu bar.
        tap_code(clockwise ? KC_MNXT : KC_MPRV);
        consumed = true;
        return false;
    }

    // Unclaimed, so the encoder map gets it. On the FN layer that is a hue
    // change — open the preview window so the blackout steps aside and the
    // colour being dialled in is actually visible.
    if (layer_state_is(_FN)) {
        rgb_preview_timer = timer_read32();
        rgb_preview       = true;
    }

    consumed = false;
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
// a position does, so every indicator follows VIA remaps instead of drifting
// out of sync with them — remap Num Lock elsewhere and the blue follows it.
//
// Nothing paints the side bars: they stay on the baseline colour so the left
// and right sides always match.
//
// Called once per frame with a slice of the LED range, so everything is
// clipped to [led_min, led_max).

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // Past the warning threshold the board blinks red. Let go now and the
    // release lands under RESET_WIPE_MS; keep holding and it wipes.
    //
    // This return short-circuits before the magenta SOCD bar is ever drawn,
    // so a red flash during a RESET_CFG hold hides that warning for as long
    // as the hold lasts. Deliberate, not an oversight: RESET_CFG is a
    // momentary, self-terminating hold (release within a couple of seconds
    // either way), while SOCD armed is a persistent state the user already
    // saw light up and will see again the instant this returns. Do not
    // change this.
    if (reset_held && timer_elapsed32(reset_timer) >= RESET_WARN_MS) {
        const bool on = (timer_read() / 150) % 2;
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_matrix_set_color(i, on ? 0xFF : 0x00, 0, 0);
        }
        return false;
    }

    const led_t leds = host_keyboard_led_state();

    // A recent FN-layer knob turn opens the preview window. It closes on its
    // own once you stop turning.
    if (rgb_preview && timer_elapsed32(rgb_preview_timer) >= RGB_PREVIEW_MS) {
        rgb_preview = false;
    }

    const bool fn_held = layer_state_is(_FN);

    // The blackout — not the green highlight — is what the preview suspends.
    // Normally holding Fn darkens every key the layer doesn't use, so the
    // functional ones stand out. But that also hides the colour you are
    // dialling in with the knob, so while the preview window is open the
    // unused keys keep showing the live hue and only the functional keys are
    // overpainted. Stop turning, wait it out, and the blackout returns.
    if (fn_held && !rgb_preview) {
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }

    // Caps Lock floods every key green — impossible to miss, and the way the
    // previous keymap on this board did it. Key LEDs only; the side bars stay
    // on the baseline colour like everything else.
    //
    // Skipped while Fn is held: the FN highlight is also green, so flooding
    // here would erase the distinction between "this key does something on
    // the layer" and "Caps is on". Painted before the per-key indicators
    // below so lock states, the status light and the SOCD warning all still
    // show through on top.
    if (leds.caps_lock && !fn_held) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (g_led_config.flags[i] & LED_FLAG_KEYLIGHT) {
                rgb_matrix_set_color(i, RGB_GREEN);
            }
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
                    rgb_matrix_set_color(idx, RGB_GREEN);
                }
                // Amber on the NKRO key while NKRO is off, so the warning is
                // only present when you are already looking at the layer.
                if (fn_kc == NK_TOGG && !keymap_config.nkro) {
                    rgb_matrix_set_color(idx, RGB_ORANGE);
                }
            }

            // Lock states live on the key that toggles them, and only while
            // the lock is actually on. An indicator that is lit most of the
            // time carries no information — which is why Num Lock is on N
            // rather than the side bar it used to own.
            const uint16_t base_kc = keymap_key_to_keycode(_BASE, pos);

            if (leds.num_lock && base_kc == KC_N) {
                rgb_matrix_set_color(idx, RGB_BLUE);
            }
            if (leds.scroll_lock && base_kc == KC_F9) {
                rgb_matrix_set_color(idx, RGB_RED);
            }

            // SOCD armed: magenta on WASD, the keys it actually affects.
            // Painted last so it outranks everything, including the FN
            // blackout — this warning must never be hidden.
            if (socd_cleaner_enabled &&
                (base_kc == KC_W || base_kc == KC_A || base_kc == KC_S || base_kc == KC_D)) {
                rgb_matrix_set_color(idx, RGB_MAGENTA);
            }

            // Status light on '='. Green when the board is in its normal
            // state, red when anything is off-normal — SOCD armed, or NKRO
            // dropped to 6KRO. One glance, before joining a game.
            //
            // It also sits on the one LED whose blue channel is physically
            // dead, which would otherwise render every blue-ish hue as
            // yellow. Driving it red-or-green only means that fault can
            // never show: neither colour needs blue. Deliberate placement,
            // not a coincidence — if that LED is ever repaired or the board
            // replaced, this can move anywhere.
            if (base_kc == KC_EQL) {
                const uint8_t v = rgb_matrix_get_val();  // track board brightness
                if (socd_cleaner_enabled || !keymap_config.nkro) {
                    rgb_matrix_set_color(idx, v, 0, 0);  // red: something needs attention
                } else {
                    rgb_matrix_set_color(idx, 0, v, 0);  // green: all normal
                }
            }
        }
    }

    // Both side bars are left on the baseline colour deliberately. Every
    // indicator now sits on the key it refers to, so the bars stay uniform.

    return false;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
#ifdef ENCODER_MAP_ENABLE
    if (record->event.type == ENCODER_CW_EVENT || record->event.type == ENCODER_CCW_EVENT) {
        return encoder_dispatch(record);
    }
#endif

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
                    //
                    // rgb_matrix_mode_eeprom_helper and
                    // rgb_matrix_sethsv_eeprom_helper both bail out at their
                    // very first line if the matrix is currently disabled
                    // (rgb_matrix.c:581,633 in upstream QMK) — only
                    // rgb_matrix_set_speed_eeprom_helper (:722) lacks that
                    // guard. So with the backlight off, the three calls below
                    // would silently apply speed alone and skip mode/HSV,
                    // leaving this documented recovery path half-broken in
                    // exactly the state a user is likeliest to reach for it.
                    //
                    // Force it on first. RGB_MATRIX_DEFAULT_ON is true, so
                    // "on" is itself part of the compiled default this tap is
                    // restoring, not an extra side effect.
                    rgb_matrix_enable();
                    // That "on" is real and saved, not a warning override —
                    // don't let a stale forced-on latch undo it once whatever
                    // warning is showing (if any) later clears. Same reason
                    // RM_TOGG below drops the latch.
                    rgb_forced_on = false;
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
