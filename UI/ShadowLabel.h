#pragma once

#include <visage_graphics/font.h>
#include <visage_ui/frame.h>

#include <string>

class ShadowLabel : public visage::Frame
{
public:
    ShadowLabel(std::string label, visage::Font textFont, visage::Font::Justification justification);

    void draw(visage::Canvas &canvas) override;

    void setFontSize(float size) noexcept { font = font.withSize(size); redraw(); }
    void setShadowOffset(float offset) noexcept { shadowOffset = offset; }

private:
    std::string text;
    visage::Font font;
    visage::Font::Justification justification;
    float shadowOffset = 2.0f;
};
