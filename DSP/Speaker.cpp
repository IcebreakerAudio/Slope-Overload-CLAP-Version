#include "Speaker.h"

#include <algorithm>

#include "IA_Waveshaping/BasicClippers.hpp"
#include "IRLoader.h"

void Speaker::initialize(double sampleRate, int maxBlockSize, int numChannels)
{
    fadeLength = std::max(1, static_cast<int>(sampleRate * 0.02));  // ~20ms crossfade

    dryScratch.assign(static_cast<size_t>(numChannels), std::vector<float>(static_cast<size_t>(maxBlockSize), 0.0f));
    fromScratch = dryScratch;
    toScratch = dryScratch;
    zeroScratch.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    dummyScratch.assign(static_cast<size_t>(maxBlockSize), 0.0f);

    static constexpr std::array<SpeakerIR, numIRs> irs{SpeakerIR::HS200Close, SpeakerIR::VL1Edge};
    for (size_t i = 0; i < numIRs; ++i)
    {
        auto irSamples = resampleSpeakerIR(irs[i], static_cast<int>(sampleRate));
        banks[i].irLengthSamples = irSamples.size();
        banks[i].flushRemaining = 0;
        banks[i].channels.clear();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto conv = std::make_unique<fftconvolver::FFTConvolver>();
            conv->init(static_cast<size_t>(maxBlockSize), irSamples.data(), irSamples.size());
            banks[i].channels.push_back(std::move(conv));
        }
    }

    currentChoice = -1;
    fadeFromChoice = -1;
    fadeToChoice = -1;
    fadeRemaining = 0;
}

void Speaker::setSpeaker(int choice) noexcept
{
    const auto effectiveCurrent = fadeRemaining > 0 ? fadeToChoice : currentChoice;
    if (choice == effectiveCurrent)
    {
        return;
    }

    if (fadeRemaining > 0)
    {
        finalizeFade();
    }

    fadeFromChoice = currentChoice;
    fadeToChoice = choice;
    fadeRemaining = fadeLength;

    if (fadeFromChoice >= 0)
    {
        banks[static_cast<size_t>(fadeFromChoice)].flushRemaining = 0;
    }
    if (fadeToChoice >= 0)
    {
        banks[static_cast<size_t>(fadeToChoice)].flushRemaining = 0;
    }
}

void Speaker::finalizeFade() noexcept
{
    currentChoice = fadeToChoice;
    if (fadeFromChoice >= 0)
    {
        banks[static_cast<size_t>(fadeFromChoice)].flushRemaining =
            static_cast<long>(banks[static_cast<size_t>(fadeFromChoice)].irLengthSamples);
    }
    fadeFromChoice = -1;
    fadeToChoice = -1;
    fadeRemaining = 0;
}

void Speaker::process(AudioBuffer &buffer) noexcept
{
    const auto numCh = std::min(buffer.numChannels(), static_cast<uint32_t>(banks[0].channels.size()));
    const auto numFrames = static_cast<int>(buffer.numFrames());

    for (auto &bank : banks)
    {
        if (bank.flushRemaining > 0)
        {
            const auto n = std::min<long>(bank.flushRemaining, numFrames);
            for (uint32_t ch = 0; ch < numCh; ++ch)
            {
                bank.channels[ch]->process(zeroScratch.data(), dummyScratch.data(), static_cast<size_t>(n));
            }
            bank.flushRemaining -= n;
        }
    }

    if (fadeRemaining > 0)
    {
        for (uint32_t ch = 0; ch < numCh; ++ch)
        {
            auto span = buffer.channel(ch);
            std::copy(span.begin(), span.end(), dryScratch[ch].begin());
            if (fadeFromChoice >= 0)
            {
                banks[static_cast<size_t>(fadeFromChoice)].channels[ch]->process(
                    dryScratch[ch].data(), fromScratch[ch].data(), buffer.numFrames());
            }
            if (fadeToChoice >= 0)
            {
                banks[static_cast<size_t>(fadeToChoice)].channels[ch]->process(
                    dryScratch[ch].data(), toScratch[ch].data(), buffer.numFrames());
            }
        }

        for (int i = 0; i < numFrames; ++i)
        {
            const auto t = std::clamp(
                static_cast<float>(fadeLength - fadeRemaining + i + 1) / static_cast<float>(fadeLength), 0.0f, 1.0f);
            for (uint32_t ch = 0; ch < numCh; ++ch)
            {
                auto span = buffer.channel(ch);
                const auto idx = static_cast<size_t>(i);
                const auto fromSample = fadeFromChoice < 0
                                             ? dryScratch[ch][idx]
                                             : IADSP::BasicClippers::cubicSoftClip(fromScratch[ch][idx]);
                const auto toSample = fadeToChoice < 0 ? dryScratch[ch][idx]
                                                        : IADSP::BasicClippers::cubicSoftClip(toScratch[ch][idx]);
                span[idx] = fromSample * (1.0f - t) + toSample * t;
            }
        }

        fadeRemaining = std::max(0, fadeRemaining - numFrames);
        if (fadeRemaining == 0)
        {
            finalizeFade();
        }
    }
    else if (currentChoice >= 0)
    {
        for (uint32_t ch = 0; ch < numCh; ++ch)
        {
            auto span = buffer.channel(ch);
            std::copy(span.begin(), span.end(), dryScratch[ch].begin());
            banks[static_cast<size_t>(currentChoice)].channels[ch]->process(dryScratch[ch].data(), span.data(),
                                                                             buffer.numFrames());
            for (auto &s : span)
            {
                s = IADSP::BasicClippers::cubicSoftClip(s);
            }
        }
    }
    // currentChoice < 0 and not fading: pure bypass, buffer left untouched.
}
