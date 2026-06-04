#include "control/signal_filter.h"

SignalFilter::SignalFilter(float alpha)
    : alpha_(alpha)
{
}

float SignalFilter::update(float sample)
{
    if (!initialized_) {
        value_ = sample;
        initialized_ = true;
        return value_;
    }

    value_ = value_ + alpha_ * (sample - value_);
    return value_;
}

bool SignalFilter::has_value() const
{
    return initialized_;
}
