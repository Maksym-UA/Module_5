#pragma once

class SignalFilter
{
public:
    explicit SignalFilter(float alpha);

    float update(float sample);
    bool has_value() const;

private:
    float alpha_ = 0.0F;
    bool initialized_ = false;
    float value_ = 0.0F;
};