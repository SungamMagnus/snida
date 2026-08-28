#include "CapicolaEngine.h"

#include <algorithm>
#include <cmath>

namespace capi
{

using namespace capicola;

namespace
{
/* Stereo re-alignment guardrail: when exactly one channel auto-fires and the
   two grids have drifted past this, the quiet channel is forced to re-anchor. */
constexpr double kMaxDriftSeconds = 1.0;

/* Cancels the sinc shaper's 2x small-signal gain so the loop matches tanh. */
constexpr float kFbSatGain = 0.5f;

/* The low-passed Teager energy is tiny; norm = sqrt(clamp01(env * kEnvGain)). */
constexpr float kEnvGain = 200.0f;

inline float envNorm (float env)
{
    float e = env * kEnvGain;
    if (e < 0.0f) e = 0.0f;
    else if (e > 1.0f) e = 1.0f;
    return std::sqrt (e);
}

/* Hz -> the filters' normalised cutoff (0 .. fs/2). */
inline float normFc (float hz, double fs)
{
    const float fc = (float) (2.0 * (double) hz / fs);
    return fc < 1.0e-6f ? 1.0e-6f : (fc > 0.99f ? 0.99f : fc);
}
} // namespace

CapicolaEngine::CapicolaEngine()
    : sparseL_ (std::make_unique<Recorder>()),
      sparseR_ (std::make_unique<Recorder>())
{
}

CapicolaEngine::~CapicolaEngine() = default;

void CapicolaEngine::prepare (double sampleRate)
{
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
    shapers_.Init();
    reset();
}

void CapicolaEngine::reset()
{
    const float fs = (float) sampleRate_;

    sparseL_->Init (&shapers_, fs);
    sparseR_->Init (&shapers_, fs);
    for (auto* r : { sparseL_.get(), sparseR_.get() })
    {
        r->SetThreshold (0.001f);
        r->SetGrainPitch (1.0f);
        r->SetGrainLeash (128);
        r->SetGrainFade (fadeSamples_);
    }

    fbSvfL_.Init();
    fbSvfR_.Init();
    fbSvfL_.SetControls (normFc (480.0f, sampleRate_), 0.01f);
    fbSvfR_.SetControls (normFc (480.0f, sampleRate_), 0.01f);

    outDet_.Init (fs);
    outDet_.SetCutoff (normFc (34.0f, sampleRate_));

    for (int i = 0; i < kChunk; ++i) { fbL_[i] = fbR_[i] = 0.0f; }

    inGate_ = outGate_ = false;
    envNormIn_ = envNormOut_ = inPeak_ = 0.0f;

    sparseL_->SubmitRequest (Request::LIVE_EFFECT);
    sparseR_->SubmitRequest (Request::LIVE_EFFECT);
}

void CapicolaEngine::process (float* left, float* right, int n)
{
    for (int offset = 0; offset < n; offset += kChunk)
        processChunk (left + offset, right + offset, std::min (kChunk, n - offset));
}

void CapicolaEngine::processChunk (float* left, float* right, int n)
{
    /* The recorders write into the output buffers, so the dry signal the blend
       and the peak meter need has to be stashed first. */
    for (int i = 0; i < n; ++i) { dryL_[i] = left[i]; dryR_[i] = right[i]; }

    /* Feedback: last chunk's wet output -> sinc saturator -> bandpass, scaled
       and summed into the recorder input with a hard +/-1 clamp on injection. */
    if (fbAmt_ > 0.0f)
    {
        for (int i = 0; i < n; ++i)
        {
            fbSvfL_.Tick (kFbSatGain * shapers_.ReadSinc (fbL_[i]));
            fbSvfR_.Tick (kFbSatGain * shapers_.ReadSinc (fbR_[i]));

            float injL = fbSvfL_.GetBandpass() * fbAmt_;
            float injR = fbSvfR_.GetBandpass() * fbAmt_;
            injL = injL > 1.0f ? 1.0f : (injL < -1.0f ? -1.0f : injL);
            injR = injR > 1.0f ? 1.0f : (injR < -1.0f ? -1.0f : injR);

            left[i]  += injL;
            right[i] += injR;
        }
    }

    sparseL_->ProcessBlock (left,  left,  (std::size_t) n);
    sparseR_->ProcessBlock (right, right, (std::size_t) n);

    const bool firedL = sparseL_->FiredThisBlock();
    const bool firedR = sparseR_->FiredThisBlock();
    if (firedL != firedR)
    {
        const double lagL = sparseL_->GridLag();
        const double lagR = sparseR_->GridLag();
        const double drift = lagL > lagR ? lagL - lagR : lagR - lagL;
        if (drift > kMaxDriftSeconds * sampleRate_)
            (firedL ? *sparseR_ : *sparseL_).SubmitRequest (Request::SLICE);
    }

    /* Stash the wet output for the next chunk's feedback, before the blend. */
    for (int i = 0; i < n; ++i) { fbL_[i] = left[i]; fbR_[i] = right[i]; }

    const float wet = mix_;
    const float dry = 1.0f - wet;
    for (int i = 0; i < n; ++i)
    {
        left[i]  = wet * left[i]  + dry * dryL_[i];
        right[i] = wet * right[i] + dry * dryR_[i];
    }

    /* Output follower on the final post-mix mono sum, fed at 2x (no halving):
       TKEO is quadratic, so this lifts it 4x to sit level with the input side. */
    for (int i = 0; i < n; ++i)
        outDet_.Analyze (left[i] + right[i]);

    const float envL = sparseL_->TkeoEnvelope();
    const float envR = sparseR_->TkeoEnvelope();
    envNormIn_  = envNorm (envL > envR ? envL : envR);
    envNormOut_ = envNorm (outDet_.Envelope());
    inGate_     = sparseL_->DetectorGate() || sparseR_->DetectorGate();
    outGate_    = outDet_.Gate();

    constexpr float kPeakDecay = 0.95f;
    float pk = inPeak_ * kPeakDecay;
    for (int i = 0; i < n; ++i)
    {
        const float al = std::fabs (dryL_[i]);
        const float ar = std::fabs (dryR_[i]);
        if (al > pk) pk = al;
        if (ar > pk) pk = ar;
    }
    inPeak_ = pk;
}

void CapicolaEngine::setPitch (float semis)
{
    const float v = std::exp2 (semis / 12.0f);
    sparseL_->SetGrainPitch (v);
    sparseR_->SetGrainPitch (v);
}

void CapicolaEngine::setStretch (float stretch)
{
    sparseL_->SetGrainStretch (stretch);
    sparseR_->SetGrainStretch (stretch);
}

void CapicolaEngine::setThreshold (float ratio)
{
    sparseL_->SetTransientThreshold (ratio);
    sparseR_->SetTransientThreshold (ratio);
    outDet_.SetThreshold (ratio);
}

void CapicolaEngine::setGrainSize (float keyframes)
{
    const int k = (int) (keyframes + 0.5f);
    sparseL_->SetGrainLeash (k);
    sparseR_->SetGrainLeash (k);
}

void CapicolaEngine::setQuality (float epsilon)
{
    sparseL_->SetThreshold (epsilon);
    sparseR_->SetThreshold (epsilon);
}

void CapicolaEngine::setMix (float wet01)
{
    mix_ = wet01 < 0.0f ? 0.0f : (wet01 > 1.0f ? 1.0f : wet01);
}

void CapicolaEngine::setFeedback (float amount)
{
    fbAmt_ = amount < 0.0f ? 0.0f : amount;
}

void CapicolaEngine::setFeedbackTone (float hz)
{
    const float fc = normFc (hz, sampleRate_);
    fbSvfL_.SetControls (fc, 0.01f);
    fbSvfR_.SetControls (fc, 0.01f);
}

void CapicolaEngine::setSmoothing (float hz)
{
    const float fc = normFc (hz, sampleRate_);
    sparseL_->SetTkeoCutoff (fc);
    sparseR_->SetTkeoCutoff (fc);
    outDet_.SetCutoff (fc);
}

void CapicolaEngine::setFade (float milliseconds)
{
    fadeSamples_ = (float) (milliseconds * 0.001 * sampleRate_);
    if (fadeSamples_ < 1.0f) fadeSamples_ = 1.0f;
    sparseL_->SetGrainFade (fadeSamples_);
    sparseR_->SetGrainFade (fadeSamples_);
}

void CapicolaEngine::setDrive (float drive)
{
    sparseL_->SetDistortDrive (drive);
    sparseR_->SetDistortDrive (drive);
}

void CapicolaEngine::setCharacter (float character)
{
    sparseL_->SetDistortCharacter (character);
    sparseR_->SetDistortCharacter (character);
}

void CapicolaEngine::triggerSlice()
{
    if (sparseL_->GetState() == State::LIVE_EFFECT)
    {
        sparseL_->SubmitRequest (Request::SLICE);
        sparseR_->SubmitRequest (Request::SLICE);
    }
}

} // namespace capi
