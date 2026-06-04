#pragma once

class SignalFilter
{
public:
    explicit SignalFilter(float alpha);

    float update(float sample);
    bool has_value() const;

private:
    float alpha_ = 0.0F; // Smoothing factor for the filter (0 < alpha <= 1).
    bool initialized_ = false; // Indicates if the filter has been initialized with a value.
    float value_ = 0.0F; // Current filtered value.
};
