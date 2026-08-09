# maxma keymap — GMMK Pro rev1 ANSI

A small custom keymap: one function layer, functional lighting instead of decorative animations,
and a knob that does more when a modifier is held. It replaces the `gourdo1` keymap that used to
ship on this board; nothing from that keymap was carried over except the two directly-useful ideas
(indicator lighting, a knob with modifier awareness) — reimplemented from scratch, not ported.

Firmware size at last build: **34492 bytes**.

## Base layer

Stock GMMK Pro ANSI, with two changes.

**1. The right-hand column below the knob reads Del / Ins / Home / End** instead of stock's Del /
Page Up / Page Down / End — rows 1 and 4 (Del, End) match stock; rows 2 and 3 (Ins and Home, swapped
in for Page Up and Page Down) are what changed.

**2. Y and Z are swapped.** The key right of T sends `KC_Z`; the key right of X sends `KC_Y`. This
board has German (QWERTZ) keycaps but Windows is set to a US layout, so without the swap the
legends lie — the cap printed Z would type `y`. The swap makes the output match the legends.
**Only these two keys are swapped.** Everything else stays US, so symbols, brackets and quotes are
where a US layout puts them, not where German keycaps might suggest. If you ever switch Windows to
a German layout, undo this swap or you'll be double-swapped back to wrong.

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

Seven behaviors: a plain turn/press, four modifier combinations, and Fn+press.

| Input | Result | Path | VIA-remappable? |
|---|---|---|---|
| turn | volume up / down | `encoder_map[_BASE]` | yes |
| press | mute | ordinary matrix key | yes |
| Fn + turn | RGB hue (colour), with live preview | `encoder_map[_FN]` | yes |
| Shift + turn | **select** word by word | custom, `encoder_dispatch()` | no |
| Ctrl + turn | jump word by word | custom, `encoder_dispatch()` | no |
| Alt + turn | previous / next **track** | custom, `encoder_dispatch()` | no |
| Fn + press | RGB matrix on / off | FN layer keymap entry (`RM_TOGG`) | yes |

Fn+turn needs no custom code at all: Fn is a momentary layer, so holding it already selects
`encoder_map[_FN]`. Only the modifiers need interception, because `encoder_map` has no way to see
"Ctrl is held."

### The knob logic is NOT in `encoder_update_user`. Do not move it there.

This looks like the obvious home for it, and it is wrong. With `ENCODER_MAP_ENABLE` set, QMK's
`encoder_handle()` (`quantum/encoder.c`) dispatches turns through `action_exec()` and only calls
`encoder_update_kb`/`_user` in the `#else` branch — so with the map enabled **that hook is never
invoked**. An implementation there compiles cleanly, passes review, and silently does nothing.

It did exactly that here. Ctrl+turn and Alt+turn were written, reviewed three times, shipped, and
never once ran. The bug surfaced only when the hue preview — added later to the same dead function
— also failed to appear on hardware.

Encoder turns instead arrive in `process_record_user` as keyrecords whose `event.type` is
`ENCODER_CW_EVENT` or `ENCODER_CCW_EVENT`, which is where `encoder_dispatch()` handles them. Each
turn fires a press *and* a release, so anything that consumes the press must consume the matching
release too, or the encoder map's keycode gets registered and never released.

**Shift+turn is checked before Ctrl+turn** so that Ctrl+Shift+turn extends the selection rather
than falling through to a plain word jump. It adds Ctrl only when Ctrl isn't already physically
held — otherwise the cleanup would release the user's real Ctrl.

**Ctrl+turn taps a bare arrow key**, not `C(KC_RGHT)`. Ctrl is already physically held down, so the
bare arrow picks the modifier up on its own. Wrapping it in `C(...)` would make QMK release the held
Ctrl the instant the tap ends, desyncing it from the key that's still physically down. The code does
strip any *other* held modifier first, though — a bare arrow otherwise collects every modifier
currently down, and Ctrl+Alt+Arrow is a Windows/GPU screen-rotation shortcut, not a word jump.

Alt+turn strips Alt for the opposite reason: media keycodes should never arrive modified, so Alt is
removed before the tap and restored after.

Ctrl+turn sends a word jump rather than a scroll event on purpose — Ctrl+scroll is browser/OS zoom.

## Indicators

Baseline lighting is a solid colour, hue 24 / sat 255 / val 150 — orange. Everything below paints
over it in `rgb_matrix_indicators_advanced_user()`.

