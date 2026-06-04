#include "control/pid_controller.h"

PidController::PidController(float kp, float ki, float kd, float out_min, float out_max, float deadband)
    : kp_(kp),
      ki_(ki),
      kd_(kd),
      out_min_(out_min),
      out_max_(out_max),
      deadband_(deadband)
{
}

float PidController::clamp(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }

    if (value > max_value) {
        return max_value;
    }

    return value;
}

// Computes the PID output based on the setpoint, current input, and time delta.
float PidController::compute(float setpoint, float input, float dt_seconds)
{
    if (dt_seconds <= 0.0F) {
        dt_seconds = 0.001F;
    }

    const float error = setpoint - input;
    const float abs_error = (error < 0.0F) ? -error : error;

    if (abs_error < deadband_) {
        previous_error_ = error;
        return clamp(kp_ * error, out_min_, out_max_);
    }

    const float candidate_integral = integral_ + (error * dt_seconds);
    const float derivative = (error - previous_error_) / dt_seconds;

    const float unconstrained_output =
        (kp_ * error) +
        (ki_ * candidate_integral) +
        (kd_ * derivative);

    const float constrained_output = clamp(unconstrained_output, out_min_, out_max_);

    if (constrained_output == unconstrained_output) {
        integral_ = candidate_integral;
    } else {
        const bool saturated_high = constrained_output >= out_max_;
        const bool saturated_low = constrained_output <= out_min_;
        const bool pulls_toward_center =
            (saturated_high && error < 0.0F) ||
            (saturated_low && error > 0.0F);

        if (pulls_toward_center) {
            integral_ = candidate_integral;
        }
    }

    previous_error_ = error;
    return constrained_output;
}
