#include "ShadowToggleButton.h"

#include <visage_graphics/canvas.h>

ShadowToggleButton::ShadowToggleButton(const std::string &label, visage::Font textFont)
    : ToggleButton(label), text(label), font(std::move(textFont))
{
}

void ShadowToggleButton::resized()
{
    font = font.withSize(height() * 1.25f);
}

void ShadowToggleButton::draw(visage::Canvas &canvas, float)
{
    constexpr auto justification = visage::Font::kCenter;
    const auto w = width();
    const auto h = height();

    canvas.setColor(0x20000000u);
    canvas.text(text, font, justification, shadowOffset, shadowOffset, w, h);

    if (toggled())
    {
        canvas.setColor(0xff2a2e0du);
        canvas.text(text, font, justification, 0, 0, w, h);
    }
}
