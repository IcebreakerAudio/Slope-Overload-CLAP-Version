#pragma once

#include <visage_file_embed/embedded_file.h>
#include <visage_widgets/button.h>

class PowerToggleButton : public visage::ToggleIconButton
{
public:
    PowerToggleButton(const visage::EmbeddedFile &offIcon, const visage::EmbeddedFile &onIcon);

    void toggleValueChanged() override;

    // The base class tints the icon by theme color on every hover-animation frame. Our icons
    // (PowerButton_On/Off.svg) already carry their own baked-in colors/gradients, so skip that.
    void draw(visage::Canvas &, float) override { }

private:
    visage::EmbeddedFile offSvgFile;
    visage::EmbeddedFile onSvgFile;
};
