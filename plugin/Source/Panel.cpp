#include "Panel.h"

namespace capi::panel
{

namespace
{
/* Eurorack pot sweep, kept from the hardware: 111.4 deg -> 428.6 deg, y-down,
   so a knob at noon sits exactly where the module's does. */
constexpr float kArc0 = 111.4f, kArcSpan = 317.2f;

float arcRad (float deg) { return juce::degreesToRadians (deg + 90.0f); }
} // namespace

juce::Font mono (float h, bool bold)
{
    return juce::Font (juce::FontOptions()
                           .withName (juce::Font::getDefaultMonospacedFontName())
                           .withHeight (h)
                           .withStyle (bold ? "Bold" : "Regular"));
}

void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r,
           float size, juce::Colour c, juce::Justification j, bool bold)
{
    g.setColour (c);
    g.setFont (mono (size, bold));
    g.drawFittedText (s, r.getSmallestIntegerContainer(), j, 1, 0.9f);
}

void tracked (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r,
              float size, juce::Colour c, float tracking, bool leftAlign, bool bold)
{
    g.setColour (c);
    g.setFont (mono (size, bold));
    const auto f = g.getCurrentFont();

    float total = -tracking;
    for (int i = 0; i < s.length(); ++i)
        total += juce::GlyphArrangement::getStringWidth (f, s.substring (i, i + 1)) + tracking;

    float x = leftAlign ? r.getX() : r.getCentreX() - total * 0.5f;
    for (int i = 0; i < s.length(); ++i)
    {
        const auto ch = s.substring (i, i + 1);
        const float w = juce::GlyphArrangement::getStringWidth (f, ch);
        g.drawText (ch, juce::Rectangle<float> (x, r.getY(), w, r.getHeight()),
                    juce::Justification::centred, false);
        x += w + tracking;
    }
}

void trackedVertical (juce::Graphics& g, const juce::String& s, float cx, float cy,
                      float size, juce::Colour c, float tracking)
{
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, cx, cy));
    tracked (g, s, { cx - 220.0f, cy - size * 0.8f, 440.0f, size * 1.7f },
             size, c, tracking, false);
    g.restoreState();
}

void rule (juce::Graphics& g, float x, float y, float w, float h, float alpha)
{
    g.setColour (ink (alpha));
    g.fillRect (x, y, w, h);
}

void knob (juce::Graphics& g, float cx, float cy, float r, float norm,
           juce::Colour colour, float lineW, float arcW)
{
    const juce::Point<float> c (cx, cy);
    norm = juce::jlimit (0.0f, 1.0f, norm);

    g.setColour (ink (0.18f));
    g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), lineW);

    g.setColour (ink (0.32f));
    g.drawLine ({ c.getPointOnCircumference (r + 3.0f, 0.0f),
                  c.getPointOnCircumference (r + 7.0f, 0.0f) }, lineW);

    if (std::abs (norm - 0.5f) > 0.004f)
    {
        juce::Path p;
        p.addCentredArc (cx, cy, r, r, 0.0f,
                         arcRad (kArc0 + kArcSpan * juce::jmin (norm, 0.5f)),
                         arcRad (kArc0 + kArcSpan * juce::jmax (norm, 0.5f)), true);
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (arcW, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::butt));
    }

    const float ang = arcRad (kArc0 + kArcSpan * norm);
    g.setColour (hue::ink);
    g.drawLine ({ c, c.getPointOnCircumference (r * 0.78f, ang) }, lineW + 0.5f);
    g.fillEllipse (juce::Rectangle<float> (r * 0.14f, r * 0.14f).withCentre (c));
}

void fader (juce::Graphics& g, float cx, float norm, juce::Colour colour,
            const juce::String& label, const juce::String& readout)
{
    constexpr float trackW = 6.0f, handleW = 14.0f, handleH = 4.0f;
    const float y = faderBot - juce::jlimit (0.0f, 1.0f, norm) * faderTrack;

    tracked (g, label, { cx - 30.0f, faderTop - 26.0f, 60.0f, 12.0f }, 8.5f,
             ink (0.55f), 1.0f, false);

    g.setColour (ink (0.13f));
    for (int i = 0; i <= 8; ++i)
    {
        const float ty = faderTop + faderTrack * (float) i / 8.0f;
        g.fillRect (cx - 15.0f, ty, (i % 4 == 0) ? 6.0f : 3.5f, 1.0f);
    }

    g.setColour (ink (0.15f));
    g.fillRoundedRectangle (cx - trackW * 0.5f, faderTop, trackW, faderTrack, trackW * 0.5f);

    if (faderBot - y > 1.0f)
    {
        g.setColour (colour);
        g.fillRoundedRectangle (cx - trackW * 0.5f, y, trackW, faderBot - y, trackW * 0.5f);
    }

    g.setColour (hue::ink);
    g.fillRect (cx - handleW * 0.5f, y - handleH * 0.5f, handleW, handleH);

    text (g, readout, { cx - 28.0f, faderBot + 12.0f, 56.0f, 14.0f }, 10.5f, colour);
}

void toggle3 (juce::Graphics& g, juce::Rectangle<float> r, int selected,
              juce::Colour colour, const juce::StringArray& labels)
{
    const float segW = r.getWidth() / 3.0f;

    for (int i = 0; i < 3; ++i)
    {
        auto seg = juce::Rectangle<float> (r.getX() + segW * (float) i, r.getY(), segW, r.getHeight());
        if (i == selected)
        {
            g.setColour (colour);
            g.fillRect (seg);
            text (g, labels[i], seg, 9.0f, hue::paper);
        }
        else
        {
            text (g, labels[i], seg, 9.0f, ink (0.5f));
        }
    }

    g.setColour (ink (0.28f));
    g.drawRect (r, 1.0f);
    g.setColour (ink (0.18f));
    for (int i = 1; i < 3; ++i)
        g.fillRect (r.getX() + segW * (float) i, r.getY(), 1.0f, r.getHeight());
}

void meter (juce::Graphics& g, juce::Rectangle<float> r, float level, juce::Colour colour)
{
    g.setColour (ink (0.11f));
    g.fillRect (r);
    g.setColour (colour);
    g.fillRect (r.withWidth (r.getWidth() * juce::jlimit (0.0f, 1.0f, level)));
    g.setColour (ink (0.22f));
    g.drawRect (r, 1.0f);
}

void meterBipolar (juce::Graphics& g, juce::Rectangle<float> r, float value, juce::Colour colour)
{
    g.setColour (ink (0.11f));
    g.fillRect (r);

    const float mid = r.getCentreX();
    const float half = r.getWidth() * 0.5f * juce::jlimit (-1.0f, 1.0f, value);
    g.setColour (colour);
    if (half >= 0.0f) g.fillRect (juce::Rectangle<float> (mid, r.getY(), half, r.getHeight()));
    else              g.fillRect (juce::Rectangle<float> (mid + half, r.getY(), -half, r.getHeight()));

    g.setColour (ink (0.22f));
    g.drawRect (r, 1.0f);
    g.fillRect (mid - 0.5f, r.getY() - 2.0f, 1.0f, r.getHeight() + 4.0f);
}

void lamp (juce::Graphics& g, float cx, float cy, bool on, juce::Colour colour)
{
    auto s = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ cx, cy });
    g.setColour (on ? colour : ink (0.10f));
    g.fillRect (s);
    g.setColour (ink (0.28f));
    g.drawRect (s, 1.0f);
}

} // namespace capi::panel
