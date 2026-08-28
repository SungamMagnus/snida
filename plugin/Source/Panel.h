#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Flat drawing primitives and the panel grid. Everything is expressed in a
 * fixed 960 x 468 design space; the editor applies one scale transform, so
 * layout constants double as hit-test geometry.
 *
 * Bone paper and ink. Colour is never decoration — a hue means "this control
 * belongs to that section", or it is a live signal. Nothing else is coloured.
 */
namespace capi::panel
{

constexpr float designW = 960.0f, designH = 468.0f;

/* ── Palette ─────────────────────────────────────────────────────────────── */
namespace hue
{
const juce::Colour paper   { 0xfff2f1ec };
const juce::Colour ink     { 0xff191c1b };
const juce::Colour perform { 0xffc2521a };   // burnt orange
const juce::Colour mod     { 0xff2e4b8f };   // ink blue
const juce::Colour voice   { 0xff0f7a72 };   // deep teal
const juce::Colour depth   { 0xffa8285a };   // magenta
const juce::Colour signal  { 0xff2e8b4a };   // envelope green
const juce::Colour clip    { 0xffc62b1c };
const juce::Colour wheel   { 0xff6b34b0 };   // mod wheel
}

inline juce::Colour ink (float alpha) { return hue::ink.withAlpha (alpha); }

/* ── Grid ────────────────────────────────────────────────────────────────── */
constexpr float titleCy = 215.0f;            // shared centre for the three titles

constexpr float performTitleX = 54.0f, performX = 70.0f;
constexpr float modTitleX     = 338.0f;
constexpr float voiceTitleX   = 670.0f, voiceX  = 686.0f;
constexpr float rule1X = 314.0f, rule2X = 642.0f;
constexpr float contentL = 44.0f, contentR = 916.0f;

constexpr float faderStep  = 46.0f;
constexpr float faderTop   = 220.0f;
constexpr float faderTrack = 140.0f;
constexpr float faderBot   = faderTop + faderTrack;

constexpr float bigKnobY = 96.0f, bigKnobR = 32.0f;

constexpr float tgtX = 354.0f, tgtW = 44.0f;
constexpr float togX = 406.0f, togW = 138.0f;
constexpr float depthKnobX = 566.0f, depthValX = 584.0f, depthKnobR = 13.0f;
constexpr float row0 = 76.0f, rowStep = 54.0f;

constexpr float statusY = 426.0f;

inline float faderX (float columnX, int i) { return columnX + faderStep * (0.5f + (float) i); }
inline float rowY   (int i)                { return row0 + rowStep * (float) i; }
inline float bigKnobX (float columnX)      { return columnX + faderStep * 2.5f; }

inline juce::Rectangle<float> toggleRect (int i)
{
    return { togX, rowY (i) - 10.0f, togW, 20.0f };
}

inline juce::Rectangle<float> sliceRect()
{
    return { contentR - 68.0f - 82.0f, statusY - 10.0f, 60.0f, 20.0f };
}

/* ── Type ────────────────────────────────────────────────────────────────── */
juce::Font mono (float h, bool bold = false);

void text (juce::Graphics&, const juce::String&, juce::Rectangle<float>,
           float size, juce::Colour,
           juce::Justification = juce::Justification::centred, bool bold = true);

/** Letter-spaced run — the section titles and the wordmark. */
void tracked (juce::Graphics&, const juce::String&, juce::Rectangle<float>,
              float size, juce::Colour, float tracking,
              bool leftAlign = true, bool bold = true);

/** Section title, reading bottom-to-top beside its own controls. */
void trackedVertical (juce::Graphics&, const juce::String&, float cx, float cy,
                      float size, juce::Colour, float tracking);

void rule (juce::Graphics&, float x, float y, float w, float h, float alpha = 0.13f);

/* ── Controls ────────────────────────────────────────────────────────────── */

/** Bipolar knob: one circle, one arc from noon, one pointer. */
void knob (juce::Graphics&, float cx, float cy, float r, float norm,
           juce::Colour, float lineW = 1.4f, float arcW = 3.0f);

/** Unipolar fader. The handle hugs the travel — 6 pt track, 14 pt handle. */
void fader (juce::Graphics&, float cx, float norm, juce::Colour,
            const juce::String& label, const juce::String& readout);

/** Three-position toggle; every position stays labelled. */
void toggle3 (juce::Graphics&, juce::Rectangle<float>, int selected,
              juce::Colour, const juce::StringArray& labels);

void meter        (juce::Graphics&, juce::Rectangle<float>, float level, juce::Colour);
void meterBipolar (juce::Graphics&, juce::Rectangle<float>, float value, juce::Colour);
void lamp         (juce::Graphics&, float cx, float cy, bool on, juce::Colour);

} // namespace capi::panel
