#pragma once

#include "ParamSource.h"
#include "Parameter.h"

#include <atomic>
#include <functional>

struct PendingParamChange
{
    std::atomic<bool> beginPending{false};
    std::atomic<bool> endPending{false};
    std::atomic<bool> valueDirty{false};
    std::atomic<double> pendingValue{0.0};
};

class ParamAttachment : public ParamSource
{
public:
    ParamAttachment(Parameter &param, PendingParamChange &pending, std::function<void()> hostFlushFn);

    double value() const noexcept override;
    double defaultValue() const noexcept override;
    double minValue() const noexcept override;
    double maxValue() const noexcept override;
    double skew() const noexcept override;
    bool valueToText(double v, std::string &out) const override;

    void beginGesture() noexcept override;
    void setValue(double newValue) noexcept override;
    void endGesture() noexcept override;

private:
    Parameter &boundParam;
    PendingParamChange &pendingChange;
    std::function<void()> requestHostFlush;
};
