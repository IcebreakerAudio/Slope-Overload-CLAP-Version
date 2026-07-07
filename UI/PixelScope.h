#pragma once

#include "ScopeSource.h"

#include <visage_graphics/canvas.h>
#include <visage_ui/frame.h>

#include <array>
#include <vector>

class PixelScope : public visage::Frame, public visage::EventTimer
{
public:
    explicit PixelScope(ScopeSource &scopeSource);

    void draw(visage::Canvas &canvas) override;
    void resized() override;
    void timerCallback() override;

private:
    static constexpr int columns = 60;
    static constexpr int rows = 18;
    static constexpr float basePixelSize = 4.0f;
    static constexpr float referenceWidth = 480.0f;
    static constexpr int pollIntervalMs = 83;
    static constexpr unsigned int traceColor = 0xff2a2e0du;
    static constexpr unsigned int gridDotColor = 0x20000000u;

    static float normalize(float value) noexcept;
    void setColumn(int index, float minValue, float maxValue) noexcept;
    void pollFifo();

    ScopeSource &source;

    std::vector<float> scratch;
    std::array<float, columns> dataMin;
    std::array<float, columns> dataMax;

    float colWidth = 0.0f;
    float rowHeight = 0.0f;
    float pixelSize = basePixelSize;
    float pixelOffset = basePixelSize * 0.5f;
};
