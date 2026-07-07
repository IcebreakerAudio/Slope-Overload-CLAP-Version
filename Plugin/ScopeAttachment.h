#pragma once

#include "IA_Utilities/FiFo.hpp"
#include "ScopeSource.h"

#include <algorithm>

class ScopeAttachment : public ScopeSource
{
public:
    explicit ScopeAttachment(Fifo<float> &fifo) noexcept : boundFifo(fifo) {}

    int samplesAvailable() const noexcept override { return boundFifo.getSizeToRead(); }

    int read(float *dest, int maxSamples) noexcept override
    {
        const int numToRead = std::min(boundFifo.getSizeToRead(), maxSamples);
        if (numToRead <= 0)
        {
            return 0;
        }
        boundFifo.readFromFifo(dest, numToRead);
        return numToRead;
    }

private:
    Fifo<float> &boundFifo;
};
