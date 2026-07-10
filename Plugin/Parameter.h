#pragma once

#include <atomic>
#include <clap/clap.h>
#include <string>
#include <vector>

enum class ParamFormat
{
    Toggle,
    Decibels,
    Integer,
    Choice
};

class Parameter
{
public:
    Parameter(clap_id id, std::string name, double min, double max, double defaultValue,
               clap_param_info_flags flags, ParamFormat format,
               std::vector<std::string> choiceLabels = {}, double skew = 1.0);

    clap_id id() const noexcept { return _id; }
    void info(clap_param_info_t *out) const noexcept;

    double value() const noexcept { return _value.load(std::memory_order_relaxed); }
    void setValue(double v) noexcept { _value.store(v, std::memory_order_relaxed); }
    double defaultValue() const noexcept { return _default; }
    double min() const noexcept { return _min; }
    double max() const noexcept { return _max; }
    double skew() const noexcept { return skewFactor; }
    const std::vector<std::string> &choiceLabels() const noexcept { return _choiceLabels; }

    bool valueToText(double value, char *out, uint32_t size) const noexcept;
    bool textToValue(const char *text, double *out) const noexcept;

private:
    clap_id _id;
    std::string _name;
    double _min;
    double _max;
    double _default;
    clap_param_info_flags _flags;
    ParamFormat _format;
    std::vector<std::string> _choiceLabels;
    double skewFactor;

    // Written from the UI thread, read every audio block: must stay lock-free, or process() would
    // start blocking on a mutex.
    std::atomic<double> _value;
    static_assert(std::atomic<double>::is_always_lock_free,
                  "std::atomic<double> is no longer lock-free on this target - Parameter::_value "
                  "would make process() block on a mutex");
};
