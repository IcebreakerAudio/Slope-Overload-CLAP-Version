#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include "AudioBuffer.h"
#include "FFTConvolver.h"

// Speaker convolution using FFTConvolver. Both real IRs are pre-built once in initialize()
// - there are only ever two, known in advance - so switching between them at runtime just
// means choosing which already-built convolver's output to use: no dynamic IR loading, no
// allocation on the audio thread, no background thread needed at all. Switching choices
// (including to/from bypass) crossfades over a short window, and the convolver being
// switched away from is fed a burst of silence afterward to drain its internal
// overlap-add tail, so a later reselect doesn't resurrect stale audio.
class Speaker
{
public:
    void initialize(double sampleRate, int maxBlockSize, int numChannels);
    void setSpeaker(int choice) noexcept;  // -1 = off/bypass, 0 = HS200Close, 1 = VL1Edge

    void process(AudioBuffer &buffer) noexcept;

private:
    static constexpr int numIRs = 2;

    struct ConvolverBank
    {
        std::vector<std::unique_ptr<fftconvolver::FFTConvolver>> channels;
        size_t irLengthSamples = 0;  // resampled length at the host rate; also the flush duration
        long flushRemaining = 0;     // > 0 while draining silence after being faded away from
    };

    void finalizeFade() noexcept;

    std::array<ConvolverBank, numIRs> banks;  // banks[0] = HS200Close, banks[1] = VL1Edge

    int currentChoice = -1;
    int fadeFromChoice = -1;
    int fadeToChoice = -1;
    int fadeRemaining = 0;
    int fadeLength = 1;

    std::vector<std::vector<float>> dryScratch, fromScratch, toScratch;  // [channel][maxBlockSize]
    std::vector<float> zeroScratch, dummyScratch;                       // shared across channels
};
