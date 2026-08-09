# Feature set for the maxma keymap.
# Board-level features (RGB matrix, encoder, NKRO capability) come from keyboard.json.

VIA_ENABLE = yes           # runtime remapping
ENCODER_MAP_ENABLE = yes   # per-layer encoder, and the VIA-remappable path
LTO_ENABLE = yes           # link-time optimisation; meaningful saving on this MCU

CONSOLE_ENABLE = no
COMMAND_ENABLE = no
MOUSEKEY_ENABLE = no       # no mouse-key layer in this keymap
