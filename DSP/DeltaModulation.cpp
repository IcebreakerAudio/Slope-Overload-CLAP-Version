#include "DeltaModulation.h"

#include <algorithm>
#include <cmath>

namespace
{
// env is always >= 0 (it's an envelope-follower output), so this only ever needs the
// non-negative-base case - no need for pow()'s general handling of negative bases/NaN/Inf. The
// gate curve's exponent is a fixed 50, so exponentiation-by-squaring (50 = 32+16+2) replaces the
// transcendental pow() call with 7 multiplies in this per-sample, oversampled-rate hot path.
float gatePow50(float x) noexcept
{
    const auto x2 = x * x;
    const auto x4 = x2 * x2;
    const auto x8 = x4 * x4;
    const auto x16 = x8 * x8;
    const auto x32 = x16 * x16;
    return x32 * x16 * x2;
}
}  // namespace

void DeltaModulation::initialize(double sampleRate, int maxBlockSize, int numChannels)
{
    int numStages = 0;
    double rate = sampleRate;
    while (rate < targetSampleRate)
    {
        rate *= 2.0;
        ++numStages;
    }

    oversampler.setNumStages(numStages);
    oversampler.prepare(numChannels, maxBlockSize);
    oversampledRate = sampleRate * static_cast<double>(1 << numStages);

    rmsFollower.setNumChannels(numChannels);
    rmsFollower.setSampleRate(oversampledRate);
    rmsFollower.setAttackTime(0.0f);
    rmsFollower.setReleaseTime(50.0f);

    peakFollower.setNumChannels(numChannels);
    peakFollower.setSampleRate(oversampledRate);
    peakFollower.setAttackTime(0.0f);
    peakFollower.setReleaseTime(10.0f);

    dcPreFilter.setNumChannels(numChannels);
    dcPreFilter.setSampleRate(sampleRate);
    dcPreFilter.setCutoffFrequency(20.0);

    dcPostFilter.setNumChannels(numChannels);
    dcPostFilter.setSampleRate(sampleRate);
    dcPostFilter.setCutoffFrequency(20.0);

    highBoost.setNumChannels(numChannels);
    highBoost.setSampleRate(sampleRate);
    highBoost.setFrequency(1000.0f);
    highBoost.setGainDB(6.0f);

    for (auto &f : aaFilters)
    {
        f.setNumChannels(numChannels);
        f.setSampleRate(sampleRate);
        f.setResonance(aaResonance);
    }
    postFilter.setNumChannels(numChannels);
    postFilter.setSampleRate(sampleRate);
    postFilter.setResonance(aaResonance);

    z1.assign(numChannels, 0.0f);
    heldOutput.assign(numChannels, 0.0f);
    clockPhase.assign(numChannels, 0.0);

    initialized = true;
    updateInternalRate();

    reset();
}

void DeltaModulation::reset()
{
    std::fill(z1.begin(), z1.end(), 63.0f);
    std::fill(heldOutput.begin(), heldOutput.end(), 0.0f);
    std::fill(clockPhase.begin(), clockPhase.end(), 1.0);

    highBoost.reset();
    for (auto &f : aaFilters)
    {
        f.reset();
    }
    postFilter.reset();
    oversampler.reset();
    rmsFollower.reset();
    peakFollower.reset();
    dcPreFilter.reset();
    dcPostFilter.reset();
}

void DeltaModulation::setSampleRateIndex(int index) noexcept
{
    const auto clamped = std::clamp(index, 0, 15);
    if (clamped != srIndex)
    {
        srIndex = clamped;
        updateInternalRate();
    }
}

void DeltaModulation::setSystem(System newSystem) noexcept
{
    if (newSystem != system)
    {
        system = newSystem;
        updateInternalRate();
    }
}

void DeltaModulation::setAntiAliasing(bool enabled) noexcept
{
    if (enabled != antiAliasing)
    {
        antiAliasing = enabled;
        for (auto &f : aaFilters)
        {
            f.reset();
        }
        postFilter.reset();
    }
}

int DeltaModulation::getLatencySamples() const noexcept
{
    return static_cast<int>(oversampler.getLatency());
}

void DeltaModulation::updateInternalRate() noexcept
{
    if (!initialized)
    {
        return;
    }

    internalSampleRate = (system == System::PAL) ? srLookupPAL[static_cast<size_t>(srIndex)]
                                                   : srLookupNTSC[static_cast<size_t>(srIndex)];
    clockInc = internalSampleRate / oversampledRate;

    const auto cutoff = internalSampleRate * 0.5;
    for (auto &f : aaFilters)
    {
        f.setCutoffFrequency(cutoff);
    }
    postFilter.setCutoffFrequency(cutoff);
}

float DeltaModulation::processSample(float inputValue, int channel) noexcept
{
    auto env = rmsFollower.processSample(inputValue, channel);
    env = peakFollower.processSample(env, channel);

    if (clockPhase[static_cast<size_t>(channel)] >= 1.0)
    {
        clockPhase[static_cast<size_t>(channel)] -= 1.0;

        auto x = inputValue * bitFactor + bitFactor;
        x = std::clamp(x, 0.0f, bitDepth);

        const auto target = std::round(x);
        const auto step = (target > z1[static_cast<size_t>(channel)]) ? 1.0f : -1.0f;
        z1[static_cast<size_t>(channel)] += step;

        heldOutput[static_cast<size_t>(channel)] = z1[static_cast<size_t>(channel)] / bitFactor - 1.0f;
    }
    clockPhase[static_cast<size_t>(channel)] += clockInc;

    const auto gain = (env > threshold) ? 1.0f : gatePow50(env * bitFactor);
    return heldOutput[static_cast<size_t>(channel)] * gain;
}

void DeltaModulation::process(AudioBuffer &buffer) noexcept
{
    const auto numChannels = buffer.numChannels();

    for (uint32_t ch = 0; ch < numChannels; ++ch)
    {
        for (auto &s : buffer.channel(ch))
        {
            if (antiAliasing)
            {
                for (auto &f : aaFilters)
                {
                    s = f.processSample(s, static_cast<int>(ch));
                }
            }
            s = dcPreFilter.processSample(s, static_cast<int>(ch));
            s += highBoost.processSample(s, static_cast<int>(ch));
        }
    }

    const auto numUp = oversampler.upsample(buffer.data(), buffer.numFrames());
    auto **os = oversampler.getInternalBuffer();

    for (uint32_t ch = 0; ch < numChannels; ++ch)
    {
        for (size_t i = 0; i < numUp; ++i)
        {
            os[ch][i] = processSample(os[ch][i], static_cast<int>(ch));
        }
    }

    oversampler.downsample(buffer.data(), buffer.numFrames());

    for (uint32_t ch = 0; ch < numChannels; ++ch)
    {
        for (auto &s : buffer.channel(ch))
        {
            if (antiAliasing)
            {
                s = postFilter.processSample(s, static_cast<int>(ch));
            }
            s = dcPostFilter.processSample(s, static_cast<int>(ch));
        }
    }

    if (antiAliasing)
    {
        for (auto &f : aaFilters)
        {
            f.snapToZero();
        }
        postFilter.snapToZero();
    }
    dcPreFilter.snapToZero();
    dcPostFilter.snapToZero();
}
