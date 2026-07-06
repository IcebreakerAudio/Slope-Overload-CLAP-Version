#include "PowerToggleButton.h"

PowerToggleButton::PowerToggleButton(const visage::EmbeddedFile &offIcon, const visage::EmbeddedFile &onIcon)
    : ToggleIconButton(offIcon), offSvgFile(offIcon), onSvgFile(onIcon)
{
}

void PowerToggleButton::toggleValueChanged()
{
    setIcon(toggled() ? onSvgFile : offSvgFile);
}
