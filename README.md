# `libsstvenc`: A simple SSTV encoder library in C.

**THIS PROJECT IS A WORK-IN-PROGRESS**.

The idea of this project is to provide a reasonable quality SSTV encoder
implementation in C for use in custom SSTV applications.

- Source code: https://github.com/sjlongland/libsstvenc or
  https://codeberg.org/sjlongland/libsstvenc
- Documentation: https://static.vk4msl.com/projects/libsstvenc/

## SSTV Modes

### Implemented modes:

- Robot 8, 12 and 24 monochrome modes
- Robot 36 and 72 colour modes
- Scottie S1, S2 and DX
- Martin M1 and M2
- Pasokon P3, P5 and P7
- PD-50, PD-90, PD-120, PD-160, PD-180, PD-240 and PD-290
- Wraase SC-2 120 and 180

### Future possible modes

- Robot 24 Colour
- Martin M3, M4

Maybe some of the exotic modes supported by MMSSTV and QSSTV, if specifications
can be found for them?

### Not in scope

- HamDRM digital SSTV: this is basically a file transfer mechanism using an
  adapted form of Digital Radio Mondiale, much more complex OFDM transmission
  system.  It'd be nice to have, but should be made into its own project.

## Current work:

### Packaging

Right now there's just a top-level `Makefile` which can compile a shared object
library.  It's pretty sparse.  As things mature, it would be good to offer
`.deb` packages for `amd64`, `arm64` and `armhf` platforms, perhaps `.rpm`
packages too.

A CLI application that can put the library through its paces more sophisticated
than the example `png-to-sstv.c` distributed will probably be the next focus.

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

## Platform/OS support

### Tier 1

These are platforms I use regularly for this application and/or I use to
directly test and develop this software.

- Debian GNU/Linux `arm64` (Raspbian 64-bit)
- Gentoo Linux `amd64`

### Tier 2

These are platforms I have access to, and can dig out and run tests on, but
don't use regularly, so will see less frequent testing (or may be completely
untested).

- Red Hat-derived Linux distributions
- Debian/Ubuntu `amd64`, `armhf`, `armel`, `i386`
- AlpineLinux `amd64`, `arm64`, `i386`
- Other Linux distributions (e.g. Slackware, Arch)
- DragonFlyBSD `amd64`
- FreeBSD `amd64`, `arm64`, `armhf`, `i386`
- NetBSD `amd64`, `arm64`, `armhf`, `i386`
- OpenBSD `amd64`, `arm64`, `i386`, `mips64el`
- Other BSD variants
- Apple MacOS X 10.6 `amd64`

### Tier 3

These are platforms I do not have access to and thus require feedback from
people who use those platforms.  Patches for these platforms are accepted on
the proviso the patches do not break Tier 1 or Tier 2 platforms, or make
maintenance cumbersome.

- Apple MacOS X later than 10.6 or platforms other than `amd64` (e.g. `arm64`
  and `ppc64`)
- Microsoft Windows
