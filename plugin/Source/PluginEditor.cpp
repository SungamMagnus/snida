#include "PluginEditor.h"

using namespace capi;
using namespace capi::panel;

namespace
{
constexpr float kSliceFlashMs = 90.0f;

/* Perform: pitch is the bipolar knob, the rest are the fader bank. */
const char* kPerformLabel[5] = { "STRETCH", "THRESH", "GRAIN", "QUALITY", "FEEDBACK" };

/* Voice: character (sec[3]) is the knob, so the faders skip it. */
constexpr int kVoiceFaderParam[5] = { 0, 1, 2, 4, 5 };
const char* kVoiceLabel[5] = { "SMOOTH", "FADE", "DRIVE", "MIX", "FB TONE" };

const char* kTargetLabel[6] = { "PITCH", "STRETCH", "THRESH", "GRAIN", "QUALITY", "FEEDBK" };

/* Control indices into `ctls`, in build order: pitch knob, 5 perform faders,
   6 toggles, 6 depth knobs, character knob, 5 voice faders. */
constexpr int kToggle    = 6;
constexpr int kDepthKnob = 12;
constexpr int kCharKnob  = 18;
constexpr int kVoiceFade = 19;
constexpr int kLimiter   = 24;
} // namespace

CapicolaEditor::CapicolaEditor (CapicolaProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setOpaque (true);
    buildControls();

    setResizable (true, true);
    setResizeLimits (720, (int) (720.0f * designH / designW),
                     1680, (int) (1680.0f * designH / designW));
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);
    setSize ((int) designW, (int) designH);

    startTimerHz (30);
}

CapicolaEditor::~CapicolaEditor() = default;

/* ── Control table ───────────────────────────────────────────────────────── */

void CapicolaEditor::buildControls()
{
    auto add = [this] (Kind kind, const juce::String& id, juce::Rectangle<float> hit, float travel)
    {
        Ctl c;
        c.kind = kind;
        c.param = proc.apvts.getParameter (id);
        c.hit = hit;
        c.travel = travel;
        jassert (c.param != nullptr);
        ctls.push_back (c);
    };

    auto knobBox = [] (float cx, float cy, float r)
    {
        return juce::Rectangle<float> (r * 2.4f, r * 2.4f).withCentre ({ cx, cy });
    };

    /* Perform. */
    add (Kind::knob, pid::perf[0], knobBox (bigKnobX (performX), bigKnobY, bigKnobR), 150.0f);
    for (int i = 0; i < 5; ++i)
        add (Kind::fader, pid::perf[i + 1],
             { faderX (performX, i) - 16.0f, faderTop - 8.0f, 32.0f, faderTrack + 16.0f },
             faderTrack);

    /* Modulation. */
    for (int i = 0; i < 6; ++i)
        add (Kind::toggle, pid::route[i], toggleRect (i), 0.0f);
    for (int i = 0; i < 6; ++i)
        add (Kind::knob, pid::depth[i],
             knobBox (depthKnobX, rowY (i), depthKnobR), 120.0f);

    /* Voice. */
    add (Kind::knob, pid::sec[3], knobBox (bigKnobX (voiceX), bigKnobY, bigKnobR), 150.0f);
    for (int i = 0; i < 5; ++i)
        add (Kind::fader, pid::sec[kVoiceFaderParam[i]],
             { faderX (voiceX, i) - 16.0f, faderTop - 8.0f, 32.0f, faderTrack + 16.0f },
             faderTrack);

    /* Output. */
    add (Kind::toggle, pid::limiter, limiterBox().expanded (6.0f), 0.0f);
}

float CapicolaEditor::scale() const
{
    return (float) getWidth() / designW;
}

juce::Point<float> CapicolaEditor::toDesign (juce::Point<float> px) const
{
    const float k = juce::jmax (0.0001f, scale());
    return { px.x / k, px.y / k };
}

int CapicolaEditor::controlAt (juce::Point<float> design) const
{
    for (size_t i = 0; i < ctls.size(); ++i)
        if (ctls[i].hit.contains (design))
            return (int) i;
    return -1;
}

