# maxma keymap — GMMK Pro rev1 ANSI

A small custom keymap: one function layer, functional lighting instead of decorative animations,
and a knob that does more when a modifier is held. It replaces the `gourdo1` keymap that used to
ship on this board; nothing from that keymap was carried over except the two directly-useful ideas
(indicator lighting, a knob with modifier awareness) — reimplemented from scratch, not ported.

Firmware size at last build: **34476 bytes**.

## Base layer

Stock GMMK Pro ANSI, with two changes: the right-hand column below the knob reads **Del / Ins /
Home / End** instead of stock's Del / Page Up / Page Down / End — rows 1 and 4 (Del, End) match
stock; rows 2 and 3 (Ins and Home, swapped in for Page Up and Page Down) are what changed.
Everything else — alphas, numbers, symbols, both shifts, Caps Lock, Print Screen, the arrow
cluster — is unmodified stock.

| Right column (top → bottom) | Keycode |
|---|---|
| knob | *(press: mute, see knob table)* |
| row 1 | `KC_DEL` — unchanged from stock |
| row 2 | `KC_INS` — **changed from stock `KC_PGUP`** |
| row 3 | `KC_HOME` — **changed from stock `KC_PGDN`** |
| row 4 | `KC_END` — unchanged from stock |

Page Up and Page Down aren't lost — they move to the FN layer (see below), reachable in two places
each.

## FN layer

25 assignments. Every other key on this layer is `KC_TRNS` (transparent — falls through to base).

| Key | Keycode | Behavior |
|---|---|---|
| Esc | `RESET_CFG` (custom) | tap: reset lighting to compiled defaults · hold ≥2 s: full EEPROM wipe + reboot |
| F1 | `KC_MPRV` | previous track |
| F2 | `KC_MPLY` | play / pause |
| F3 | `KC_MNXT` | next track |
| F4 | `KC_MSTP` | stop |
| F5 | `KC_MUTE` | mute |
| F6 | `KC_VOLD` | volume down |
| F7 | `KC_VOLU` | volume up |
| F8 | `C(A(S(KC_M)))` | mic mute — sends **Ctrl+Alt+Shift+M** (see below) |
| F9 | `KC_SCRL` | Scroll Lock |
| F10 | `KC_PAUS` | Pause / Break |
| PrtSc | `QK_BOOT` | enter bootloader |
| `-` | `RM_VALD` | RGB brightness down |
| `=` | `RM_VALU` | RGB brightness up |
| Backspace | `KC_DEL` | delete forward |
| K | `NK_TOGG` (intercepted, see NKRO section) | toggle NKRO for this session only |
| N | `KC_NUM` | toggle Num Lock |
| S | `SOCDTOG` | arm / disarm the SOCD cleaner — see the SOCD section below before using this |
| ↑ / ↓ | `KC_PGUP` / `KC_PGDN` | page up / down |
| ← / → | `KC_HOME` / `KC_END` | home / end |
| Ins (right column) | `KC_PGUP` | page up |
| Home (right column) | `KC_PGDN` | page down |
| knob press | `RM_TOGG` | RGB matrix on / off |

Page Up/Down and Home/End each exist in two places. That redundancy is intentional, not an
oversight — it was chosen over trimming.

Holding Fn lights exactly **24** keys white, not 25. `RM_TOGG` sits on the knob press, which is the
one `LAYOUT` position on this board with no LED behind it — the knob staying dark while Fn is held
is correct, not a bug. See the indicator table below for why the layer lights up at all.

### `RESET_CFG` timing (Fn+Esc)

A single custom keycode (`QK_KB_0`, so VIA can see and remap it).

| Hold time | Result |
|---|---|
| < 500 ms (tap) | Lighting resets to compiled defaults (mode, hue/sat/val) and saves |
| 500 ms – 1500 ms | Nothing. Dead band — a hesitant press is a no-op |
| ≥ 1500 ms while still held | Matrix blinks red — the escape hatch |
| ≥ 2000 ms on release | `eeconfig_init()` + reboot — full EEPROM wipe, VIA remaps included |

