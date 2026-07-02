#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "AudioBuffer.h"
#include "IA_Filters/EQ/OnePoleEQFilter.hpp"
#include "IA_Filters/FirstOrderFilter.hpp"
#include "IA_Filters/SecondOrderFilter.hpp"
#include "IA_Utilities/EnvelopeFollower.hpp"
#include "IA_Utilities/Oversampler.hpp"

// 1-bit delta-modulation encoder emulating the NES/Famicom DPCM sample channel.
// Signal flow: anti-aliasing pre-filter -> DC pre-filter + high-frequency boost
// -> oversample to ~133kHz -> 1-bit delta quantization (7-bit depth) -> downsample
// -> anti-aliasing post-filter -> DC post-filter.
class DeltaModulation
{
public:
    enum class System { PAL, NTSC };

    void initialize(double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setSampleRateIndex(int index) noexcept;
    void setSystem(System newSystem) noexcept;
    void setAntiAliasing(bool enabled) noexcept;

    int getLatencySamples() const noexcept;

    void process(AudioBuffer &buffer) noexcept;

private:
    float processSample(float inputValue, int channel) noexcept;
    void updateInternalRate() noexcept;

    static constexpr double targetSampleRate = 133000.0;
    static constexpr float bitDepth = 127.0f;
    static constexpr float bitFactor = bitDepth * 0.5f;
    static constexpr float threshold = 1.0f / bitFactor;
    static constexpr float gateRatio = 50.0f;
    // IADSP::SecondOrderFilter's "resonance" is not a raw Q value: from its coefficient math
    // (p = 2*(1-resonance) plays the role of 1/Q), replicating the original's Butterworth-flat
    // Q = 1/sqrt(2) requires resonance = 1 - sqrt(2)/2, not 1/sqrt(2) itself.
    static constexpr float aaResonance = 0.292893f;

    static constexpr std::array<double, 16> srLookupPAL{4177.4,  4696.63, 5261.41, 5579.22, 6023.94,  7044.94,
                                                        7917.18, 8397.01, 9446.63, 11233.8, 12595.5,  14089.9,
                                                        16965.4, 21315.5, 25191.0, 33252.1};
    static constexpr std::array<double, 16> srLookupNTSC{4181.71, 4709.93, 5264.04, 5593.04, 6257.95,  7046.35,
                                                         7919.35, 8363.42, 9419.86, 11186.1, 12604.0,  13982.6,
                                                         16884.6, 21306.8, 24858.0, 33143.9};

    IADSP::OnePoleEQFilter<float> highBoost{IADSP::OnePoleEQFilterMode::HighPass};
    std::array<IADSP::SecondOrderFilter<float>, 4> aaFilters;
    IADSP::SecondOrderFilter<float> postFilter;
    IADSP::Oversampler<float> oversampler;
    IADSP::EnvelopeFollower<float> rmsFollower{IADSP::EnvelopeFollowerMode::RMS};
    IADSP::EnvelopeFollower<float> peakFollower{IADSP::EnvelopeFollowerMode::Peak};
    IADSP::FirstOrderFilter<float> dcPreFilter{IADSP::FirstOrderFilterMode::Highpass};
    IADSP::FirstOrderFilter<float> dcPostFilter{IADSP::FirstOrderFilterMode::Highpass};

    std::vector<float> z1, heldOutput;
    std::vector<double> clockPhase;
    bool antiAliasing = true;
    bool initialized = false;
    int srIndex = 7;
    System system = System::PAL;
    double internalSampleRate = 0.0;
    double oversampledRate = 0.0;
    double clockInc = 1.0;
};
