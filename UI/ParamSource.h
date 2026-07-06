#pragma once

#include <string>

class ParamSource
{
public:
    virtual ~ParamSource() = default;

    virtual double value() const noexcept = 0;
    virtual double defaultValue() const noexcept = 0;
    virtual double minValue() const noexcept = 0;
    virtual double maxValue() const noexcept = 0;
    virtual double skew() const noexcept = 0;
    virtual bool valueToText(double value, std::string &out) const = 0;

    virtual void beginGesture() noexcept = 0;
    virtual void setValue(double newValue) noexcept = 0;
    virtual void endGesture() noexcept = 0;
};
