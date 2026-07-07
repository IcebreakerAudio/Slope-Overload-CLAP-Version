#include "SlopeOverloadEditor.h"

#include "embedded/SlopeOverloadAssets.h"

using namespace visage::dimension;

SlopeOverloadEditor::SlopeOverloadEditor(ParamSource &activeSource, ParamSource &inGainSource,
                                         ParamSource &outGainSource, ParamSource &sRateSource,
                                         ParamSource &aaFiltSource, ParamSource &speakerSource,
                                         ScopeSource &scopeSource)
    : active(activeSource),
      background(assets::Background_svg),
      scope(scopeSource),
      inGainSlider(inGainSource, visage::Font(28.0f, assets::VT323_Regular_ttf), false),
      outGainSlider(outGainSource, visage::Font(28.0f, assets::VT323_Regular_ttf), false),
      sRateSlider(sRateSource, visage::Font(32.0f, assets::DigitalNumbers_Regular_ttf), true),
      speakerToggle(speakerSource, std::vector<std::string>{"A", "B", "C"},
                    visage::Font(28.0f, assets::VT323_Regular_ttf)),
      aaFiltToggle(aaFiltSource, std::vector<std::string>{"OFF", "ON"},
                   visage::Font(28.0f, assets::VT323_Regular_ttf), "/"),
      powerButton(assets::PowerButton_Off_svg, assets::PowerButton_On_svg),
      inLabel("IN:", visage::Font(28.0f, assets::VT323_Regular_ttf), visage::Font::kLeft),
      outLabel("OUT:", visage::Font(28.0f, assets::VT323_Regular_ttf), visage::Font::kLeft),
      filterLabel("FILTER", visage::Font(28.0f, assets::VT323_Regular_ttf), visage::Font::kCenter),
      sRateLabel("S.RATE", visage::Font(28.0f, assets::VT323_Regular_ttf), visage::Font::kCenter),
      speakerLabel("SPEAKER", visage::Font(28.0f, assets::VT323_Regular_ttf), visage::Font::kCenter)
{
    addChild(&background);
    addChild(&scope);
    addChild(&inGainSlider);
    addChild(&outGainSlider);
    addChild(&sRateSlider);
    addChild(&speakerToggle);
    addChild(&aaFiltToggle);
    addChild(&powerButton);
    addChild(&inLabel);
    addChild(&outLabel);
    addChild(&filterLabel);
    addChild(&sRateLabel);
    addChild(&speakerLabel);

    powerButton.setToggled(active.value() >= 0.5);
    powerButton.onToggle() += [this](visage::Button *, bool on)
    {
        active.beginGesture();
        active.setValue(on ? 1.0 : 0.0);
        active.endGesture();
    };

    setMinimumDimensions(357.5f, 230.0f);
    setWindowDimensions(715_px, 460_px);
    setFixedAspectRatio(true);

    startTimer(30);
}

void SlopeOverloadEditor::resized()
{
    background.setBounds(localBounds());

    const float ratio = width() / kOriginalWidth;
    const auto scaled = [ratio](float x, float y, float w, float h)
    {
        return visage::Bounds(x * ratio, y * ratio, w * ratio, h * ratio);
    };

    scope.setBounds(scaled(140.0f, 131.0f, 480.0f, 155.0f));

    inGainSlider.setBounds(scaled(190.0f, 82.0f, 90.0f, 34.0f));
    outGainSlider.setBounds(scaled(524.0f, 82.0f, 90.0f, 34.0f));
    sRateSlider.setBounds(scaled(351.0f, 331.0f, 53.0f, 48.0f));
    aaFiltToggle.setBounds(scaled(137.0f, 339.0f, 152.0f, 34.0f));
    speakerToggle.setBounds(scaled(475.0f, 335.0f, 155.0f, 38.0f));
    powerButton.setBounds(scaled(36.0f, 180.0f, 35.0f, 35.0f));

    inLabel.setBounds(scaled(144.0f, 82.0f, 139.0f, 34.0f));
    outLabel.setBounds(scaled(478.0f, 82.0f, 139.0f, 34.0f));
    filterLabel.setBounds(scaled(144.0f, 295.0f, 139.0f, 34.0f));
    sRateLabel.setBounds(scaled(311.0f, 295.0f, 139.0f, 34.0f));
    speakerLabel.setBounds(scaled(484.0f, 295.0f, 139.0f, 34.0f));

    const float shadowDistance = 2.0f * ratio;

    inGainSlider.setShadowOffset(shadowDistance);
    inGainSlider.setFontSize(28.0f * ratio);
    outGainSlider.setShadowOffset(shadowDistance);
    outGainSlider.setFontSize(28.0f * ratio);
    sRateSlider.setShadowOffset(shadowDistance);
    sRateSlider.setFontSize(32.0f * ratio);

    aaFiltToggle.setShadowOffset(shadowDistance);
    speakerToggle.setShadowOffset(shadowDistance);

    inLabel.setShadowOffset(shadowDistance);
    inLabel.setFontSize(28.0f * ratio);
    outLabel.setShadowOffset(shadowDistance);
    outLabel.setFontSize(28.0f * ratio);
    filterLabel.setShadowOffset(shadowDistance);
    filterLabel.setFontSize(28.0f * ratio);
    sRateLabel.setShadowOffset(shadowDistance);
    sRateLabel.setFontSize(28.0f * ratio);
    speakerLabel.setShadowOffset(shadowDistance);
    speakerLabel.setFontSize(28.0f * ratio);
}

void SlopeOverloadEditor::timerCallback()
{
    inGainSlider.refreshFromParam();
    outGainSlider.refreshFromParam();
    sRateSlider.refreshFromParam();
    speakerToggle.refreshFromParam();
    aaFiltToggle.refreshFromParam();
    powerButton.setToggled(active.value() >= 0.5);
}
