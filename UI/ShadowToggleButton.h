#pragma once

#include <visage_graphics/font.h>
#include <visage_widgets/button.h>

#include <string>

class ShadowToggleButton : public visage::ToggleButton
{
public:
    ShadowToggleButton(const std::string &label, visage::Font textFont);

    void draw(visage::Canvas &canvas, float hoverAmount) override;
    void resized() override;

    void setShadowOffset(float offset) noexcept { shadowOffset = offset; }

private:
    std::string text;
    visage::Font font;
    float shadowOffset = 2.0f;
};
