# Sníða — interactive manual copy

> **Generated file — do not edit.** Regenerating overwrites it.
> The copy lives in `src/capicola_manual.cpp`; edit there, then rerun
> `tools/manual_copy.py`. This is the exact text the programmer on
> hermeticmodular.com renders.

**Tagline** — Transient-locked keyframe time stretcher

## Preamble

Incoming stereo is stretched, and in parallel its envelope and transients are extracted. When a transient passes the threshold, the lagging read head snaps to the current time with a short crossfade and stretches on from there — time stretching that stays true to the rhythm of the input. Pitch and time are fully decoupled.

The engine is [Keyframe Time Stretching via Extrema Sampling](https://github.com/heavylight-industries/dafx26-paper), DAFx26.

## Section — Getting Around  `#getting-around`

Four pages in two groups. **B1** walks Primary → Depth, **B3** walks Routing → Secondary. Each group remembers where it was left, so the other group's button jumps back to where you were.

Every page stores its own value per knob. After a page switch a knob stays decoupled until the pot sweeps through the stored value — the ring shows a pip there — then it catches. Nothing jumps. Routing catches per zone.

## Section — Routing  `#routing`

Each pot picks the modulation source for the same-numbered Primary knob, CCW → CW: input follower, output follower, CV In. The ring shows all three zones dim with the selected one lit, so it doubles as the position map.

The default is the input follower, so the module self-patches out of the box: every performance control is normalized to the envelope followers, and the dynamics of the signal modulate them in real time. The output follower is the wilder option — it hears what the module is already doing, so depth alone closes a loop.

Both followers are the detectors' own smoothed TKEO envelopes — input side is max of L/R, output side runs on the post-mix sum. The output follower never triggers a slice; it is a signal source only.

## Section — Reading the Panel  `#panel`

All three buttons wear the visible page's hue and are never off, so activity reads as a **dark flash**: B2 blinks off on a slice, and all three blink three times to confirm a save or a reset.

The pip at six o'clock on **P3**–**P6** mirrors the four CV outputs — Trig 1, Trig 2, Env 1, Env 2. Env 1 turns red when the input clips.

## Primary page knobs

### Pitch  `perf.pitch` — linear -12…12 st
Grain pitch, decoupled from time. A detent at noon snaps to true unity.

*See also: perf.stretch, depth.p1*

### Stretch  `perf.stretch` — 0…100 %
Playback rate of the keyframe grid: realtime at CCW, a **true freeze** at the stop — the grid stops rather than crawling. Triggers still re-anchor it while frozen.

*See also: perf.pitch, depth.p2*

### Threshold  `perf.threshold` — 0…100 %
Self-calibrating: events are kept at `knob × running average` of the detector's own envelope. **The top of the knob mutes auto-triggering** — only B2 or Trig In slices.

*See also: trig.in, b2*

### Grain Size  `perf.grain` — linear 32…4096
How much keyframe material a grain covers before the head is sliced on.

### Quality  `perf.quality` — 0…100 %
Analyzer keyframe threshold. CW is full fidelity; CCW drops keyframes for a sparse, crunchy reconstruction. Lower quality = longer buffer.

### Feedback  `perf.feedback` — linear 0…1.5
Loop gain. Wet output → sinc saturator → bandpass → back into the recorder, where it is re-sliced and re-pitched. **Above unity is deliberately unstable.** The tap sits before the mix blend, so Mix down does not starve it.

*See also: perf.threshold*

## Depth page knobs

### P1 Depth  `depth.p1` — linear -1…1
Depth for **Pitch**.

*See also: perf.pitch*

### P2 Depth  `depth.p2` — linear -1…1
Depth for **Stretch**.

*See also: perf.stretch*

### P3 Depth  `depth.p3` — linear -1…1
Depth for **Threshold**.

*See also: perf.threshold*

### P4 Depth  `depth.p4` — linear -1…1
Depth for **Grain Size**.

*See also: perf.grain*

### P5 Depth  `depth.p5` — linear -1…1
Depth for **Quality**.

*See also: perf.quality*

### P6 Depth  `depth.p6` — linear -1…1
Depth for **Feedback**.

*See also: perf.feedback*

## Routing page

### P1 Source  `route.p1` — In Env / Out Env / CV In
Source for **Pitch**.

*See also: perf.pitch*

### P2 Source  `route.p2` — In Env / Out Env / CV In
Source for **Stretch**.

*See also: perf.stretch*

### P3 Source  `route.p3` — In Env / Out Env / CV In
Source for **Threshold** — a follower steering the detector's own threshold is where the module starts breathing.

*See also: perf.threshold*

### P4 Source  `route.p4` — In Env / Out Env / CV In
Source for **Grain Size**.

*See also: perf.grain*

### P5 Source  `route.p5` — In Env / Out Env / CV In
Source for **Quality**.

*See also: perf.quality*

### P6 Source  `route.p6` — In Env / Out Env / CV In
Source for **Feedback**. The output follower acts as a governor here: the louder it gets, the further the loop gain can go before it runs.

*See also: perf.feedback*

## Secondary page

### Smoothing  `sec.smoothing` — exp 1.2…3000 Hz
Cutoff of the follower both envelopes run through. Low pass filters the signal before detection.

### Fade  `sec.fade` — exp 10…250 ms
Real time latency, and button triggered slice duration. Longer times means more latency with longer crossfades on button press.

### Drive  `sec.drive` — linear 0.5…4
Gain into the keyframe shaper. Applied to the keyframes themselves to reduce aliasing.

### Character  `sec.character` — linear 0…1
Tape-like dropout saturation curve to clean to wobbly boost saturator. The tape saturator reduces the volume of low amplitude signals, the boost saturator boosts them.

### Mix  `sec.mix` — 0…100 %
Dry/wet mix.

### FB Tone  `sec.fbtone` — exp 48…21600 Hz
Bandpass centre inside the feedback loop. Affects the tonality of the feedback.

## Jacks

### In L  `in.l` — audio-in, silk **IN L**
Left input. The two channels are detected and sliced independently.

### In R  `in.r` — audio-in, silk **IN R**
Right input.

### Trig In  `trig.in` — trig, silk **TRIG IN**
External slice trigger. Rising edge, always accepted — even at threshold mute.

*See also: perf.threshold, b2*

### CV In  `cv.in` — cv-bi, silk **CV IN**
External modulation source, routable to any Primary knob from the Routing page. The only bipolar source of the three.

### Trig 1  `trig.1` — trig, silk **TRIG 1**
Triggers a slice, same as pressing the slice button.

*See also: perf.threshold*

### Env 1  `env.1` — cv-uni, silk **ENV 1**
Input follower envelope, mirrored on the P5 pip.

### Trig 2  `trig.2` — trig, silk **TRIG 2**
Gate on the output follower's transients.

### Env 2  `env.2` — cv-uni, silk **ENV 2**
Output follower envelope, mirrored on the P6 pip.

### Out L  `out.l` — audio-out, silk **OUT L**
Left output, post-mix.

### Out R  `out.r` — audio-out, silk **OUT R**
Right output, post-mix.

## Buttons

### B1  `b1`
Rotates between the two primary pages (warm colors).

- **Primary / Depth** (`click`) — Steps Primary → Depth, or returns to where the group was left.
- **Reset visible page** (`hold-b1-b3`) — With B3, half a second: resets the visible page to factory defaults.

*See also: b3*

### B2  `b2`
Slice, and save.

- **Slice** (`press`) — Fires a slice, jumping the read head to the current time.
- **Save boot state** (`hold`) — A second and a half: saves all four pages as the boot state. The press still slices on the way down.

*See also: perf.threshold, trig.in*

### B3  `b3`
Rotates between the two secondary pages (cool colors).

- **Routing / Secondary** (`click`) — Steps Routing → Secondary, or returns to where the group was left.

*See also: b1*

## Presets — SDK stock text (override with `Manual::PresetsHelp`)

Slot 0 is the boot state: **B2** held writes it, and the module loads it at power-up. The remaining slots are read and written from the programmer.

