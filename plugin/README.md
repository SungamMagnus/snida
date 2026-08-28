# Colacut — VST3 / AU

Part of [Colacut](../README.md), a fork of
[Capicola](https://github.com/heavylight-industries/capicola) by Heavylight
Industries. The DSP core under `lib/` is theirs; this plug-in shell is the
part this fork adds.

The Alchemy Lab module as a plugin. The DSP is the firmware's, unchanged:
`plugin/Source/CapicolaEngine.cpp` is a port of `src/audio/audio_engine.cpp`
with the Daisy-specific parts (SDRAM globals, `CpuLoadMeter`, the fixed
48 kHz / 64-sample callback) swapped for heap storage, the host sample rate and
internal 64-sample chunking. Everything in `lib/` is used verbatim.

## Build

Needs CMake ≥ 3.22 and a JUCE ≥ 8 checkout (tested against JUCE 9.0.1).

```sh
cmake -S plugin -B plugin/build -G Xcode -DJUCE_DIR=/Applications/JUCE
```

```sh
cmake --build plugin/build --config Release
```

`JUCE_DIR` defaults to `/Applications/JUCE`. Builds VST3, AU and a standalone
app into `plugin/build/Capicola_artefacts/Release/`, and copies the VST3 and AU
into the user plug-in folders.

On macOS the build is universal (`arm64;x86_64`, minimum 11.0), pinned in
CMakeLists before `project()`. Do not drop that: left to itself CMake targets
its own architecture, and an x86_64 CMake — Intel Homebrew running under
Rosetta — yields an Intel-only plug-in. Native arm64 hosts then skip it
silently while `auval` still passes under translation, which looks exactly like
a signing or quarantine problem and isn't one. Check a build with:

```sh
lipo -archs ~/Library/Audio/Plug-Ins/VST3/Colacut.vst3/Contents/MacOS/Colacut
```

Add `-DCAPICOLA_PANEL_SHOT=ON` for `panel_shot`, a console tool that renders the
panel to a PNG without a host — useful when working on the layout.

## Panel

One surface, 960 x 468, flat and typographic — bone paper, ink, monospace,
hairline rules. Colour is never decoration: a hue means "this control belongs to
that section", or it is a live signal. [MANUAL.md](../MANUAL.md) is the
reference for what every control does.

Three sections, titled vertically down their own edge:

- **PERFORM** — pitch (knob) and the five performance faders.
- **MODULATION** — the matrix: one row per perform control, `target -> source ->
  depth`. Source is a three-position toggle with every position labelled.
- **VOICE** — character (knob) and the five voicing faders.

Bipolar controls are knobs and unipolar ones are faders, so the shape tells you
whether a control has a meaningful centre. Knobs keep the module's own pot
sweep (111.4 deg to 428.6 deg), so noon lands where the hardware's does, and the
arc grows from noon rather than from the CCW stop.

The perform knob and faders track their *modulated* position — the routing
matrix pushes them live, exactly as the LED rings move on the module.

The status bar along the bottom is entirely live: input and output envelope
meters with their gate lamps, the bipolar mod-wheel value, clip, and the current
reported latency. There are no decorative elements and no fake jacks.

| Gesture | Effect |
|---|---|
| Drag a fader | 1:1 with the visible travel; hold Shift for fine |
| Drag a knob | 150 px for a full sweep; Shift for fine |
| Click a toggle position | Selects that source |
| Double-click | Back to default |
| Scroll | Adjust |

## Differences from the module

- **The input jacks are MIDI.** Any note-on slices; the mod wheel (CC 1) is the
  bipolar **MOD** source. The module's four signal outputs stay internal, where
  the modulation matrix can still reach them, and are mirrored in the status
  bar.
- **Cutoffs are Hz, not normalised fc.** Env smoothing and feedback tone track
  the host sample rate instead of assuming 48 kHz; so do the detector's gate
  and hold-off timings.
- **Latency is reported.** The wet path lags the dry by the crossfade, so the
  Fade fader moves the plugin's reported latency with it, and the host
  compensates. Mid-mix comb filtering against sustained material is the
  module's behaviour and is preserved.
- **Keyframe rings are 2^20 frames per channel** (24 MB total) rather than the
  module's 2^21 in SDRAM.
- No pages, no pot catch, no preset store — every control is visible at once
  and the host owns state.

## The one firmware change

`lib/KeyframeRecorder.h`'s `Init()` gained an optional sample-rate argument,
defaulted to 48000 so every existing caller is unaffected. Without it the
transient detector's gate width, hold-off and averaging window are hard-wired to
48 kHz and mis-scale at any other rate.
