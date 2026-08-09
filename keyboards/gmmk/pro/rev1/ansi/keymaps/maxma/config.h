// Copyright 2026 Max Mendes (@verifizieren)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// NKRO on at every boot. Fn+K still toggles it off for the current session,
// for a BIOS or KVM that chokes on it, but it never stays off silently.
#define FORCE_NKRO

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

// Current QMK enables no animations for this board — SOLID_COLOR is the only
// ungated effect. So we opt in to five, rather than undefining forty as the
// 2023 reference keymap had to.
#    define ENABLE_RGB_MATRIX_BREATHING
#    define ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
#    define ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
#    define ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
#    define ENABLE_RGB_MATRIX_TYPING_HEATMAP

// Warm white. Neutral enough that the coloured indicators stand out.
#    define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#    define RGB_MATRIX_DEFAULT_HUE 24
#    define RGB_MATRIX_DEFAULT_SAT 60
#    define RGB_MATRIX_DEFAULT_VAL 150
#    define RGB_MATRIX_DEFAULT_SPD 127
#endif
