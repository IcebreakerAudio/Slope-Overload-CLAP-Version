#pragma once

#include <pmmintrin.h>
#include <xmmintrin.h>

// RAII guard that flushes denormals to zero (FTZ) and treats denormal inputs as zero (DAZ)
// for the duration of its scope. x86/SSE-specific; matches what juce::ScopedNoDenormals did
// for every processBlock() call in the original plugin.
class ScopedNoDenormals
{
public:
    ScopedNoDenormals() noexcept : previousMXCSR(_mm_getcsr()) { _mm_setcsr(previousMXCSR | 0x8040u); }

    ~ScopedNoDenormals() noexcept { _mm_setcsr(previousMXCSR); }

    ScopedNoDenormals(const ScopedNoDenormals &) = delete;
    ScopedNoDenormals &operator=(const ScopedNoDenormals &) = delete;

private:
    unsigned int previousMXCSR;
};
