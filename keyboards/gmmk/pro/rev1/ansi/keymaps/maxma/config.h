// Copyright 2026 Max Mendes (@verifizieren)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// NKRO on at every boot. FORCE_NKRO was removed from QMK (2026-05-31
// release), so this is enforced by hand in keyboard_post_init_user() in
// keymap.c (keymap_config.nkro = true, no eeconfig write) instead of here.
// Fn+K still toggles it off for the current session, for a BIOS or KVM that
// chokes on it, but it never stays off silently — a replug forces it on.

// Base, FN, and two spares to experiment with in VIA.
// All four are present in keymaps[] and encoder_map[] — see keymap.c.
#define DYNAMIC_KEYMAP_LAYER_COUNT 4

// The GMMK Pro has no real EEPROM; QMK emulates it in flash behind a
// wear-levelling layer. Four VIA layers do not fit the 1024-byte default.
// Undersizing fails loudly at link time, so these are safe to tune.
#define WEAR_LEVELING_LOGICAL_SIZE 2048
#define WEAR_LEVELING_BACKING_SIZE 4096

#ifdef RGB_MATRIX_ENABLE
#    define RGB_MATRIX_SLEEP  // lights off while the host is suspended

// Required by the reactive effects and the heatmap respectively.
#    define RGB_MATRIX_KEYPRESSES
#    define RGB_MATRIX_FRAMEBUFFER_EFFECTS

// The brief's premise here was wrong: keyboards/gmmk/pro/info.json enables
// 43 animations and rev1/ansi inherits all of them, so these five #defines
// were already in that inherited set and changed nothing on their own.
// A keymap.json `rgb_matrix.animations` block was tried first, per the
// coordinator's fix-round instructions — QMK's deep_update() merge
// (lib/python/qmk/json_schema.py) only overwrites the keys it lists and
// leaves everything else from the parent info.json untouched, so listing
// five `true` entries is a no-op against a parent that already has them
// true; the other 38 stayed enabled (confirmed by size: identical byte
// count before and after). So instead: undefine every animation this
// keymap does not want. This is certain to work because
// keyboards/*/keymaps/maxma/config.h is appended to the build's CONFIG_H
// list AFTER the generated info_config.h (see
// builddefs/build_keyboard.mk, info_config.h added at the keyboard-level
// block, this keymap's config.h added later near the end), so these
// #undefs are processed after info_config.h's #defines and are not
// re-defined afterward.
//
// Keep exactly six modes: SOLID_COLOR (always compiled, not gated by an
// ENABLE_RGB_MATRIX_* flag) plus the five below.
#    define ENABLE_RGB_MATRIX_BREATHING
#    define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#    define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#    define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#    define ENABLE_RGB_MATRIX_TYPING_HEATMAP

// Every other animation info.json enables, turned back off. Enumerated from
// keyboards/gmmk/pro/info.json's rgb_matrix.animations block.
#    undef ENABLE_RGB_MATRIX_ALPHAS_MODS
#    undef ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
#    undef ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
#    undef ENABLE_RGB_MATRIX_BAND_SAT
#    undef ENABLE_RGB_MATRIX_BAND_VAL
#    undef ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
#    undef ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
#    undef ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
#    undef ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
#    undef ENABLE_RGB_MATRIX_CYCLE_ALL
#    undef ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
#    undef ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
#    undef ENABLE_RGB_MATRIX_CYCLE_OUT_IN
#    undef ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
#    undef ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
#    undef ENABLE_RGB_MATRIX_CYCLE_SPIRAL
#    undef ENABLE_RGB_MATRIX_DUAL_BEACON
#    undef ENABLE_RGB_MATRIX_RAINBOW_BEACON
#    undef ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
#    undef ENABLE_RGB_MATRIX_RAINDROPS
#    undef ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
#    undef ENABLE_RGB_MATRIX_HUE_BREATHING
#    undef ENABLE_RGB_MATRIX_HUE_PENDULUM
#    undef ENABLE_RGB_MATRIX_HUE_WAVE
#    undef ENABLE_RGB_MATRIX_PIXEL_RAIN
#    undef ENABLE_RGB_MATRIX_PIXEL_FLOW
#    undef ENABLE_RGB_MATRIX_PIXEL_FRACTAL
#    undef ENABLE_RGB_MATRIX_DIGITAL_RAIN
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
#    undef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
#    undef ENABLE_RGB_MATRIX_SPLASH
#    undef ENABLE_RGB_MATRIX_MULTISPLASH
#    undef ENABLE_RGB_MATRIX_SOLID_SPLASH
#    undef ENABLE_RGB_MATRIX_SOLID_MULTISPLASH

// Warm white. Neutral enough that the coloured indicators stand out.
#    define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#    define RGB_MATRIX_DEFAULT_HUE 24
#    define RGB_MATRIX_DEFAULT_SAT 60
#    define RGB_MATRIX_DEFAULT_VAL 150
#    define RGB_MATRIX_DEFAULT_SPD 127
#endif
