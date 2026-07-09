#include "Parameter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

Parameter::Parameter(clap_id id, std::string name, double min, double max, double defaultValue,
                       clap_param_info_flags flags, ParamFormat format,
                       std::vector<std::string> choiceLabels, double skew)
    : _id(id),
      _name(std::move(name)),
      _min(min),
      _max(max),
      _default(defaultValue),
      _flags(flags),
      _format(format),
      _choiceLabels(std::move(choiceLabels)),
      skewFactor(skew),
      _value(defaultValue)
{
}

void Parameter::info(clap_param_info_t *out) const noexcept
{
    out->id = _id;
    out->flags = _flags;
    out->cookie = nullptr;
    std::snprintf(out->name, sizeof(out->name), "%s", _name.c_str());
    out->module[0] = '\0';
    out->min_value = _min;
    out->max_value = _max;
    out->default_value = _default;
}

bool Parameter::valueToText(double value, char *out, uint32_t size) const noexcept
{
    if (out == nullptr || size == 0)
    {
        return false;
    }

    switch (_format)
    {
    case ParamFormat::Toggle:
        std::snprintf(out, size, "%s", value >= 0.5 ? "On" : "Off");
        return true;

    case ParamFormat::Decibels:
        std::snprintf(out, size, "%+.1fdB", value);
        return true;

    case ParamFormat::Integer:
        std::snprintf(out, size, "%d", static_cast<int>(std::lround(value)));
        return true;

    case ParamFormat::Choice:
    {
        auto index = static_cast<size_t>(std::lround(value));
        if (index >= _choiceLabels.size())
        {
            return false;
        }
        std::snprintf(out, size, "%s", _choiceLabels[index].c_str());
        return true;
    }
    }

    return false;
}

bool Parameter::textToValue(const char *text, double *out) const noexcept
{
    if (text == nullptr || out == nullptr)
    {
        return false;
    }

    if (_format == ParamFormat::Choice)
    {
        for (size_t i = 0; i < _choiceLabels.size(); ++i)
        {
            if (_choiceLabels[i] == text)
            {
                *out = static_cast<double>(i);
                return true;
            }
        }
    }

    if (_format == ParamFormat::Toggle)
    {
        if (std::strcmp(text, "On") == 0)
        {
            *out = _max;
            return true;
        }
        if (std::strcmp(text, "Off") == 0)
        {
            *out = _min;
            return true;
        }
    }

    char *end = nullptr;
    double parsed = std::strtod(text, &end);
    if (end == text)
    {
        return false;
    }

    *out = std::clamp(parsed, _min, _max);
    return true;
}
