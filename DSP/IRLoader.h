#pragma once

#include <vector>

#include "IRData/IRData.h"

enum class SpeakerIR
{
    HS200Close,
    VL1Edge
};

const SpeakerIRData &getSpeakerIRData(SpeakerIR ir) noexcept;

std::vector<float> resampleSpeakerIR(SpeakerIR ir, int targetSampleRate);
