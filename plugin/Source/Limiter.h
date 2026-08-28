#pragma once

#include <cmath>

namespace capi
{

/**
 * Output limiter — the same one the web build runs.
 *
 * Feedback and drive can push the output past 0 dBFS. That is the module's
 * own behaviour, and in a DAW it is often what you want, so this is off
 * unless asked for. Peak follower with an instant attack and a slow release, then a
 * hard clamp for the little a 2 ms attack lets through. No lookahead: it never
 * adds latency, and what slips past is brief and small enough for the clamp.
 */
class Limiter
{
public:
    void prepare (double sampleRate)
    {
        const double sr = sampleRate > 0.0 ? sampleRate : 48000.0;
        attack_  = (float) (1.0 - std::exp (-1.0 / (0.002 * sr)));
        release_ = (float) (1.0 - std::exp (-1.0 / (0.150 * sr)));
        reset();
    }

    void reset() { env_ = 0.0f; gain_ = 1.0f; reduction_ = 1.0f; }

    void process (float* left, float* right, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const float a = std::fmax (std::fabs (left[i]), std::fabs (right[i]));
            env_ = a > env_ ? a : env_ + (a - env_) * release_;

            const float target = env_ > kCeiling ? kCeiling / env_ : 1.0f;
            gain_ += (target - gain_) * (target < gain_ ? attack_ : release_);

            const float l = left[i] * gain_;
            const float r = right[i] * gain_;
            left[i]  = l < -1.0f ? -1.0f : (l > 1.0f ? 1.0f : l);
            right[i] = r < -1.0f ? -1.0f : (r > 1.0f ? 1.0f : r);

            if (gain_ < reduction_)
                reduction_ = gain_;
        }
    }

    /** Lowest gain since the last read, so the panel can show it working. */
    float readReduction()
    {
        const float r = reduction_;
        reduction_ = 1.0f;
        return r;
    }

private:
    static constexpr float kCeiling = 0.92f;

    float attack_ = 0.0f, release_ = 0.0f;
    float env_ = 0.0f, gain_ = 1.0f, reduction_ = 1.0f;
};

} // namespace capi