Saturation is deliberately full rather than the pale warm white this started as. At sat 60 the board
is ~90% white, so rotating the hue only nudges the tint, and the brightness difference between pale
yellow and pale blue makes the hue knob feel like a brightness dial. Turn saturation down in VIA if
you want the pastel look back, and accept that the knob stops looking like it does anything.

| Indicator | Color | Location | Trigger |
|---|---|---|---|
| Caps Lock on | green | **Caps key** | host Caps Lock state |
| Num Lock on | blue | **N key** | host Num Lock state |
| Scroll Lock on | red | **F9 key** | host Scroll Lock state |
| SOCD armed | **magenta** | **W A S D** | `socd_cleaner_enabled` |
| FN layer held | white on live keys, everything else dark | whole matrix | `layer_state_is(_FN)` |
| NKRO off, Fn held | amber | K key | `!keymap_config.nkro`, only visible while Fn is held |
| `RESET_CFG` past warning threshold | blinking red | whole matrix | held ≥1500 ms, see timing table above |

**Nothing paints the side bars.** Both stay on the baseline colour so the left and right sides always
match. Every indicator sits on the key it refers to instead.

That is a deliberate reversal. The lock states originally lived on the left light bar and SOCD on
the right, which meant the two sides were usually different colours for no reason the eye could
read — and worse, Num Lock is on by default on most PCs, so the left bar was permanently blue. An
indicator that is lit almost all the time carries no information. Putting each one on its own key
means it only appears when it means something, and where you'd look for it.

**No precedence chain is needed any more.** Caps, Num, Scroll and SOCD are on different keys, so any
combination shows at once without competing.

**The SOCD warning is still non-suppressible.** WASD is painted last in the loop, after the FN
blackout, so holding Fn cannot hide it — and `housekeeping_task_user` still forces the matrix on if
the lighting is off while SOCD is armed.

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
- **W, A, S and D glow magenta for the entire time it is armed**, so the state is visible before you
  launch anything — no need to hold a key or check a menu. The warning sits on the four keys the
  cleaner actually alters, which is both where you'll look and a reminder of what it's doing. See
  the indicator section above for why it stays visible even with the backlight nominally off.

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

## Flashing: read this or you will lose an hour

**With VIA enabled, the board does not type from `keymap.c`.** It types from a copy of the keymap
held in EEPROM. The compiled `keymaps[]` only *seeds* that copy, once, when the EEPROM is
initialised. The same is true of `encoder_map`.

The consequence is deeply counter-intuitive: **change a key, rebuild, flash — and nothing happens.**
The new firmware is genuinely on the board, and the board goes on serving the old keymap out of
EEPROM. It looks exactly like a failed flash. It is not.

Worse, `Fn+Esc` held for 3 seconds — the built-in EEPROM wipe — did **not** reliably reseed in
practice. Three separate keymap changes were flashed and silently ignored before this was diagnosed.

**So: whenever you change `keymap.c`, `config.h`, or `encoder_map`, flash with a mass erase.**

```bash
qmk compile -kb gmmk/pro/rev1/ansi -km maxma

# board in bootloader, then:
dfu-util -a 0 -d 0483:df11 -s 0x08000000:mass-erase:force \
         -D gmmk_pro_rev1_ansi_maxma.bin
```

`mass-erase:force` wipes the entire flash, EEPROM region included, so there is nothing left to
reseed *from* and the board is forced to rebuild the keymap from the firmware you just wrote. Then
unplug and replug — `dfu-util` leaves the board in DFU otherwise.

Plain `qmk flash` is fine when you have only changed code that isn't the keymap or the encoder map
— indicator logic, `process_record_user`, `housekeeping_task_user`. When in doubt, mass erase; the
only cost is losing your VIA remaps and lighting settings.

This also covers the first flash from the old `gourdo1` firmware: this keymap's
`WEAR_LEVELING_BACKING_SIZE` (4096) differs from the platform default (2048), which relocates the
emulated-EEPROM region, so the outgoing firmware's EEPROM would not validate anyway.

## Cheatsheet

A one-page printable summary of every binding lives at
[`docs/cheatsheet.pdf`](../../../../../../docs/cheatsheet.pdf) in the repo root.

Regenerate it after changing any binding — the HTML source sits next to it:

```bash
chrome --headless=new --no-pdf-header-footer \
       --print-to-pdf=docs/cheatsheet.pdf docs/cheatsheet.html
```

## Testing

A full hardware verification checklist lives at
[`testing.md`](testing.md) in this directory. Work through it top to bottom after any firmware
change that touches lighting, the SOCD cleaner, or `RESET_CFG` — later sections assume earlier ones
passed, and the whole thing finishes on confirming the board boots into its safe state (SOCD
disarmed, no stray warnings lit).
