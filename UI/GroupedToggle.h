#pragma once

#include "ParamSource.h"
#include "ShadowToggleButton.h"

#include <visage_graphics/font.h>
#include <visage_ui/frame.h>

#include <string>
#include <vector>

class GroupedToggle : public visage::Frame
{
public:
    GroupedToggle(ParamSource &paramSource, std::vector<std::string> labels, visage::Font textFont,
                  std::string dividerText = "");

    void draw(visage::Canvas &canvas) override;
    void resized() override;
    void refreshFromParam();
    void setShadowOffset(float offset) noexcept;

private:
    void selectIndex(size_t index);

    ParamSource &param;
    std::vector<ShadowToggleButton *> buttons;
    std::string dividerLabel;
    visage::Font dividerFont;
    std::vector<visage::Bounds> dividerRects;
};
