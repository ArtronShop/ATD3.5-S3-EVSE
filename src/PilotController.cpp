#include <Arduino.h>
#include "PilotController.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "soc/gpio_struct.h"
#include "soc/sens_struct.h"
#include "esp_log.h"

static const char * TAG = "PilotController";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t adc_cali_handle = NULL;
static hw_timer_t * cp_pwm_timer_handle = NULL;

static volatile int cp_mV_in_pwm_high_last = 0;
static volatile bool cp_mV_in_pwm_high_last_update_flag = false;
static volatile int cp_mV_in_pwm_low_last = 0;
static volatile bool cp_mV_in_pwm_low_last_update_flag = false;

static void PilotManagerTask(void *) ;

extern bool mainPowerIsReady() ;
extern bool onPowerOnReq() ;
extern void onPowerOffReq() ;
extern void onStateChange(PilotState_t from, PilotState_t to) ;

PilotController::PilotController() {
    // Constructor does not need to initialize anything
}

int IRAM_ATTR getCPVoltage() {
    int raw = 0;
    int voltage = 0;

    adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &raw);
    adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage);

    int cp_voltage = map(voltage, 230, 2382, -11800, 11000); // Convert to CP voltage based on the voltage divider ratio

    // ESP_LOGI(TAG, "Raw: %d\tVoltage: %d mV\tCP Voltage: %d mV", raw, voltage, cp_voltage);

    // Read the voltage from the CP pin
    return cp_voltage; // Assuming cp_rx_pin is connected to an analog input
}

void startPWMTimer(uint64_t alarm_value) {
    timerStop(cp_pwm_timer_handle);
    timerRestart(cp_pwm_timer_handle);
    timerAlarm(cp_pwm_timer_handle, alarm_value - 13, false, 0); // 13 is time of timer ready to start
    timerStart(cp_pwm_timer_handle);
}

void IRAM_ATTR cpPWMTimerISR(void * args) {
        PilotController * controller = (PilotController *) args;

        if (controller->pwm_state == 0) {
            startPWMTimer(100);

            controller->setCPLogic(HIGH);
            controller->pwm_state = 1;

            delayMicroseconds(10);

            int v = getCPVoltage();
            if ((v >= -13000) && (v <= 13000)) {
                cp_mV_in_pwm_high_last = v;
                cp_mV_in_pwm_high_last_update_flag = true;
            }
        } else if (controller->pwm_state == 1) {
            startPWMTimer(900);

            controller->setCPLogic(LOW);
            controller->pwm_state = 0;

            // delayMicroseconds(10);

            cp_mV_in_pwm_low_last = getCPVoltage();
            cp_mV_in_pwm_low_last_update_flag = true;
        }
    }

void PilotManagerTask(void * args) {
    PilotController * controller = (PilotController *) args;

    while (true) {
        controller->run(); // Call the run method in the task
    }

    vTaskDelete(NULL); // Delete the task when done (though it won't reach here)
}

bool PilotController::begin(int cp_tx_pin, int cp_rx_pin, int max_current) {
    this->cp_tx_pin = cp_tx_pin;
    this->cp_rx_pin = cp_rx_pin;
    this->max_current = max_current;

    // Set up the pins
    pinMode(cp_tx_pin, OUTPUT);
    last_cp_logic = 0;
    pinMode(cp_rx_pin, INPUT);

    
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &config);

    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);

    xTaskCreate(PilotManagerTask, "PilotManagerTask", 16 * 1024, this, 19, NULL);

    pilot_controller_event_group = xEventGroupCreate();

    cp_pwm_timer_handle = timerBegin(1E6);
    timerAttachInterruptArg(cp_pwm_timer_handle, cpPWMTimerISR, this);

    return true; // Indicate successful initialization
}

void PilotController::setCPLogic(uint8_t logic) {
    // digitalWrite(cp_tx_pin, logic); // Set the CP TX pin to the specified logic level
    if (logic) {
        GPIO.out1_w1ts.val = (1U << (42 - 32));
    } else {
        GPIO.out1_w1tc.val = (1U << (42 - 32));
    }
    last_cp_logic = logic;
}

int PilotController::getCPLogic() {
    return last_cp_logic;
}