**The escape hatch:** the red blink starts at 1500 ms, a full 500 ms *before* the 2000 ms wipe
threshold. See red, let go immediately, and the release lands inside the earlier 500–1500 ms dead
band — a no-op, not a wipe. The outcome is only committed once you cross 2000 ms. This is why the
blink starts early rather than exactly at the threshold: without that lead time there would be no
window to react in.

The tap variant only touches lighting. The hold variant wipes everything persisted, VIA remaps
included. NKRO has no persisted state to wipe — it's a session-only RAM flag
(`keyboard_post_init_user()`, `process_record_user()`) forced on at every boot regardless of what
the EEPROM wipe does.

## The knob

Six behaviors: a plain turn/press, three modifier combinations, and Fn+press.

| Input | Result | Path | VIA-remappable? |
|---|---|---|---|
| turn | volume up / down | `encoder_map[_BASE]` | yes |
| press | mute | ordinary matrix key | yes |
| Fn + turn | RGB brightness up / down | `encoder_map[_FN]` | yes |
| Ctrl + turn | previous / next **word** | custom code in `encoder_update_user` | no |
| Alt + turn | previous / next **track** | custom code in `encoder_update_user` | no |
| Fn + press | RGB matrix on / off | FN layer keymap entry (`RM_TOGG`) | yes |

Fn+turn needs no custom code at all: Fn is a momentary layer, so holding it already selects
`encoder_map[_FN]`. Only Ctrl and Alt need interception, because they're modifiers rather than
layers — `encoder_map` has no way to see "Ctrl is held."

**Ctrl+turn taps a bare arrow key**, not `C(KC_RGHT)`. Ctrl is already physically held down, so the
bare arrow picks the modifier up on its own. Wrapping it in `C(...)` would make QMK release the held
Ctrl the instant the tap ends, desyncing it from the key that's still physically down. The code does
strip any *other* held modifier first, though — a bare arrow otherwise collects every modifier
currently down, and Ctrl+Alt+Arrow is a Windows/GPU screen-rotation shortcut, not a word jump.

Alt+turn strips Alt for the opposite reason: media keycodes should never arrive modified, so Alt is
removed before the tap and restored after.

Ctrl+turn sends a word jump rather than a scroll event on purpose — Ctrl+scroll is browser/OS zoom.

## Indicators

Baseline lighting is solid warm white (hue 24, sat 60, val 150). Everything below paints over it in
`rgb_matrix_indicators_advanced_user()`.

| Indicator | Color | Location | Trigger |
|---|---|---|---|
| Caps Lock on | green | Caps key + **left** bar | host Caps Lock state |
| Num Lock on | blue | **left** bar | host Num Lock state, only if Caps and Scroll are both off |
| Scroll Lock on | red | **left** bar | host Scroll Lock state, only if Caps is off |
| SOCD armed | **magenta** | **right** bar | `socd_cleaner_enabled` |
| FN layer held | white on live keys, everything else dark | whole matrix | `layer_state_is(_FN)` |
| NKRO off, Fn held | amber | K key | `!keymap_config.nkro`, only visible while Fn is held |
| `RESET_CFG` past warning threshold | blinking red | whole matrix | held ≥1500 ms, see timing table above |

**Left-bar precedence: Caps > Scroll > Num.** One bar, three possible claimants, so only one shows
at a time — Caps Lock wins if more than one is active.

**The right bar is independent of the left bar on purpose.** It exists solely to carry the SOCD
warning. If it shared the left bar's precedence chain, an active lock state (Caps, say) could hide
an armed SOCD cleaner — exactly the failure a safety indicator can't have. Caps Lock on and SOCD
armed at once shows green on the left and magenta on the right simultaneously.

