#include "IRLoader.h"

#include <cmath>

#include <IA_Utilities/ResamplingFilter.hpp>

#include "IRData/HS200_SM58_Close.h"
#include "IRData/VL1_SM58_Edge.h"

const SpeakerIRData &getSpeakerIRData(SpeakerIR ir) noexcept
{
    switch (ir)
    {
    case SpeakerIR::HS200Close:
        return HS200_SM58_Close;
    case SpeakerIR::VL1Edge:
        return VL1_SM58_Edge;
    }

    return HS200_SM58_Close;
}

std::vector<float> resampleSpeakerIR(SpeakerIR ir, int targetSampleRate)
{
    const SpeakerIRData &data = getSpeakerIRData(ir);
    const auto sampleCount = data.samples.size();

    if (targetSampleRate == data.sampleRate)
    {
        return std::vector<float>(data.samples.begin(), data.samples.end());
    }

    const double ratio = static_cast<double>(data.sampleRate) / static_cast<double>(targetSampleRate);
    const auto outputCount = static_cast<size_t>(std::ceil(static_cast<double>(sampleCount) / ratio));

    ResamplingFilter filter;
    filter.prepare(1, static_cast<int>(sampleCount));
    filter.setResamplingRatio(ratio);

    std::vector<float> output(outputCount);
    filter.processChannel(data.samples.data(), output.data(), sampleCount);

    return output;
}