void PilotController::run() {
    if (state == STATE_INITIAL) {
        timerStop(cp_pwm_timer_handle);

        // Set ready state for the TX pin
        setCPLogic(HIGH); // Set TX pin to HIGH for output +12V at CP pin
        state = STATE_A; // Set state to state A (EVSE Ready, Vehicle Not connected)
        onStateChange(STATE_INITIAL, STATE_A);
        ESP_LOGI(TAG, "Transitioning to STATE_A: EVSE Ready, Vehicle Not connected");
    } else if (state == STATE_A) {// State A : EVSE Ready, Vehicle Not connected
        int cp_mV = getCPVoltage();
        if ((cp_mV < 10000) && (cp_mV > 5000)) { // Check if the CP voltage is within the range for state B (9V +-1%)
            if (mainPowerIsReady()) { // Main power is working
                // Start Timer for send PWM signal
                setCPLogic(LOW); // Set TX pin to LOW to stop output +12V at CP pin
                pwm_state = 0;
                startPWMTimer(900);

                state = STATE_B; // Transition to state B (EVSE Ready, Vehicle Connected)
                onStateChange(STATE_A, STATE_B);
                ESP_LOGI(TAG, "Transitioning to STATE_B: EVSE Ready, Vehicle Connected");
            } else {
                state = STATE_ERROR;
                onStateChange(STATE_A, STATE_ERROR);
                ESP_LOGE(TAG, "Transitioning to STATE_ERROR: Main power not ready");
            }
        } else if (cp_mV < 5000) {
            state = STATE_ERROR; // Transition to error state if CP voltage is too low
            onStateChange(STATE_A, STATE_ERROR);
            ESP_LOGE(TAG, "Transitioning to STATE_ERROR: CP voltage too low (%d mV)", cp_mV);
        }
    } else if (state == STATE_B) { // State B : EVSE Ready, Vehicle Connected
        if (cp_mV_in_pwm_high_last_update_flag) {
            cp_mV_in_pwm_high_last_update_flag = false;
            if ((cp_mV_in_pwm_high_last >= 5000) && (cp_mV_in_pwm_high_last <= 7000)) {
                if (onPowerOnReq()) {
                    ESP_LOGI(TAG, "Transitioning to STATE_C: EVSE Ready, Vehicle Connected, Power out ON");
                    state = STATE_C; // Transition to state B (EVSE Ready, Vehicle Connected and Ready to charge)
                    onStateChange(STATE_B, STATE_C);
                } else {
                    ESP_LOGI(TAG, "Ready to STATE_C but wait user confirm: EVSE Ready, Vehicle Connected, Power out stall Off");
                }
            } else if (cp_mV_in_pwm_high_last > 7000) {
                ESP_LOGI(TAG, "Transitioning to STATE_A: detect disconnect cabal (%d mV)", cp_mV_in_pwm_high_last);
                timerStop(cp_pwm_timer_handle);
                setCPLogic(HIGH);
                state = STATE_A; // Set state to state A (EVSE Ready, Vehicle Not connected)
                onStateChange(STATE_B, STATE_A);
            }
        }
        if (cp_mV_in_pwm_low_last_update_flag) {
            cp_mV_in_pwm_low_last_update_flag = false;
            // ESP_LOGI(TAG, "CP in low : %d", cp_mV_in_pwm_low_last);
            if (cp_mV_in_pwm_low_last > -10000) {
                ESP_LOGI(TAG, "Transitioning to STATE_ERROR: CP voltage in negative pluse too low (%d mV)", cp_mV_in_pwm_low_last);

                state = STATE_ERROR; // Transition to error state if CP voltage is too low
                onStateChange(STATE_B, STATE_ERROR);
            }
        }
    } else if (state == STATE_C) { // State C : EVSE Ready, Vehicle Connected
        if (cp_mV_in_pwm_high_last_update_flag) {
            cp_mV_in_pwm_high_last_update_flag = false;
            if (cp_mV_in_pwm_high_last > 7000) {
                // Change finish
                onPowerOffReq();
                state = STATE_B;
                onStateChange(STATE_C, STATE_B);
                ESP_LOGI(TAG, "Transitioning to STATE_B: Change Finish, Power out OFF (%d mV)", cp_mV_in_pwm_high_last);
            }/* else if (cp_mV_in_pwm_high_last < 5000) {
                ESP_LOGI(TAG, "Transitioning to STATE_ERROR: CP voltage in positive pluse too low (%d mV)", cp_mV_in_pwm_high_last);

                onPowerOffReq();
                onStateChange(STATE_C, STATE_ERROR);
                state = STATE_ERROR; // Transition to error state if CP voltage is too low
            }*/
        }
    } else if (state == STATE_ERROR) {
        timerStop(cp_pwm_timer_handle);
        setCPLogic(HIGH);
        state = STATE_ERROR_WAIT_DISCONNECT;
    } else if (state == STATE_ERROR_WAIT_DISCONNECT) {
        int cp_mV = getCPVoltage();
        if (cp_mV >= 9500) {
            state = STATE_INITIAL;
            onStateChange(STATE_ERROR, STATE_INITIAL);
        }
    }
    delay(1000); // Delay for 1 second before the next iteration
}