float CapicolaEditor::modulatedNorm (int pot) const
{
    auto* perf  = proc.apvts.getParameter (pid::perf[pot]);
    auto* depth = proc.apvts.getParameter (pid::depth[pot]);
    auto* route = proc.apvts.getParameter (pid::route[pot]);

    const int src = juce::jlimit (0, 2, juce::roundToInt (route->getValue() * 2.0f));
    const float sig = src == 0 ? proc.panel.envIn.load (std::memory_order_relaxed)
                    : src == 1 ? proc.panel.envOut.load (std::memory_order_relaxed)
                               : proc.panel.modIn.load (std::memory_order_relaxed);

    return juce::jlimit (0.0f, 1.0f,
                         perf->getValue() + depth->convertFrom0to1 (depth->getValue()) * sig);
}

/* ── Interaction ─────────────────────────────────────────────────────────── */

void CapicolaEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto d = toDesign (e.position);

    if (sliceRect().contains (d))
    {
        proc.requestSlice();
        return;
    }

    if (limiterBox().expanded (6.0f).contains (d))
    {
        auto* param = ctls[(size_t) kLimiter].param;
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->getValue() > 0.5f ? 0.0f : 1.0f);
        param->endChangeGesture();
        repaint();
        return;
    }

    const int idx = controlAt (d);
    if (idx < 0)
        return;

    auto& c = ctls[(size_t) idx];

    if (c.kind == Kind::toggle)
    {
        const int seg = juce::jlimit (0, 2, (int) ((d.x - c.hit.getX()) / (c.hit.getWidth() / 3.0f)));
        c.param->beginChangeGesture();
        c.param->setValueNotifyingHost (seg * 0.5f);
        c.param->endChangeGesture();
        repaint();
        return;
    }

    dragIdx = idx;
    dragStartNorm = c.param->getValue();
    dragStartY = d.y;
    c.param->beginChangeGesture();
    setMouseCursor (juce::MouseCursor::NoCursor);
}

void CapicolaEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (dragIdx < 0)
        return;

    auto& c = ctls[(size_t) dragIdx];
    const float sensitivity = e.mods.isShiftDown() ? 0.22f : 1.0f;
    const float delta = (dragStartY - toDesign (e.position).y) / c.travel;

    c.param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f,
                                                  dragStartNorm + delta * sensitivity));
    repaint();
}

void CapicolaEditor::mouseUp (const juce::MouseEvent&)
{
    if (dragIdx < 0)
        return;

    ctls[(size_t) dragIdx].param->endChangeGesture();
    dragIdx = -1;
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void CapicolaEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int idx = controlAt (toDesign (e.position));
    if (idx < 0 || ctls[(size_t) idx].kind == Kind::toggle)
        return;

    auto* param = ctls[(size_t) idx].param;
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->getDefaultValue());
    param->endChangeGesture();
    repaint();
}

void CapicolaEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int idx = controlAt (toDesign (e.position));
    if (idx < 0)
        return;

    auto& c = ctls[(size_t) idx];
    /* A notch is deltaY ~0.1: one position on a toggle, ~1.6% elsewhere. */
    const float gain = c.kind == Kind::toggle ? 4.0f
                                              : (e.mods.isShiftDown() ? 0.04f : 0.16f);
    const float delta = w.deltaY * (w.isReversed ? -1.0f : 1.0f) * gain;

    c.param->beginChangeGesture();
    c.param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, c.param->getValue() + delta));
    c.param->endChangeGesture();
    repaint();
}

void CapicolaEditor::timerCallback()
{
    const int slices = proc.panel.sliceCount.load (std::memory_order_relaxed);
    if (slices != lastSliceCount)
    {
        lastSliceCount = slices;
        sliceFlashMs = juce::Time::getMillisecondCounter();
    }
    repaint();
}

/* ── Paint ───────────────────────────────────────────────────────────────── */

