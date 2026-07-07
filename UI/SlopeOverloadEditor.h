#pragma once

#include "DigitalSlider.h"
#include "GroupedToggle.h"
#include "PixelScope.h"
#include "ParamSource.h"
#include "PowerToggleButton.h"
#include "ScopeSource.h"
#include "ShadowLabel.h"

#include <visage/app.h>
#include <visage/ui.h>

class SlopeOverloadEditor : public visage::ApplicationWindow, public visage::EventTimer
{
public:
    SlopeOverloadEditor(ParamSource &activeSource, ParamSource &inGainSource, ParamSource &outGainSource,
                        ParamSource &sRateSource, ParamSource &aaFiltSource, ParamSource &speakerSource,
                        ScopeSource &scopeSource);

    void resized() override;
    void timerCallback() override;

private:
    static constexpr float kOriginalWidth = 715.0f;
    static constexpr float kOriginalHeight = 460.0f;

    ParamSource &active;

    visage::SvgFrame background;
    PixelScope scope;

    DigitalSlider inGainSlider;
    DigitalSlider outGainSlider;
    DigitalSlider sRateSlider;
    GroupedToggle speakerToggle;
    GroupedToggle aaFiltToggle;
    PowerToggleButton powerButton;

    ShadowLabel inLabel;
    ShadowLabel outLabel;
    ShadowLabel filterLabel;
    ShadowLabel sRateLabel;
    ShadowLabel speakerLabel;
};
