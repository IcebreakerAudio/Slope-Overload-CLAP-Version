#include "GroupedToggle.h"

#include <visage_graphics/canvas.h>

#include <cmath>
#include <memory>

GroupedToggle::GroupedToggle(ParamSource &paramSource, std::vector<std::string> labels, visage::Font textFont,
                              std::string dividerText)
    : param(paramSource), dividerLabel(std::move(dividerText)), dividerFont(textFont)
{
    for (auto &label : labels)
    {
        auto button = std::make_unique<ShadowToggleButton>(label, textFont);
        auto *buttonPtr = button.get();
        const size_t index = buttons.size();
        buttonPtr->onToggle() += [this, buttonPtr, index](visage::Button *, bool on)
        {
            if (on)
            {
                selectIndex(index);
            }
            else
            {
                buttonPtr->setToggled(true);
            }
        };

        buttons.push_back(buttonPtr);
        addChild(std::move(button));
    }

    refreshFromParam();
}

void GroupedToggle::selectIndex(size_t index)
{
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->setToggled(i == index);
    }

    param.beginGesture();
    param.setValue(static_cast<double>(index));
    param.endGesture();
}

void GroupedToggle::refreshFromParam()
{
    const auto index = static_cast<size_t>(std::lround(param.value()));
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->setToggled(i == index);
    }
}

void GroupedToggle::setShadowOffset(float offset) noexcept
{
    for (auto *button : buttons)
    {
        button->setShadowOffset(offset);
    }
}

void GroupedToggle::resized()
{
    if (buttons.empty())
    {
        return;
    }

    const auto bounds = localBounds();
    dividerFont = dividerFont.withSize(bounds.height() * 1.1f);
    dividerRects.clear();

    const size_t numDividers = dividerLabel.empty() ? 0 : buttons.size() - 1;
    const float dividerWidth = numDividers > 0 ? bounds.height() * 0.5f : 0.0f;
    const float buttonWidth = (bounds.width() - dividerWidth * static_cast<float>(numDividers)) /
                              static_cast<float>(buttons.size());

    float x = bounds.x();
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->setBounds(x, bounds.y(), buttonWidth, bounds.height());
        x += buttonWidth;

        if (numDividers > 0 && i < buttons.size() - 1)
        {
            dividerRects.emplace_back(x, bounds.y(), dividerWidth, bounds.height());
            x += dividerWidth;
        }
    }
}

void GroupedToggle::draw(visage::Canvas &canvas)
{
    if (dividerLabel.empty())
    {
        return;
    }

    constexpr auto justification = visage::Font::kCenter;
    canvas.setColor(0x20000000u);
    for (const auto &rect : dividerRects)
    {
        canvas.text(dividerLabel, dividerFont, justification, rect.x(), rect.y(), rect.width(), rect.height());
    }
}
