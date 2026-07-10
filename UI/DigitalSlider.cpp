#include "DigitalSlider.h"

#include <visage_graphics/canvas.h>

#include <algorithm>
#include <cmath>

DigitalSlider::DigitalSlider(ParamSource &paramSource, visage::Font textFont, bool useDigitalReadout)
    : param(paramSource), font(std::move(textFont)), digitalReadout(useDigitalReadout)
{
}

double DigitalSlider::valueToNormalized(double value) const noexcept
{
    const auto lo = param.minValue();
    const auto hi = param.maxValue();
    if (hi <= lo)
    {
        return 0.0;
    }

    const auto t = std::clamp((value - lo) / (hi - lo), 0.0, 1.0);
    const auto skewAmount = param.skew();
    return skewAmount == 1.0 ? t : std::pow(t, 1.0 / skewAmount);
}

double DigitalSlider::normalizedToValue(double normalized) const noexcept
{
    const auto lo = param.minValue();
    const auto hi = param.maxValue();
    const auto clamped = std::clamp(normalized, 0.0, 1.0);
    const auto skewAmount = param.skew();
    const auto t = skewAmount == 1.0 ? clamped : std::pow(clamped, skewAmount);
    return lo + (hi - lo) * t;
}

void DigitalSlider::draw(visage::Canvas &canvas)
{
    std::string text;
    param.valueToText(param.value(), text);

    constexpr auto justification = visage::Font::kRight;
    const auto w = width();
    const auto h = height();

    canvas.setColor(0x20000000u);
    if (digitalReadout)
    {
        canvas.text("88", font, justification, shadowOffset, shadowOffset, w, h);
    }
    else
    {
        canvas.text(text, font, justification, shadowOffset, shadowOffset, w, h);
    }

    canvas.setColor(0xff2a2e0du);
    canvas.text(text, font, justification, 0, 0, w, h);
}

void DigitalSlider::mouseDown(const visage::MouseEvent &e)
{
    if (e.isAltDown() || e.repeatClickCount() >= 2)
    {
        param.beginGesture();
        param.setValue(param.defaultValue());
        param.endGesture();
        redraw();
        return;
    }

    dragging = true;
    dragStartY = e.position.y;
    dragStartNormalized = valueToNormalized(param.value());
    param.beginGesture();
}

void DigitalSlider::mouseDrag(const visage::MouseEvent &e)
{
    if (!dragging)
    {
        return;
    }

    const float pixelDelta = dragStartY - e.position.y;
    const double normalized = dragStartNormalized + static_cast<double>(pixelDelta) / kDragRangePixels;
    param.setValue(normalizedToValue(normalized));
    redraw();
}

void DigitalSlider::mouseUp(const visage::MouseEvent &)
{
    if (!dragging)
    {
        return;
    }

    dragging = false;
    param.endGesture();
}
