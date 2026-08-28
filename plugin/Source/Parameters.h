#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace capi
{

static constexpr int numPots = 6;

/** Modulation sources, matching the three toggle positions. */
enum class ModSource { inFollower = 0, outFollower, modWheel };

namespace pid
{
extern const juce::String perf[numPots];   // pitch, stretch, thresh, grain, quality, feedback
extern const juce::String depth[numPots];
extern const juce::String route[numPots];
extern const juce::String sec[numPots];
extern const juce::String limiter;    // smooth, fade, drive, character, mix, fbtone
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/* ── Knob taper, verbatim from src/main.cpp ──────────────────────────────── */

/** Stretch taper: (1-n)^2.5. n=0 realtime, noon ~ 5.7x slow, n=1 a true freeze. */
float stretchCurve (float n);

/** Threshold: 0..90% of the sweep is ratio 0..4, the last 10% climbs 4..8, and
    the very top returns the mute sentinel (>= Detector::kMuteAt). */
float thresholdRatio (float n);

/** Analyzer keyframe threshold: n=0 -> eps 0.1 (crunchy), n=1 -> eps 0.001. */
float qualityEpsilon (float n);

/** Pitch detent: within +/-0.2 st of noon, snap to exactly unity. */
float pitchDetent (float semis);

/* ── Panel readouts ──────────────────────────────────────────────────────
 * Compact strings for the faceplate. The host keeps the verbose forms that
 * createLayout() installs. */

/** A perform control at its *modulated* position. */
juce::String performReadout (int pot, float norm);

/** A voice control at its parameter value (engineering units). */
juce::String voiceReadout (int pot, float value);

/** A modulation depth, -1..+1. */
juce::String depthReadout (float depth);

} // namespace capi