void CapicolaEditor::paint (juce::Graphics& g)
{
    g.fillAll (hue::paper);
    g.addTransform (juce::AffineTransform::scale (scale()));

    const auto& state = proc.panel;
    const bool flashing = juce::Time::getMillisecondCounter() - sliceFlashMs
                              < (juce::uint32) kSliceFlashMs;

    g.setColour (ink (0.22f));
    g.drawRect (juce::Rectangle<float> (16.0f, 16.0f, designW - 32.0f, designH - 32.0f), 1.0f);

    rule (g, rule1X, 40.0f, 1.0f, 350.0f, 0.11f);
    rule (g, rule2X, 40.0f, 1.0f, 350.0f, 0.11f);

    /* ── Perform ─────────────────────────────────────────────────────────── */
    {
        trackedVertical (g, "PERFORM", performTitleX, titleCy, 9.5f, hue::perform, 3.0f);

        const float cx = bigKnobX (performX);
        const float n = modulatedNorm (0);
        tracked (g, "PITCH", { cx - 60.0f, 44.0f, 120.0f, 12.0f }, 8.5f, ink (0.55f), 1.0f, false);
        knob (g, cx, bigKnobY, bigKnobR, n, hue::perform);
        text (g, performReadout (0, n), { cx - 60.0f, 138.0f, 120.0f, 16.0f }, 12.5f, hue::perform);
        text (g, "semitones, unity at noon", { cx - 84.0f, 158.0f, 168.0f, 12.0f }, 8.2f,
              ink (0.34f), juce::Justification::centred, false);

        for (int i = 0; i < 5; ++i)
        {
            const float fn = modulatedNorm (i + 1);
            fader (g, faderX (performX, i), fn, hue::perform,
                   kPerformLabel[i], performReadout (i + 1, fn));
        }
    }

    /* ── Modulation ──────────────────────────────────────────────────────── */
    {
        trackedVertical (g, "MODULATION", modTitleX, titleCy, 9.5f, hue::mod, 3.0f);

        text (g, "TARGET", { tgtX, 44.0f, tgtW, 12.0f }, 8.0f, ink (0.38f),
              juce::Justification::right, false);
        text (g, "SOURCE", { togX, 44.0f, togW, 12.0f }, 8.0f, ink (0.38f),
              juce::Justification::centred, false);
        text (g, "DEPTH", { depthKnobX - 22.0f, 44.0f, 88.0f, 12.0f }, 8.0f, ink (0.38f),
              juce::Justification::centred, false);

        const juce::StringArray zones { "IN", "OUT", "MOD" };
        for (int i = 0; i < 6; ++i)
        {
            const float y = rowY (i);
            auto* route = ctls[(size_t) (kToggle + i)].param;
            auto* depth = ctls[(size_t) (kDepthKnob + i)].param;

            text (g, kTargetLabel[i], { tgtX - 4.0f, y - 7.0f, tgtW, 14.0f }, 9.0f,
                  ink (0.75f), juce::Justification::right);

            toggle3 (g, toggleRect (i),
                     juce::jlimit (0, 2, juce::roundToInt (route->getValue() * 2.0f)),
                     hue::mod, zones);

            const float dv = depth->convertFrom0to1 (depth->getValue());
            knob (g, depthKnobX, y, depthKnobR, depth->getValue(), hue::depth, 1.0f, 2.4f);
            text (g, depthReadout (dv), { depthValX, y - 7.0f, 44.0f, 14.0f }, 9.0f,
                  std::abs (dv) < 0.01f ? ink (0.30f) : hue::depth,
                  juce::Justification::left);
        }

        /* Each source is described under the column it selects. */
        const float segW = togW / 3.0f;
        const char* line1[3] = { "INPUT", "OUTPUT", "MOD WHEEL" };
        const char* line2[3] = { "FOLLOWER", "FOLLOWER", "CC 1" };
        for (int i = 0; i < 3; ++i)
        {
            const juce::Rectangle<float> col (togX + segW * (float) i, 0.0f, segW, 11.0f);
            text (g, line1[i], col.withY (372.0f), 7.6f, ink (0.42f), juce::Justification::centred, false);
            text (g, line2[i], col.withY (383.0f), 7.6f, ink (0.42f), juce::Justification::centred, false);
        }
    }

    /* ── Voice ───────────────────────────────────────────────────────────── */
    {
        trackedVertical (g, "VOICE", voiceTitleX, titleCy, 9.5f, hue::voice, 3.0f);

        const float cx = bigKnobX (voiceX);
        auto* character = ctls[(size_t) kCharKnob].param;
        tracked (g, "CHARACTER", { cx - 60.0f, 44.0f, 120.0f, 12.0f }, 8.5f, ink (0.55f), 1.0f, false);
        knob (g, cx, bigKnobY, bigKnobR, character->getValue(), hue::voice);
        text (g, voiceReadout (3, character->convertFrom0to1 (character->getValue())),
              { cx - 60.0f, 138.0f, 120.0f, 16.0f }, 12.5f, hue::voice);
        text (g, "quake / clean / sinc", { cx - 84.0f, 158.0f, 168.0f, 12.0f }, 8.2f,
              ink (0.34f), juce::Justification::centred, false);

        for (int i = 0; i < 5; ++i)
        {
            auto* param = ctls[(size_t) (kVoiceFade + i)].param;
            fader (g, faderX (voiceX, i), param->getValue(), hue::voice, kVoiceLabel[i],
                   voiceReadout (kVoiceFaderParam[i], param->convertFrom0to1 (param->getValue())));
        }
    }

    /* ── Status bar — every item is live ─────────────────────────────────── */
    {
        rule (g, contentL, 404.0f, contentR - contentL, 1.0f, 0.18f);

        const float y = statusY;

        /* The output meter carries the limiter's state - lit when it is
           guarding the output, grey when the output is running free. */
        const bool limiting = ctls[(size_t) kLimiter].param->getValue() > 0.5f;

        text (g, "IN", { 44.0f, y - 7.0f, 20.0f, 14.0f }, 8.5f, ink (0.45f), juce::Justification::left, false);
        meter (g, { 66.0f, y - 3.5f, 70.0f, 7.0f }, state.envIn.load (std::memory_order_relaxed), hue::signal);
        lamp (g, 146.0f, y, state.inGate.load (std::memory_order_relaxed) || flashing, hue::signal);

        text (g, "OUT", { 168.0f, y - 7.0f, 26.0f, 14.0f }, 8.5f, ink (0.45f), juce::Justification::left, false);
        meter (g, outMeterRect(), state.envOut.load (std::memory_order_relaxed),
               limiting ? hue::signal : ink (0.30f));
        lamp (g, 278.0f, y, state.outGate.load (std::memory_order_relaxed), hue::signal);

        checkbox (g, limiterBox(), limiting, hue::signal);
        text (g, "LIM", { 306.0f, y - 7.0f, 26.0f, 14.0f }, 7.8f,
              limiting ? hue::signal : ink (0.38f), juce::Justification::left, false);

        text (g, "MOD", { 344.0f, y - 7.0f, 26.0f, 14.0f }, 8.5f, ink (0.45f), juce::Justification::left, false);
        meterBipolar (g, { 374.0f, y - 3.5f, 70.0f, 7.0f }, state.modIn.load (std::memory_order_relaxed), hue::wheel);

        text (g, "CLIP", { 460.0f, y - 7.0f, 30.0f, 14.0f }, 8.5f, ink (0.45f), juce::Justification::left, false);
        lamp (g, 504.0f, y, state.inPeak.load (std::memory_order_relaxed) > 0.99f, hue::clip);

        text (g, "LATENCY", { 524.0f, y - 7.0f, 58.0f, 14.0f }, 8.5f, ink (0.45f),
              juce::Justification::left, false);
        text (g, juce::String (proc.getLatencySamples() * 1000.0
                                   / juce::jmax (1.0, proc.getSampleRate()), 1) + " ms",
              { 586.0f, y - 7.0f, 62.0f, 14.0f }, 10.0f, hue::ink, juce::Justification::left);

        /* Bottom-right corner: the action, then the mark. */
        const juce::Rectangle<float> mark (contentR - 68.0f, y - 6.0f, 68.0f, 12.0f);
        tracked (g, juce::CharPointer_UTF8 ("SNÍÐA"), mark, 8.5f, ink (0.62f), 3.2f);

        const auto b = sliceRect();
        g.setColour (flashing ? hue::ink : hue::perform);
        g.fillRect (b);
        tracked (g, "SLICE", b.withY (b.getY() + 5.0f).withHeight (11.0f), 8.5f, hue::paper, 1.6f, false);

        text (g, "midi note on", { b.getX() - 74.0f, y - 7.0f, 66.0f, 14.0f }, 7.6f, ink (0.32f),
              juce::Justification::right, false);
    }
}
