#pragma once

class ScopeSource
{
public:
    virtual ~ScopeSource() = default;

    virtual int samplesAvailable() const noexcept = 0;
    virtual int read(float *dest, int maxSamples) noexcept = 0;
};
