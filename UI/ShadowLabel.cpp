#include "ShadowLabel.h"

#include <visage_graphics/canvas.h>

ShadowLabel::ShadowLabel(std::string label, visage::Font textFont, visage::Font::Justification justification)
    : text(std::move(label)), font(std::move(textFont)), justification(justification)
{
    setIgnoresMouseEvents(true, false);
}

void ShadowLabel::draw(visage::Canvas &canvas)
{
    const auto w = width();
    const auto h = height();

    canvas.setColor(0x20000000u);
    canvas.text(text, font, justification, shadowOffset, shadowOffset, w, h);

    canvas.setColor(0xff2a2e0du);
    canvas.text(text, font, justification, 0, 0, w, h);
}
