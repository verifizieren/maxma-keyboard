# GMMK Pro "maxma" — hardware test pass

One consolidated run, since per-task hardware gates were batched to the end. Work top to bottom;
later sections assume earlier ones passed.

**Before you start:** your board is still running the gourdo1 firmware, so the bootloader shortcut
for this *first* flash is **Fn+\\** (or Fn+B). After this flash it becomes **Fn+PrtSc**.

**Immediately after that first flash, before starting the checklist below:** do a Clear EEPROM in
QMK Toolbox, or hold Fn+Esc for 3 seconds. This keymap sets `WEAR_LEVELING_BACKING_SIZE 4096`
against the platform default of 2048, which moves the emulated-EEPROM region to a different offset
in flash — the outgoing gourdo1 firmware's EEPROM contents won't validate against the new geometry,
so start from a known-clean slate rather than carrying over whatever garbage that mismatch produces.

If something fails, note which item and stop rather than continuing — the failures downstream of a
broken indicator are hard to read.

---

## 1. Base layer

- [ ] Right column, top to bottom below the knob, types: **Del, Ins, Home, End**
- [ ] Print Screen (left of the knob) still takes a screenshot
- [ ] Alphas, numbers, symbols, both shifts, arrows all type correctly
- [ ] Caps Lock toggles caps normally

## 2. Lighting baseline

- [ ] Board comes up **warm white**, solid, not an animation and not the old firmware's colours
- [ ] Fn+`-` dims, Fn+`=` brightens
- [ ] The brightness you set survives a replug
- [ ] Fn+knob-press turns the backlight off
- [ ] Press Fn+knob-press again → **backlight back on**. Leave it on — §8 below assumes the lights
      are on when it starts

## 3. NKRO

- [ ] Straight after a replug, **NKRO is already on** — check at <https://config.qmk.fm/#/test> or
      any rollover tester. It should be on without you touching anything
- [ ] Fn+K turns it off
- [ ] Replug → **on again**. Fn+K is session-only by design; it must not persist

## 4. FN layer keys

Hold Fn and press each:

- [ ] F1 / F2 / F3 / F4 → previous, play-pause, next, stop (have a music player running)
- [ ] F5 / F6 / F7 → mute, volume down, volume up
- [ ] F8 → sends **Ctrl+Alt+Shift+M**. Nothing visible happens yet — confirm with a key tester.
      This is the mic-mute hotkey to bind in Discord/OBS later
- [ ] F9 → Scroll Lock toggles
- [ ] F10 → Pause/Break
- [ ] Backspace → deletes **forward**, like Delete
- [ ] N → Num Lock toggles
- [ ] ↑ ↓ ← → → PgUp, PgDn, Home, End
- [ ] Ins and Home in the right column → PgUp, PgDn
- [ ] **PrtSc → board enters bootloader.** Confirm this works before you rely on it. Unplug and
      replug to come back out

## 5. Indicators

- [ ] Caps Lock on → **Caps key green** *and* **left bar green**; off → both back to warm white
- [ ] Num Lock on (Caps off) → left bar **blue**
- [ ] Scroll Lock on → left bar **red**
- [ ] Caps + Num both on → left bar is **green, not blue**. One bar, three claimants; Caps wins by
      design
- [ ] Hold Fn → board goes dark except **24 white keys**

      Count carefully. The FN layer has 25 things on it, but one is `RM_TOGG` on the **knob**, and
      the encoder is the single key position with no LED behind it. **The knob staying dark is
      correct**, not a fault.

- [ ] Fn+K to disable NKRO, keep holding Fn → the **K key is amber**, not white
- [ ] Re-enable NKRO → K back to white

## 6. The knob

In a text editor with a paragraph of text, music playing:

- [ ] Turn, nothing held → volume
- [ ] Press → mute
- [ ] Hold **Fn**, turn → brightness
- [ ] Hold **Ctrl**, turn → cursor jumps **word by word**, both directions
- [ ] **Release Ctrl, then type normally — no stuck Ctrl.** This one matters; it's the specific
      failure the implementation is shaped to avoid
