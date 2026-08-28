#include "Parameters.h"

#include <cmath>

namespace capi
{

namespace pid
{
const juce::String perf[numPots]  = { "pitch", "stretch", "thresh", "grain", "quality", "feedback" };
const juce::String depth[numPots] = { "depth1", "depth2", "depth3", "depth4", "depth5", "depth6" };
const juce::String route[numPots] = { "route1", "route2", "route3", "route4", "route5", "route6" };
const juce::String sec[numPots]   = { "smooth", "fade", "drive", "character", "mix", "fbtone" };
}

float stretchCurve (float n)
{
    n = juce::jlimit (0.0f, 1.0f, n);
    return std::pow (1.0f - n, 2.5f);
}

float thresholdRatio (float n)
{
    if (n > 0.99f) return 1.0e9f;                      // Detector::kMuteAt sentinel
    return (n <= 0.9f) ? n * (4.0f / 0.9f)
                       : 4.0f + (n - 0.9f) * 40.0f;
}

float qualityEpsilon (float n)
{
    return 0.1f + juce::jlimit (0.0f, 1.0f, n) * (0.001f - 0.1f);
}

float pitchDetent (float semis)
{
    return (semis > -0.2f && semis < 0.2f) ? 0.0f : semis;
}

namespace
{
using Range  = juce::NormalisableRange<float>;
using Attrib = juce::AudioParameterFloatAttributes;

Range logRange (float lo, float hi)
{
    Range r (lo, hi);
    r.setSkewForCentre (std::sqrt (lo * hi));
    return r;
}

std::unique_ptr<juce::AudioParameterFloat> makeFloat (const juce::String& id,
                                                      const juce::String& name,
                                                      Range range, float def,
                                                      std::function<juce::String (float, int)> fmt)
{
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, range, def,
        Attrib().withStringFromValueFunction (std::move (fmt)));
}

juce::String characterText (float v)
{
    if (v < 0.02f) return "QUAKE";
    if (v > 0.98f) return "SINC";
    if (std::abs (v - 0.5f) < 0.02f) return "CLEAN";
    return juce::String (v < 0.5f ? "QK " : "SC ")
         + juce::String (juce::roundToInt (std::abs (v - 0.5f) * 200.0f)) + "%";
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    /* ── Perform — linear ranges throughout, so a parameter's host-normalised
       position IS the panel position the modulation matrix offsets. ─────── */
    layout.add (makeFloat (pid::perf[0], "Pitch", Range (-12.0f, 12.0f, 0.0f), 0.0f,
                           [] (float v, int) { return juce::String (pitchDetent (v), 2) + " st"; }));

    layout.add (makeFloat (pid::perf[1], "Stretch", Range (0.0f, 1.0f), 0.5f,
                           [] (float v, int)
                           {
                               const float s = stretchCurve (v);
                               return s < 1.0e-4f ? juce::String ("FREEZE")
                                                  : juce::String (1.0f / s, 2) + juce::String (" x");
                           }));

    layout.add (makeFloat (pid::perf[2], "Threshold", Range (0.0f, 1.0f), 0.5f,
                           [] (float v, int)
                           {
                               const float r = thresholdRatio (v);
                               return r > 100.0f ? juce::String ("MUTE") : juce::String (r, 2);
                           }));

    layout.add (makeFloat (pid::perf[3], "Grain Size", Range (32.0f, 4096.0f, 1.0f), 1250.0f,
                           [] (float v, int) { return juce::String (juce::roundToInt (v)) + " kf"; }));

    layout.add (makeFloat (pid::perf[4], "Quality", Range (0.0f, 1.0f), 1.0f,
                           [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }));

    layout.add (makeFloat (pid::perf[5], "Feedback", Range (0.0f, 1.5f), 0.0f,
                           [] (float v, int) { return juce::String (v, 2); }));

    /* ── Depth — bipolar modulation depth per perform control. ───────────── */
    static const char* depthNames[numPots] = { "Pitch Depth", "Stretch Depth", "Threshold Depth",
                                               "Grain Depth", "Quality Depth", "Feedback Depth" };
    for (int p = 0; p < numPots; ++p)
        layout.add (makeFloat (pid::depth[p], depthNames[p], Range (-1.0f, 1.0f), 0.0f,
                               [] (float v, int) { return juce::String (v, 2); }));

    /* ── Source — which follower (or the mod wheel) drives each depth. ───── */
    static const char* routeNames[numPots] = { "Pitch Source", "Stretch Source", "Threshold Source",
                                               "Grain Source", "Quality Source", "Feedback Source" };
    for (int p = 0; p < numPots; ++p)
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { pid::route[p], 1 }, routeNames[p],
            juce::StringArray { "In Follower", "Out Follower", "Mod Wheel" }, 1));

    /* ── Voice — cutoffs are real Hz here rather than the firmware's
       normalised fc, so the voicing tracks the host sample rate. ─────────── */
    layout.add (makeFloat (pid::sec[0], "Env Smoothing", logRange (1.0f, 3000.0f), 34.0f,
                           [] (float v, int) { return juce::String (v, v < 100.0f ? 1 : 0) + " Hz"; }));

    layout.add (makeFloat (pid::sec[1], "Fade", logRange (10.0f, 250.0f), 20.0f,
                           [] (float v, int) { return juce::String (v, 1) + " ms"; }));

    layout.add (makeFloat (pid::sec[2], "Drive", Range (0.5f, 4.0f), 1.0f,
                           [] (float v, int) { return juce::String (v, 2); }));

    layout.add (makeFloat (pid::sec[3], "Character", Range (0.0f, 1.0f), 1.0f,
                           [] (float v, int) { return characterText (v); }));

    layout.add (makeFloat (pid::sec[4], "Mix", Range (0.0f, 1.0f), 1.0f,
                           [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "% wet"; }));

    layout.add (makeFloat (pid::sec[5], "Feedback Tone", logRange (40.0f, 18000.0f), 480.0f,
                           [] (float v, int) { return juce::String (v, v < 1000.0f ? 0 : -1) + " Hz"; }));

    return layout;
}

