#include "ParamAttachment.h"

ParamAttachment::ParamAttachment(Parameter &param, PendingParamChange &pending, std::function<void()> hostFlushFn)
    : boundParam(param), pendingChange(pending), requestHostFlush(std::move(hostFlushFn))
{
}

double ParamAttachment::value() const noexcept
{
    return boundParam.value();
}

double ParamAttachment::defaultValue() const noexcept
{
    return boundParam.defaultValue();
}

double ParamAttachment::minValue() const noexcept
{
    return boundParam.min();
}

double ParamAttachment::maxValue() const noexcept
{
    return boundParam.max();
}

double ParamAttachment::skew() const noexcept
{
    return boundParam.skew();
}

bool ParamAttachment::valueToText(double v, std::string &out) const
{
    char buffer[64];
    if (!boundParam.valueToText(v, buffer, sizeof(buffer)))
    {
        return false;
    }
    out = buffer;
    return true;
}

void ParamAttachment::beginGesture() noexcept
{
    pendingChange.beginPending.store(true, std::memory_order_release);
    if (requestHostFlush)
    {
        requestHostFlush();
    }
}

void ParamAttachment::setValue(double newValue) noexcept
{
    boundParam.setValue(newValue);
    pendingChange.pendingValue.store(newValue, std::memory_order_relaxed);
    pendingChange.valueDirty.store(true, std::memory_order_release);
    if (requestHostFlush)
    {
        requestHostFlush();
    }
}

void ParamAttachment::endGesture() noexcept
{
    pendingChange.endPending.store(true, std::memory_order_release);
    if (requestHostFlush)
    {
        requestHostFlush();
    }
}
