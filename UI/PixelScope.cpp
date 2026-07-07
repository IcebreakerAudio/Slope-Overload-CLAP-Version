#include "PixelScope.h"

#include <algorithm>
#include <cmath>

PixelScope::PixelScope(ScopeSource &scopeSource) : source(scopeSource)
{
    dataMin.fill(0.0f);
    dataMax.fill(0.0f);

    startTimer(pollIntervalMs);
}

void PixelScope::timerCallback()
{
    pollFifo();
}

float PixelScope::normalize(float value) noexcept
{
    value = value * 0.5f + 0.5f;
    value = 1.0f - value;
    return std::clamp(value, 0.0f, 1.0f);
}

void PixelScope::setColumn(int index, float minValue, float maxValue) noexcept
{
    const float normMin = normalize(minValue);
    const float normMax = normalize(maxValue);
    dataMin[static_cast<size_t>(index)] = std::round(normMin * rows) / static_cast<float>(rows);
    dataMax[static_cast<size_t>(index)] = std::round(normMax * rows) / static_cast<float>(rows);
}

void PixelScope::resized()
{
    const auto bounds = localBounds();
    colWidth = bounds.width() / static_cast<float>(columns);
    rowHeight = bounds.height() / static_cast<float>(rows);

    const float ratio = bounds.width() / referenceWidth;
    pixelSize = basePixelSize * ratio;
    pixelOffset = pixelSize * 0.5f;
}

void PixelScope::pollFifo()
{
    const int available = source.samplesAvailable();
    if (available <= 0)
    {
        return;
    }

    if (static_cast<int>(scratch.size()) < available)
    {
        scratch.resize(static_cast<size_t>(available));
    }

    const int numRead = source.read(scratch.data(), available);
    if (numRead <= 0)
    {
        return;
    }

    const int samplesPerColumn = std::max(1, numRead / columns);

    int column = 0;
    int countInColumn = 0;
    float columnMin = 0.0f;
    float columnMax = 0.0f;

    for (int i = 0; i < numRead && column < columns; ++i)
    {
        const float s = scratch[static_cast<size_t>(i)];
        columnMax = std::max(columnMax, s);
        columnMin = std::min(columnMin, s);

        if (++countInColumn >= samplesPerColumn)
        {
            setColumn(column, columnMin, columnMax);
            columnMin = 0.0f;
            columnMax = 0.0f;
            countInColumn = 0;
            ++column;
        }
    }

    redraw();
}

void PixelScope::draw(visage::Canvas &canvas)
{
    canvas.setColor(traceColor);
    const float h = height();
    constexpr float rowStep = 1.0f / static_cast<float>(rows);
    for (int i = 0; i < columns; ++i)
    {
        float yPos = dataMax[static_cast<size_t>(i)];
        do
        {
            canvas.rectangle(static_cast<float>(i) * colWidth, yPos * h, pixelSize, pixelSize);
            yPos += rowStep;
        } while (yPos <= dataMin[static_cast<size_t>(i)]);
    }

    canvas.setColor(gridDotColor);
    for (int x = 0; x < columns; ++x)
    {
        for (int y = 0; y < rows; ++y)
        {
            canvas.rectangle(static_cast<float>(x) * colWidth + pixelOffset,
                              static_cast<float>(y) * rowHeight + pixelOffset, pixelSize, pixelSize);
        }
    }
}
