#pragma once

class PidController
{
public:
    PidController(float kp, float ki, float kd, float out_min, float out_max, float deadband);

    float compute(float setpoint, float input, float dt_seconds);

private:
    static float clamp(float value, float min_value, float max_value);

    float kp_ = 0.0F; // Proportional gain
    float ki_ = 0.0F; // Integral gain
    float kd_ = 0.0F; // Derivative gain
    float out_min_ = 0.0F;
    float out_max_ = 0.0F;
    float deadband_ = 0.0F;
    float integral_ = 0.0F;
    float previous_error_ = 0.0F;
};
