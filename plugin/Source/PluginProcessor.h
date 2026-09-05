#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "CapicolaEngine.h"
#include "Limiter.h"
#include "Parameters.h"

/** Everything the panel animates, published from the audio thread. */
struct PanelState
{
    std::atomic<float> envIn { 0.0f };
    std::atomic<float> envOut { 0.0f };
    std::atomic<float> inPeak { 0.0f };
    std::atomic<float> modIn { 0.0f };   // mod wheel, bipolar
    std::atomic<bool>  inGate { false };
    std::atomic<bool>  outGate { false };

    /** Bumps on every slice the engine takes, so the panel can flash B2. */
    std::atomic<int> sliceCount { 0 };
};

class CapicolaProcessor final : public juce::AudioProcessor
{
public:
    CapicolaProcessor();
    ~CapicolaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sníða"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    /** SLICE / MIDI note-on — force a splice on both channels. */
    void requestSlice() { sliceRequest_.store (true); }

    juce::AudioProcessorValueTreeState apvts;
    PanelState panel;

private:
    void pushParameters (float sigIn, float sigOut, float sigWheel);

    capi::CapicolaEngine engine_;
    capi::Limiter        limiter_;
    juce::AudioBuffer<float> scratch_;

    std::atomic<bool> sliceRequest_ { false };
    float modIn_ = 0.0f;
    float lastFade_ = -1.0f;
    bool  prevInGate_ = false;

    std::array<juce::AudioParameterFloat*,  capi::numPots> perf_ {};
    std::array<juce::AudioParameterFloat*,  capi::numPots> depth_ {};
    std::array<juce::AudioParameterChoice*, capi::numPots> route_ {};
    std::array<juce::AudioParameterFloat*,  capi::numPots> sec_ {};
    juce::AudioParameterBool* limiterOn_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CapicolaProcessor)
};