- [ ] Hold **Alt**, turn → track changes
- [ ] **Release Alt — the Windows menu bar must not activate, and Alt must not stick.** Flagged in
      review as genuinely uncertain and only testable here. If the menu bar pops, tell me
- [ ] Hold **Ctrl+Alt together**, turn → word jump, and **your screen must not rotate.** Ctrl+Alt+
      Arrow is a Windows/GPU shortcut; the code strips the extra modifier to prevent it

## 7. SOCD cleaner

Test in a text editor first — holding a key shows the repeat, which makes the behaviour visible
without launching a game.

- [ ] **Replug. Without pressing anything:** hold `A`, then press `D` while still holding `A` →
      `a` keeps repeating, `d` does **not** take over. **It must boot disarmed.** This is the
      safety property — if it fails, stop and tell me
- [ ] Right light bar is **not** magenta at boot
- [ ] Fn+S → right bar goes **magenta**
- [ ] Hold `A`, press `D` while holding → output switches to `d` immediately
- [ ] Release `D` while still holding `A` → back to `a`. That reactivation is the whole point of
      `LAST` mode
- [ ] Same for `W`→`S` and `S`→`W`
- [ ] Hold `J` and `K` together → both repeat normally. Only the two configured pairs are affected
- [ ] Turn Caps Lock on while SOCD is armed → **left bar green AND right bar magenta at the same
      time.** They're independent so a lock state can never hide the SOCD warning
- [ ] **Turn the backlight off with Fn+knob, then press Fn+S to arm.** The lights must come back on
      by themselves and show magenta. This was a real hole found in review — the warning must not be
      suppressible
- [ ] Fn+S to disarm → magenta clears, and the backlight returns to off if that's where you left it
- [ ] **Replug → disarmed again**, without pressing anything

## 8. RESET_CFG (Fn+Esc)

Safe paths first.

- [ ] Change the colour in VIA or the brightness with Fn+`=`. **Tap** Fn+Esc → warm white at default
      brightness returns
- [ ] Turn the backlight off with Fn+knob, then **tap** Fn+Esc → the backlight comes back **on** at
      warm white, default brightness. The tap forces the matrix on before applying the defaults —
      "on" is part of what a compiled-default reset restores, not a state it leaves alone
- [ ] Replug → the reset persisted
- [ ] Hold Fn+Esc and watch: nothing for ~1.5s, then the board **blinks red**
- [ ] **Release at the first red blink → nothing happens.** This is the escape hatch and the single
      most important item here. The warning starts 500ms before the wipe threshold precisely so
      that letting go lands in the dead band
- [ ] Hold Fn+Esc a full 3 seconds, release → board reboots, lighting back to defaults

Now the destructive one. **Set a canary first.**

- [ ] In VIA, remap some key you'll notice
- [ ] Hold Fn+Esc 3s again → after the reboot, **the canary remap is gone**. That proves the wipe
      reaches VIA's dynamic keymap, not just the lighting
- [ ] NKRO is still on after the wipe

## 9. VIA

Open <https://usevia.app> in a Chromium browser and authorise the device.

- [ ] VIA connects and identifies the board
- [ ] It shows **four** layers
- [ ] Remap a key on layer 2, replug → it persisted
- [ ] **Remap something on the FN layer, then hold Fn** → the white highlight **follows your
      remap**. The indicator reads the live keymap rather than a hard-coded list, so this should
      just work
- [ ] Undo the test remaps

If VIA can't find a definition: the layout is unmodified stock, so the official GMMK Pro entry
should apply. If it doesn't, we sideload a tweaked copy through VIA's Design tab. Tell me and I'll
sort it.

`RESET_CFG` on Fn+Esc will likely show in VIA as an unknown custom keycode — expected, since it's a
`QK_KB_0` custom keycode with no entry in the stock definition. It still works.

## 10. Finish on the safe state

- [ ] Replug one last time. Right bar **not** magenta, and holding `A` then `D` leaves `a` winning.

The last thing verified should be that the board boots safe.
