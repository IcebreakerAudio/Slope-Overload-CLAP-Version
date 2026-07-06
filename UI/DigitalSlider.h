#pragma once

#include "ParamSource.h"

#include <visage_graphics/font.h>
#include <visage_ui/frame.h>

class DigitalSlider : public visage::Frame
{
public:
    DigitalSlider(ParamSource &paramSource, visage::Font textFont, bool useDigitalReadout);

    void draw(visage::Canvas &canvas) override;
    void mouseDown(const visage::MouseEvent &e) override;
    void mouseDrag(const visage::MouseEvent &e) override;
    void mouseUp(const visage::MouseEvent &e) override;

    void setShadowOffset(float offset) noexcept { shadowOffset = offset; }
    void setFontSize(float size) noexcept { font = font.withSize(size); redraw(); }
    void refreshFromParam() { redraw(); }

private:
    double valueToNormalized(double value) const noexcept;
    double normalizedToValue(double normalized) const noexcept;

    ParamSource &param;
    visage::Font font;
    bool digitalReadout;

    float shadowOffset = 2.0f;
    float dragStartY = 0.0f;
    double dragStartNormalized = 0.0;
    bool dragging = false;

    static constexpr float kDragRangePixels = 200.0f;
};