**The knob doesn't light up under Fn** — see the FN table note above; `RM_TOGG` sits at the one
position on the board with no LED.

### Indicators survive the lights being off

QMK skips `rgb_matrix_indicators_advanced_user()` entirely while the RGB matrix is disabled, and
Fn+knob disables it persistently (survives a replug). Left alone, that means both safety warnings —
the magenta SOCD bar and the red `RESET_CFG` blink — could be silently invisible: turn the lights
off, arm SOCD, and nothing shows.

`housekeeping_task_user()` fixes this: whenever SOCD is armed, or the `RESET_CFG` warning threshold
has been crossed, it force-enables the RGB matrix with `rgb_matrix_enable_noeeprom()` if it's
currently off. The `_noeeprom` variant matters — it doesn't touch the saved on/off preference, only
overrides it live. A latch (`rgb_forced_on`) tracks whether this code is the one holding the matrix
on, so that once the warning clears, the lights go back off only if this code was the one that
turned them on — pressing the real Fn+knob toggle while a warning is showing clears the latch
instead of fighting it, so your own choice about the lights is never silently reverted.

## NKRO

NKRO is **on at every boot**, and Fn+K (`NK_TOGG`) turns it off for the current session only — a
replug always brings it back on.

This is deliberately *not* `FORCE_NKRO`. QMK removed `FORCE_NKRO` in its 2026-05-31 release; if you
see it referenced anywhere (old notes, a stale example), it's dead — defining it in a keymap
`config.h` compiles cleanly and does nothing, because the lint that would catch the mistake only
scans keyboard-level config files, not keymap-level ones.

The actual mechanism is two pieces working together, both in `keymap.c`:

1. `keyboard_post_init_user()` sets `keymap_config.nkro = true` directly — a RAM write, no EEPROM
   involved — every time the board boots.
2. `NK_TOGG` is intercepted in `process_record_user()` and flips that same RAM flag directly,
   returning `false` so QMK's own `process_magic()` handler for `NK_TOGG` never runs.

The interception in step 2 is required, not optional: QMK's stock `NK_TOGG` handling in
`process_magic()` reloads `keymap_config` from EEPROM before toggling it and writes the result back
after. Left alone, that would both desync the toggle from the RAM value step 1 just set (the first
press would look like a no-op, because it's toggling a stale stored `false` up to `true`, matching
what was already live) and make the "off" state persist across a replug — the opposite of "session
only." Handling it directly avoids both.

`NKRO_DEFAULT_ON` was considered and rejected: it only applies after an EEPROM reset, and it makes
`NK_TOGG` persist across reboots, neither of which matches "on every boot, Fn+K for this session
only."

## SOCD cleaner — read this before using Fn+S

**`SOCD_CLEANER_LAST`, the resolution mode used here, is prohibited in Counter-Strike 2 and
Valorant.** Both games moved against this class of input filtering in 2024 — it's mechanically the
same technique as Razer's Snap Tap. Pascal Getreuer, whose module this firmware uses, states the
constraint directly in his own documentation: *"Counter-Strike does not allow SOCD filtering. It is
your responsibility to disable SOCD Cleaner where it is prohibited."* This paragraph exists so that
reading it a year from now re-teaches the risk rather than assuming it's remembered — check the
current rules of whatever you're about to play before arming it.

**What it does:** two axis pairs are cleaned, **A/D** and **W/S**, both using `SOCD_CLEANER_LAST`
resolution — whichever of the pair was pressed *most recently* wins, and releasing that key
reactivates the still-held opposite. This removes the dead moment that happens on a direction
reversal when both keys are briefly held together.

**How it's controlled:**

- **Boots disarmed, every time.** `socd_cleaner_enabled` is forced `false` in
  `keyboard_post_init_user()` on every boot — a power cycle is always a safe reset, and the cleaner
  can never come on by accident.
