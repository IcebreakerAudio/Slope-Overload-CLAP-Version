#pragma once

#include <span>

struct SpeakerIRData
{
    int sampleRate;
    std::span<const float> samples;
};
