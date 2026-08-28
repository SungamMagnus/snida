#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kControlChunk = 64;   // the module's control-loop cadence
}

CapicolaProcessor::CapicolaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("In", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Out", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "state", capi::createLayout())
{
    for (size_t p = 0; p < (size_t) capi::numPots; ++p)
    {
        perf_[p]  = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter (capi::pid::perf[p]));
        depth_[p] = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter (capi::pid::depth[p]));
        route_[p] = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (capi::pid::route[p]));
        sec_[p]   = dynamic_cast<juce::AudioParameterFloat*>  (apvts.getParameter (capi::pid::sec[p]));
        jassert (perf_[p] && depth_[p] && route_[p] && sec_[p]);
    }

    limiterOn_ = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (capi::pid::limiter));
    jassert (limiterOn_);
}

bool CapicolaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void CapicolaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine_.prepare (sampleRate);
    limiter_.prepare (sampleRate);
    scratch_.setSize (2, juce::jmax (samplesPerBlock, kControlChunk), false, true, true);
    modIn_ = 0.0f;
    lastFade_ = -1.0f;
    prevInGate_ = false;
    pushParameters (0.0f, 0.0f, 0.0f);
}

void CapicolaProcessor::pushParameters (float sigIn, float sigOut, float sigWheel)
{
    for (size_t p = 0; p < (size_t) capi::numPots; ++p)
    {
        const float sig = route_[p]->getIndex() == 0 ? sigIn
                        : route_[p]->getIndex() == 1 ? sigOut
                                                     : sigWheel;
        const float pos = perf_[p]->convertTo0to1 (perf_[p]->get()) + depth_[p]->get() * sig;
        const float n   = juce::jlimit (0.0f, 1.0f, pos);

        switch (p)
        {
            case 0: engine_.setPitch     (capi::pitchDetent (-12.0f + 24.0f * n)); break;
            case 1: engine_.setStretch   (capi::stretchCurve (n));                 break;
            case 2: engine_.setThreshold (capi::thresholdRatio (n));               break;
            case 3: engine_.setGrainSize (32.0f + 4064.0f * n);                    break;
            case 4: engine_.setQuality   (capi::qualityEpsilon (n));               break;
            default: engine_.setFeedback (1.5f * n);                               break;
        }
    }

    engine_.setSmoothing  (sec_[0]->get());
    engine_.setDrive      (sec_[2]->get());
    engine_.setCharacter  (sec_[3]->get());
    engine_.setMix        (sec_[4]->get());
    engine_.setFeedbackTone (sec_[5]->get());

    const float fade = sec_[1]->get();
    if (! juce::exactlyEqual (fade, lastFade_))
    {
        lastFade_ = fade;
        engine_.setFade (fade);
        setLatencySamples (engine_.latencySamples());
    }
}

void CapicolaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    if (numSamples == 0 || numIn == 0 || numOut == 0)
        return;

    if (scratch_.getNumSamples() < numSamples)
        scratch_.setSize (2, numSamples, false, true, true);

    scratch_.copyFrom (0, 0, buffer, 0, 0, numSamples);
    scratch_.copyFrom (1, 0, buffer, numIn > 1 ? 1 : 0, 0, numSamples);

    float* l = scratch_.getWritePointer (0);
    float* r = scratch_.getWritePointer (1);

    auto midiPos = midi.begin();
    int slices = panel.sliceCount.load (std::memory_order_relaxed);

    for (int offset = 0; offset < numSamples; offset += kControlChunk)
    {
        const int n = juce::jmin (kControlChunk, numSamples - offset);

        /* The module's two input jacks arrive over MIDI: any note-on splices,
           and the mod wheel is the bipolar modulation source. */
        for (; midiPos != midi.end() && (*midiPos).samplePosition < offset + n; ++midiPos)
        {
            const auto msg = (*midiPos).getMessage();
            if (msg.isNoteOn())
                sliceRequest_.store (true);
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modIn_ = msg.getControllerValue() / 63.5f - 1.0f;
        }

        if (sliceRequest_.exchange (false))
        {
            engine_.triggerSlice();
            ++slices;
        }

        pushParameters (engine_.envIn(), engine_.envOut(), modIn_);
        engine_.process (l + offset, r + offset, n);

        const bool gate = engine_.inGate();
        if (gate && ! prevInGate_)
            ++slices;
        prevInGate_ = gate;
    }

    if (limiterOn_->get())
        limiter_.process (l, r, numSamples);
    else
        limiter_.reset();

    buffer.copyFrom (0, 0, scratch_, 0, 0, numSamples);
    if (numOut > 1)
        buffer.copyFrom (1, 0, scratch_, 1, 0, numSamples);
    for (int ch = 2; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    panel.envIn.store (engine_.envIn(), std::memory_order_relaxed);
    panel.envOut.store (engine_.envOut(), std::memory_order_relaxed);
    panel.inPeak.store (engine_.inPeak(), std::memory_order_relaxed);
    panel.inGate.store (engine_.inGate(), std::memory_order_relaxed);
    panel.outGate.store (engine_.outGate(), std::memory_order_relaxed);
    panel.modIn.store (modIn_, std::memory_order_relaxed);
    panel.sliceCount.store (slices, std::memory_order_relaxed);
    panel.reduction.store (limiterOn_->get() ? limiter_.readReduction() : 1.0f,
                           std::memory_order_relaxed);

    midi.clear();
}

juce::AudioProcessorEditor* CapicolaProcessor::createEditor()
{
    return new CapicolaEditor (*this);
}

void CapicolaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CapicolaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid())
            return;

        apvts.replaceState (state);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CapicolaProcessor();
}