- **Fn+S (`SOCDTOG`) arms and disarms it.**
- **The right light bar glows magenta for the entire time it is armed**, so the state is visible
  before you launch anything — no need to hold a key or check a menu. See the indicator table above
  for why this is the right bar specifically, and the section above that for why it stays visible
  even with the backlight nominally off.

The module is Pascal Getreuer's `socd_cleaner`, pulled in as the community module
`getreuer/socd_cleaner` via a git submodule (`modules/getreuer`, pointing at
`getreuer/qmk-modules.git` — not `getreuer/qmk-keymap`, which nests the module two directories
deeper than the module qualifier expects and doesn't resolve). It's declared for the build in
`keymap.json`:

```json
{
    "modules": ["getreuer/socd_cleaner"]
}
```

This `keymap.json`-only approach coexists cleanly with `keymap.c` in the same directory and is what
actually builds the module in. The alternative — `COMMUNITY_MODULES += getreuer/socd_cleaner` in
`rules.mk` — was never needed.

## Mic mute (Fn+F8)

No USB HID keycode can mute a microphone — muting is an application-level concept, not something a
keyboard can do to the OS audio stack directly. Fn+F8 instead sends the key combo **Ctrl+Alt+Shift+M**,
chosen to be unlikely to collide with anything else. It does nothing on its own until you bind it as
a mute hotkey inside whatever you're using — Discord, OBS, Teams, etc. all support a custom
push-to-mute/toggle-mute hotkey in their settings; bind it to this combo there.

## VIA

`VIA_ENABLE = yes` and `DYNAMIC_KEYMAP_LAYER_COUNT 4` (base, FN, and two intentionally-blank spare
layers for experimentation) are what make VIA usable. All four layers exist fully in both
`keymaps[]` and `encoder_map[]`, including the two blank ones — VIA seeds its own EEPROM copy by
reading `keymaps[layer]` for every layer up to the declared count, so a short array would be read
past its end and the spare layers would come up full of garbage keycodes instead of transparent
ones. Wear-leveling storage was sized up (`WEAR_LEVELING_LOGICAL_SIZE 2048` /
`WEAR_LEVELING_BACKING_SIZE 4096`, from QMK's 1024-byte default) to fit VIA's dynamic keymap across
four layers — the GMMK Pro has no true EEPROM, so QMK emulates one in flash behind a wear-levelling
layer, and an undersized value fails loudly at link time rather than silently.

Connect at <https://usevia.app> in a Chromium-based browser. The official GMMK Pro VIA definition
should apply — the matrix and physical layout are unmodified stock — but this hasn't been confirmed
against four dynamic layers and the custom `RESET_CFG` keycode on hardware yet. If VIA doesn't
recognize the board, or the definition it finds doesn't line up (layer count, custom keycode), the
fallback is sideloading a matching definition through VIA's Design tab. `RESET_CFG` will most likely
show up in VIA as an unrecognized custom keycode (it's `QK_KB_0`, with no entry in the stock
definition) — that's expected and doesn't stop it from working.

## Bootloader

**Fn+PrtSc**, once this firmware is flashed.

If this is the *first* flash and the board is still running the old `gourdo1` firmware, use that
keymap's shortcut instead — **Fn+\\** or **Fn+B**.

**After that first flash, before doing anything else:** do a Clear EEPROM in QMK Toolbox, or hold
Fn+Esc for 3 seconds. This keymap's `WEAR_LEVELING_BACKING_SIZE` (4096) differs from the platform
default (2048), which relocates the emulated-EEPROM region in flash — the outgoing gourdo1
firmware's EEPROM won't validate against the new geometry, so clear it rather than let it be read
as garbage.

## Testing

A full hardware verification checklist lives at
[`testing.md`](testing.md) in this directory. Work through it top to bottom after any firmware
change that touches lighting, the SOCD cleaner, or `RESET_CFG` — later sections assume earlier ones
passed, and the whole thing finishes on confirming the board boots into its safe state (SOCD
disarmed, no stray warnings lit).
