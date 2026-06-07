#include "application.h"
#include "app_config.h"

using namespace AppConfig;

static ESP32Encoder g_encoder;

// Clamp helper for normalization and safety bounds.
static double clamp_value(double value, double min_v, double max_v)
{
    if (value < min_v) {
        return min_v;
    }
    if (value > max_v) {
        return max_v;
    }
    return value;
}

static double map_adc_to_angle_deg(int adc_raw)
{
    const double normalized = clamp_value(adc_raw, ADC_MIN, ADC_MAX) / ADC_MAX;
    return ANGLE_MIN_DEG + normalized * (ANGLE_MAX_DEG - ANGLE_MIN_DEG);
}

static double read_encoder_angle_deg()
{
    const int64_t pulse_count = g_encoder.getCount();
    const double angle = (pulse_count * 360.0) / ENCODER_CPR_X4;
    return clamp_value(angle, ANGLE_MIN_DEG, ANGLE_MAX_DEG);
}

static void set_motor_pwm(double duty_0_to_1)
{
    const double clamped_duty = clamp_value(duty_0_to_1, 0.0, 1.0);
    const uint32_t duty = static_cast<uint32_t>(clamped_duty * PWM_MAX_DUTY);
    ledcWrite(PWM_CHANNEL, duty);
}

static void control_setup()
{
    Serial.begin(115200);
    delay(200);

    analogReadResolution(12);
    analogSetPinAttenuation(POT_ADC_PIN, ADC_11db);

    if (!ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS)) {
        Serial.println("LEDC setup failed");
        while (true) {
            delay(1000);
        }
    }
    ledcAttachPin(PWM_OUT_PIN, PWM_CHANNEL);

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    g_encoder.attachFullQuad(ENCODER_A_PIN, ENCODER_B_PIN);
    g_encoder.clearCount();

    if (ENCODER_CPR_X4 <= 0) {
        Serial.println("Invalid ENCODER_CPR_X4 constant");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("PID position control started");
    Serial.println("Setpoint holds the maximum observed ADC value.");
    Serial.println("Reset (clear angle/setpoint) only when pot is near minimum.");
}

static void control_loop()
{
    static bool initialized = false;
    static bool regulation_enabled = true;
    static double pid_input = 0.0;
    static double pid_output = 0.0;
    static double pid_setpoint = 0.0;
    static PID pid(&pid_input, &pid_output, &pid_setpoint, PID_KP, PID_KI, PID_KD, DIRECT);
    static int hold_adc = 0;
    static uint32_t last_control_ms = 0;
    static uint32_t last_print_ms = 0;

    if (!initialized) {
        hold_adc = analogRead(POT_ADC_PIN);
        last_control_ms = millis();
        last_print_ms = last_control_ms;
        pid.SetSampleTime(CONTROL_PERIOD_MS);
        pid.SetOutputLimits(0.0, 1.0);
        pid.SetMode(AUTOMATIC);
        initialized = true;
    }

    const uint32_t now_ms = millis();
    if ((now_ms - last_control_ms) < CONTROL_PERIOD_MS) {
        return;
    }
    last_control_ms = now_ms;

    const int adc_raw = analogRead(POT_ADC_PIN);
    const double measured_angle_deg = read_encoder_angle_deg();

    if (!regulation_enabled) {
        set_motor_pwm(0.0);

        if (adc_raw <= ADC_REARM_THRESHOLD) {
            g_encoder.clearCount();
            pid_input = 0.0;
            pid_setpoint = 0.0;
            hold_adc = adc_raw;
            pid.SetMode(MANUAL);
            pid_output = 0.0;
            pid.SetMode(AUTOMATIC);
            regulation_enabled = true;

            if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
                last_print_ms = now_ms;
                Serial.printf("REARM: ADC=%d (<= %d), regulator enabled\n", adc_raw, ADC_REARM_THRESHOLD);
            }
        }
        return;
    }

    if (measured_angle_deg >= ANGLE_MAX_DEG || adc_raw > ADC_MAX - ADC_REARM_THRESHOLD) {
        set_motor_pwm(0.0);
        pid.SetMode(MANUAL);
        pid_output = 0.0;
        regulation_enabled = false;

        if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
            last_print_ms = now_ms;
            Serial.printf("LATCH OFF: ADC=%d PV=%.1fdeg\n", adc_raw, measured_angle_deg);
        }
        return;
    }

    if (adc_raw > hold_adc) {
        hold_adc = adc_raw;
    }

    const double setpoint_deg = map_adc_to_angle_deg(hold_adc);
    const double error_deg = setpoint_deg - measured_angle_deg;

    pid_setpoint = setpoint_deg;
    pid_input = measured_angle_deg;

    pid.Compute();
    set_motor_pwm(pid_output);

    if (now_ms - last_print_ms >= STATUS_PRINT_MS) {
        last_print_ms = now_ms;
        Serial.printf("ADC=%d HOLD=%d SP=%.1fdeg PV=%.1fdeg ERR=%.1fdeg PWM=%.2f\n",
                      adc_raw,
                      hold_adc,
                      setpoint_deg,
                      measured_angle_deg,
                      error_deg,
                      pid_output);
    }
}

void application_init()
{
    Application app;
    app.start();
}

void Application::start()
{
    run();
}

void Application::run()
{
    control_setup();
    Serial.println("Application starting...");

    while (true) {
        control_loop();
        delay(1);
    }
}
