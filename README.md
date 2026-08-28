# Colacut

A real-time time stretcher, pitch shifter and transient-driven auto-slicer —
running as a VST3 / AU plug-in, and as firmware for the Hermetic Modular
Alchemy Lab V2.

**Colacut is a fork of [Capicola](https://github.com/heavylight-industries/capicola)
by [Heavylight Industries](https://github.com/heavylight-industries).** The
keyframe time-stretching engine, the DSP core and the research behind them are
their work, not mine. What this fork adds is the plug-in build. See
[Credit](#credit) for the paper, the manual and the audio examples.

## What it does

Incoming stereo audio is stretched out, and in parallel its envelope and
transients are extracted. When a transient crosses the threshold, the lagging
read head snaps to the current time with a short crossfade, and the stretch
carries on from that new position. The result is time stretching that keeps the
rhythm of the input instead of smearing it — you can pull a loop far out of
time and still hear its hits land where they should.

Pitch is independent of time, so the two can be pushed in opposite directions.

The envelope followers and transient detectors aren't buried inside the engine.
Every performance control is normalised to them, so the dynamics of the signal
can modulate the controls as it plays — a self-patching arrangement that gets
strange quickly and rewards it. On the hardware the same envelopes and triggers
are available on the CV outputs to drive other modules; in the plug-in they
drive the controls and the panel animation.

The keyframe method itself is described in the DAFx26 paper linked below. The
core of it lives in `lib/KeyframeRecorder.h`, which works for offline recording
and playback as well as real-time use.

`lib/` also holds several pieces that stand on their own:

- Vadim Zavalishin-style zero-delay-feedback filters (1-pole, state variable)
- A delay line offering Hermite and B-spline interpolated reads from a ring
  buffer, plus 1st and 2nd derivatives and the Teager-Kaiser Energy Operator
- Tabulated function storage for cheap `tanh()`, `sin()` and friends
- A TKEO-driven peak detector with adaptive thresholding

**Panel reference:** [MANUAL.md](MANUAL.md) — pages, buttons, jack map, fixed
internals. Heavylight Industries also publish an
[interactive manual](https://heavylight-industries.github.io/capicola/manual.html)
with a live faceplate render.

## What this fork changes

Forked from upstream `93060e2` (24 August 2026). Changes since:

- **Added `plugin/`** — a JUCE VST3 / AU / Standalone shell over `lib/`, with
  its own faceplate, a modulation matrix routing the envelope followers to the
  performance controls, and MIDI note-on as the slice trigger. Upstream is
  firmware only. See [plugin/README.md](plugin/README.md).
- **Renamed** the product from Capicola to Colacut, and the vendor from
  Heavylight Industries to Sungam, in the plug-in identity and user-facing
  documentation. Code identifiers, file names and the `capicola` namespace are
  untouched, so the tree still diffs cleanly against upstream.

The firmware, `lib/` and `src/` are otherwise unmodified.

## Layout

- `plugin/` — the VST3 / AU build (this fork).
- `src/main.cpp` — the entire hardware UI, on the Alchemy Lab SDK.
- `src/audio/` — `AudioEngine`: top-level objects and audio routing.
- `lib/` — the DSP core.
- `lib/alchemy-sdk/` — the SDK (submodule); vendors libDaisy under
  `lib/alchemy-sdk/vendor/libDaisy`.

## Build — plug-in

Needs CMake ≥ 3.22 and a JUCE checkout (defaults to `/Applications/JUCE`).

```sh
cmake -S plugin -B plugin/build -DCMAKE_BUILD_TYPE=Release
cmake --build plugin/build --config Release -j8
```

VST3, AU and a standalone app are copied into the user plug-in folders. On
macOS the result is a universal `arm64;x86_64` binary — verify with:

```sh
lipo -archs ~/Library/Audio/Plug-Ins/VST3/Colacut.vst3/Contents/MacOS/Colacut
```

## Build — firmware

Requires `cmake ≥ 3.21`, `ninja`, `arm-none-eabi-gcc` (builds with 10.2.1; the
SDK nominally asks for ≥ 12), and `dfu-util` for flashing.

```sh
git submodule update --init --recursive    # first checkout only
cmake --preset arm
cmake --build --preset arm
```

Outputs `build-arm/capicola.{elf,bin,hex}`.

The `arm-bench` preset builds the same image with a 1 Hz CPU/CV serial log on
the front USB instead of HostLink (they share the CDC port), into
`build-arm-bench/`.

### Flash (Daisy bootloader / front USB-C)

Put the module in update mode — hold **B3** through the ~2 s boot window, or
hold while powering on — then:

```sh
cmake --build --preset arm --target capicola-flash
```

`...-size` prints the memory footprint.

## Credit

Capicola, the keyframe time-stretching engine and the research behind it are by
Heavylight Industries:

- **Original project** — [heavylight-industries/capicola](https://github.com/heavylight-industries/capicola)
- **Paper** — [*Keyframe Time Stretching via Extrema Sampling*](https://github.com/heavylight-industries/dafx26-paper), DAFx26
- **Interactive manual** — [heavylight-industries.github.io/capicola](https://heavylight-industries.github.io/capicola/manual.html)
- **Audio examples** — [time-stretching-examples](https://heavylight-industries.github.io/time-stretching-examples/)
- **Video** — [Dialectric Studios](https://www.youtube.com/@dialectricStudios)

If you want the original module, go to them, not here. This fork exists to run
that engine in a DAW.

## License

Colacut is free software under the **GNU Affero General Public License v3.0** —
see [LICENSE](LICENSE) — the same licence as upstream Capicola, as the AGPL
requires. If you ship hardware or a service running a modified Colacut, you
must offer the corresponding source.

Third-party code keeps its own terms: `lib/alchemy-sdk` and the libDaisy it
vendors are MIT-licensed; see their `LICENSE` files.
