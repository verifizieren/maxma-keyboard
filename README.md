# maxma-keyboard

Custom QMK firmware for a Glorious GMMK Pro rev1 ANSI (`gmmk/pro/rev1/ansi`), keymap `maxma`. This
is a [QMK userspace](https://docs.qmk.fm/newbs_external_userspace) repository — it holds only the
owner's own keymap and configuration, not a fork of QMK itself.

See [`keyboards/gmmk/pro/rev1/ansi/keymaps/maxma/readme.md`](keyboards/gmmk/pro/rev1/ansi/keymaps/maxma/readme.md)
for what the keymap actually does — layer tables, the knob, the RGB indicators, NKRO behavior, and
the SOCD cleaner (read that section before arming it). The hardware verification checklist is at
[`keyboards/gmmk/pro/rev1/ansi/keymaps/maxma/testing.md`](keyboards/gmmk/pro/rev1/ansi/keymaps/maxma/testing.md).

## Layout

```
C:\Users\maxma\source\
  qmk_firmware\        2023 QMK reference clone. Read-only, never modified.
  qmk_firmware_new\    Current upstream QMK, pristine. Updated with `git pull`. Never edited.
  maxma-keyboard\      This repo. github.com/verifizieren/maxma-keyboard
```

Three directories, no overlap. `maxma-keyboard` is pointed at the QMK checkout once via QMK's
userspace config (`qmk config user.overlay_dir=...`, see setup below); QMK reads this repo's
`qmk.json` for build targets and compiles from `qmk_firmware_new`, but nothing owned by this repo
ever lands inside the QMK clone. That's the reason for the split: because no file from here is ever
written into `qmk_firmware_new`, updating QMK is a plain `git pull` there — it cannot produce a merge
conflict, since there is nothing local for it to conflict with.

```
maxma-keyboard/
  qmk.json                                   build target list: gmmk/pro/rev1/ansi:maxma
  README.md                                  this file
  .github/workflows/build_binaries.yaml      CI: builds the target and publishes a Firmware artifact
  modules/getreuer/                          git submodule (getreuer/qmk-modules) — SOCD cleaner
  keyboards/gmmk/pro/rev1/ansi/keymaps/maxma/
    keymap.c                                 layers, encoder, indicators, custom keycode, SOCD pairs
    keymap.json                              declares the getreuer/socd_cleaner community module
    config.h                                 compile-time configuration
    rules.mk                                 feature toggles
    readme.md                                keymap documentation — layer tables, knob, indicators
    testing.md                               hardware verification checklist
```

## One-time setup

Requires `qmk` installed and set up already (`qmk setup`, see the
[QMK docs](https://docs.qmk.fm/newbs_getting_started)). Then point QMK's userspace config at this
clone:

```
qmk config user.overlay_dir=C:/Users/maxma/source/maxma-keyboard
```

This repo also uses a git submodule (`modules/getreuer`, the SOCD cleaner module) — clone with
`--recurse-submodules`, or run `git submodule update --init --recursive` afterward if already
cloned.

## Build

```
qmk compile -kb gmmk/pro/rev1/ansi -km maxma
```

If `qmk` isn't on `PATH` directly (QMK MSYS on Windows), invoke it through the MSYS shell instead —
note the single quotes; PowerShell corrupts `$(...)` inside double-quoted strings:

```powershell
$env:MSYSTEM='UCRT64'; $env:MSYS2_PATH_TYPE='inherit'; & "C:\QMK_MSYS\usr\bin\bash.exe" -lc 'cd /c/Users/maxma/source/maxma-keyboard && qmk compile -kb gmmk/pro/rev1/ansi -km maxma'
```

## Flash

```
qmk flash -kb gmmk/pro/rev1/ansi -km maxma
```

or load the built `.bin` in QMK Toolbox. The board must be in its bootloader first — **Fn+PrtSc**
once this firmware is already flashed, or **Fn+\\** / **Fn+B** if it's still running the old
`gourdo1` firmware for the very first flash.

## CI

Every push builds `gmmk/pro/rev1/ansi:maxma` via the reusable QMK userspace workflows
(`qmk/.github`) and publishes the result as a `Firmware` artifact on the run. No manual `submodules:`
checkout step is needed — the reusable build workflow handles submodule checkout itself.

## Updating QMK

```
cd C:\Users\maxma\source\qmk_firmware_new
git pull
```

Safe by construction: nothing owned by this repo is ever written into that clone, so there is
nothing there to conflict with an upstream pull.
