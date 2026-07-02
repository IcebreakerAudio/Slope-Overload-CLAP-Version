#pragma once

#include <algorithm>
#include <cstdint>
#include <span>

// A `const AudioBuffer` only prevents reseating the view itself (its pointer/counts);
// it does not stop writes through the channel data it exposes.
class AudioBuffer
{
public:
    AudioBuffer() noexcept = default;
    AudioBuffer(float **channels, uint32_t numChannels, uint32_t numFrames) noexcept
        : _channels(channels), _numChannels(numChannels), _numFrames(numFrames)
    {
    }

    uint32_t numChannels() const noexcept { return _numChannels; }
    uint32_t numFrames() const noexcept { return _numFrames; }

    std::span<float> channel(uint32_t index) const noexcept { return {_channels[index], _numFrames}; }
    float **data() const noexcept { return _channels; }

    void copyFrom(const AudioBuffer &source) noexcept;
    void clear() noexcept;

private:
    float **_channels = nullptr;
    uint32_t _numChannels = 0;
    uint32_t _numFrames = 0;
};

inline void AudioBuffer::copyFrom(const AudioBuffer &source) noexcept
{
    const uint32_t n = std::min(_numChannels, source._numChannels);
    for (uint32_t ch = 0; ch < n; ++ch)
    {
        std::ranges::copy(source.channel(ch), channel(ch).begin());
    }
}

inline void AudioBuffer::clear() noexcept
{
    for (uint32_t ch = 0; ch < _numChannels; ++ch)
    {
        std::ranges::fill(channel(ch), 0.0f);
    }
}
