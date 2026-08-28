#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "Panel.h"
#include "PluginProcessor.h"

/**
 * The panel: every control on one surface. Drawing and hit testing both work in
 * the fixed 960 x 468 design space of Panel.h, with a single scale transform
 * applied on the way out, so the layout constants are the only source of truth.
 */
class CapicolaEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit CapicolaEditor (CapicolaProcessor&);
    ~CapicolaEditor() override;

    void paint (juce::Graphics&) override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;

    enum class Kind { knob, fader, toggle };

    struct Ctl
    {
        Kind kind {};
        juce::RangedAudioParameter* param = nullptr;
        juce::Rectangle<float> hit;   // design space
        float travel = 150.0f;        // pixels of drag for a full sweep
    };

    void buildControls();

    float scale() const;
    juce::Point<float> toDesign (juce::Point<float> px) const;
    int controlAt (juce::Point<float> design) const;

    /** A perform control's position after its routed source has pushed it —
        what the module's LED ring shows. */
    float modulatedNorm (int pot) const;

    CapicolaProcessor& proc;
    std::vector<Ctl> ctls;

    int   dragIdx = -1;
    float dragStartNorm = 0.0f;
    float dragStartY = 0.0f;

    int          lastSliceCount = 0;
    juce::uint32 sliceFlashMs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CapicolaEditor)
};
