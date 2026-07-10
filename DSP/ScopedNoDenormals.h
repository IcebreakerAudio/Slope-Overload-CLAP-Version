#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define SLOPEOVERLOAD_DENORMALS_X86 1
    #include <pmmintrin.h>
    #include <xmmintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define SLOPEOVERLOAD_DENORMALS_ARM64 1
#endif

// RAII guard that flushes denormals to zero for the duration of its scope, matching what
// juce::ScopedNoDenormals did for every processBlock() call in the original plugin. x86 uses the
// MXCSR FTZ/DAZ bits; AArch64 uses the FPCR FZ bit (which alone covers both denormal inputs and
// outputs - there's no separate DAZ control). Any other architecture is a documented no-op.
class ScopedNoDenormals
{
public:
#if defined(SLOPEOVERLOAD_DENORMALS_X86)
    ScopedNoDenormals() noexcept : previousState(_mm_getcsr()) { _mm_setcsr(previousState | 0x8040u); }

    ~ScopedNoDenormals() noexcept { _mm_setcsr(previousState); }
#elif defined(SLOPEOVERLOAD_DENORMALS_ARM64)
    ScopedNoDenormals() noexcept : previousState(getFPCR()) { setFPCR(previousState | FZBit); }

    ~ScopedNoDenormals() noexcept { setFPCR(previousState); }
#else
    ScopedNoDenormals() noexcept = default;
    ~ScopedNoDenormals() noexcept = default;
#endif

    ScopedNoDenormals(const ScopedNoDenormals &) = delete;
    ScopedNoDenormals &operator=(const ScopedNoDenormals &) = delete;

private:
#if defined(SLOPEOVERLOAD_DENORMALS_ARM64)
    static constexpr std::uint64_t FZBit = 1ull << 24;

    static std::uint64_t getFPCR() noexcept
    {
        std::uint64_t value;
        asm volatile("mrs %0, fpcr" : "=r"(value));
        return value;
    }

    static void setFPCR(std::uint64_t value) noexcept { asm volatile("msr fpcr, %0" : : "r"(value)); }
#endif

#if defined(SLOPEOVERLOAD_DENORMALS_X86)
    unsigned int previousState;
#elif defined(SLOPEOVERLOAD_DENORMALS_ARM64)
    std::uint64_t previousState;
#endif
};

#undef SLOPEOVERLOAD_DENORMALS_X86
#undef SLOPEOVERLOAD_DENORMALS_ARM64
