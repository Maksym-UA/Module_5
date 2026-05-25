# Module 5
/*
 * Button Debounce with RC Filter — ESP32-S3, ESP-IDF
 *
 * Hardware RC filter (low-pass) on the button pin:
 *
 *   3.3V ──┬── R(10kΩ) ──┬── GPIO (input, pull-down OFF)
 *          │              │
 *         BTN            C(100nF)
 *          │              │
 *         GND ───────────GND
 *
 *  τ = R × C = 10kΩ × 100nF = 1 ms  →  filters bounce spikes < 1 ms
 *  The GPIO sees a clean rising/falling edge already shaped by the RC.
 *  A small software guard time (5 ms) catches any residual glitches.
 *
 * GPIO assignment
 *   BUTTON_GPIO  — pin connected after the RC network
 *   LED_GPIO     — indicator toggled on each confirmed press
 */