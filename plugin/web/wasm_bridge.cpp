// WebAssembly entry points for the browser build.
//
// A shim over the same CapicolaEngine the plug-in uses - no JUCE in it, and
// lib/ is header-only standard C++, so the page runs the same keyframe engine
// as the VST and the module.
//
// Parameters arrive already in engineering units. The web UI computes them
// with the same tapers it uses for its readouts, so what the panel says and
// what the engine gets cannot drift apart.

#include "CapicolaEngine.h"

#include <algorithm>

namespace
{
constexpr int kMaxBlock = 2048;

capi::CapicolaEngine engine;
float bufL[kMaxBlock];
float bufR[kMaxBlock];
} // namespace

extern "C"
{

__attribute__((used)) void colacut_init (float sampleRate)
{
    engine.prepare ((double) sampleRate);
}

__attribute__((used)) float* colacut_buf_l() { return bufL; }
__attribute__((used)) float* colacut_buf_r() { return bufR; }

__attribute__((used)) void colacut_set_params (
    float pitchSemis, float stretch, float threshRatio, float grainKeyframes,
    float qualityEps, float feedback, float smoothingHz, float fadeMs,
    float drive, float character, float mixWet, float feedbackToneHz)
{
    engine.setPitch        (pitchSemis);
    engine.setStretch      (stretch);
    engine.setThreshold    (threshRatio);
    engine.setGrainSize    (grainKeyframes);
    engine.setQuality      (qualityEps);
    engine.setFeedback     (feedback);
    engine.setSmoothing    (smoothingHz);
    engine.setFade         (fadeMs);
    engine.setDrive        (drive);
    engine.setCharacter    (character);
    engine.setMix          (mixWet);
    engine.setFeedbackTone (feedbackToneHz);
}

__attribute__((used)) void colacut_slice() { engine.triggerSlice(); }

__attribute__((used)) void colacut_process (int n)
{
    engine.process (bufL, bufR, std::min (n, kMaxBlock));
}

/* The followers the modulation matrix routes. */
__attribute__((used)) float colacut_env_in()  { return engine.envIn(); }
__attribute__((used)) float colacut_env_out() { return engine.envOut(); }
__attribute__((used)) float colacut_in_peak() { return engine.inPeak(); }
__attribute__((used)) int   colacut_in_gate() { return engine.inGate() ? 1 : 0; }
__attribute__((used)) int   colacut_latency() { return engine.latencySamples(); }

} // extern "C"
