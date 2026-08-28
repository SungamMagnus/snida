#pragma once

/**
 * Desktop port of src/audio/audio_engine.{h,cpp}. Same signal flow — feedback
 * injection → two independent KeyframeRecorder chains → dry/wet mix → output
 * follower — with the Daisy-specific parts (SDRAM globals, CpuLoadMeter, the
 * fixed 48 kHz / 64-sample callback) replaced by heap storage, the host sample
 * rate, and internal 64-sample chunking.
 */

#include <memory>

#include "Detector.h"
#include "KeyframeRecorder.h"
#include "Shapers.h"

namespace capi
{

class CapicolaEngine
{
public:
    CapicolaEngine();
    ~CapicolaEngine();

    void prepare (double sampleRate);
    void reset();

    /** Processes one stereo block in place. `n` may be any length; the engine
        chunks it into 64-sample sub-blocks so the reader fences, the guard
        keyframe cadence and the splice timing all match the hardware. */
    void process (float* left, float* right, int n);

    /* ── Parameter setters — engineering units, as on the module ─────────── */
    void setPitch          (float semis);
    void setStretch        (float stretch);      // 0.01 .. 1, 0 = frozen
    void setThreshold      (float ratio);        // TKEO keep ratio; >= 100 mutes
    void setGrainSize      (float keyframes);
    void setMix            (float wet01);
    void setFeedback       (float amount);
    void setFeedbackTone   (float hz);
    void setSmoothing      (float hz);           // follower cutoff
    void setFade           (float milliseconds); // OLA crossfade = latency
    void setDrive          (float drive);
    void setCharacter      (float character);    // 0 quake · 0.5 clean · 1 sinc
    void setQuality        (float epsilon);      // analyzer keyframe threshold

    void triggerSlice();

    /* ── Follower state, mirrored by the panel ───────────────────────────── */
    bool  inGate()     const noexcept { return inGate_; }
    bool  outGate()    const noexcept { return outGate_; }
    float envIn()      const noexcept { return envNormIn_; }
    float envOut()     const noexcept { return envNormOut_; }
    float inPeak()     const noexcept { return inPeak_; }

    /** Wet-path latency in samples — the current crossfade length. */
    int latencySamples() const noexcept { return (int) fadeSamples_; }

private:
    /* 2^20 keyframes x 12 B = 12 MB per channel. The module runs 2^21 in SDRAM;
       half that is still minutes of extrema at full quality, and keeps the
       plugin's footprint to 24 MB. */
    static constexpr int kRingFrames = 1 << 20;
    static constexpr int kChunk      = 64;   // the module's audio block size

    using Recorder = capicola::KeyframeRecorder<kRingFrames>;

    void processChunk (float* left, float* right, int n);

    std::unique_ptr<Recorder> sparseL_, sparseR_;
    capicola::Shapers         shapers_;

    capicola::StateVariable fbSvfL_, fbSvfR_;
    capicola::Detector      outDet_;

    double sampleRate_  = 48000.0;
    float  fadeSamples_ = 960.0f;
    float  mix_         = 1.0f;
    float  fbAmt_       = 0.0f;

    float fbL_[kChunk] = {}, fbR_[kChunk] = {};
    float dryL_[kChunk] = {}, dryR_[kChunk] = {};

    bool  inGate_ = false, outGate_ = false;
    float envNormIn_ = 0.0f, envNormOut_ = 0.0f, inPeak_ = 0.0f;
};

} // namespace capi