juce::String performReadout (int pot, float norm)
{
    norm = juce::jlimit (0.0f, 1.0f, norm);

    switch (pot)
    {
        case 0: return juce::String (pitchDetent (-12.0f + 24.0f * norm), 1) + " st";
        case 1: { const float s = stretchCurve (norm);
                  return s < 1.0e-4f ? juce::String ("FREEZE")
                                     : juce::String (1.0f / s, 1) + juce::String ("x"); }
        case 2: { const float r = thresholdRatio (norm);
                  return r > 100.0f ? juce::String ("MUTE") : juce::String (r, 1); }
        case 3: return juce::String (juce::roundToInt (32.0f + 4064.0f * norm));
        case 4: return juce::String (juce::roundToInt (norm * 100.0f)) + "%";
        default: return juce::String (norm * 1.5f, 2);
    }
}

juce::String voiceReadout (int pot, float value)
{
    switch (pot)
    {
        case 0: return value < 10.0f ? juce::String (value, 1) + "Hz"
                                     : juce::String (juce::roundToInt (value)) + "Hz";
        case 1: return juce::String (value, 1) + "ms";
        case 2: return juce::String (value, 2);
        case 3: return characterText (value);
        case 4: return juce::String (juce::roundToInt (value * 100.0f)) + "%";
        default: return value < 1000.0f
                     ? juce::String (juce::roundToInt (value)) + "Hz"
                     : juce::String (value / 1000.0f, 1) + "kHz";
    }
}

juce::String depthReadout (float depth)
{
    if (std::abs (depth) < 0.01f) return "OFF";
    return (depth > 0.0f ? "+" : "") + juce::String (depth, 2);
}

} // namespace capi
