# `libsstvenc`: A simple SSTV encoder library in C.

**THIS PROJECT IS A WORK-IN-PROGRESS**.

The idea of this project is to provide a reasonable quality SSTV encoder
implementation in C for use in custom SSTV applications.

## Implemented modes:

- Robot 8, 12 and 24 monochrome modes
- Robot 36 and 72 colour modes
- Scottie S1, S2 and DX
- Martin M1 and M2
- Pasokon P3, P5 and P7
- PD-50, PD-90, PD-120, PD-160, PD-180, PD-240 and PD-290
- Wraase SC-2 120 and 180

## Current work:

### Fixing timing issues

After recent work, many of the modes are working as they should:

- Robot monochrome modes are basically spot on, save some offset issues which
  could just be receiver-related.
- Robot colour modes work, sometimes with an offset and some colour alignment,
  but again re-playing the same recording seems to generate a "better" image,
  so this could just be the decoder.
- Scottie S1 is slanted (forward-lean), worse in `slowrx` than in QSSTV.
- Scottie S2 is basically fine, save an offset to the right.
- Scottie DX seems spot on.
- Martin M1 and M2 are fine, save a small offset.
- Pasokon P3 is basically spot on, P5 has a small offset, P7 has a slight
  backward slant.
- PD modes seem to be fine aside from an offset to the right in some cases.
- Wraase SC-2: 120 mode has a slight forward-slant, 180 mode has a slight
  offset.

Still working on issues as I find them, but we're a lot closer now.
