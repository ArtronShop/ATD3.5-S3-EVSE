#include <Arduino.h>
#include "PilotController.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "soc/gpio_struct.h"
#include "soc/sens_struct.h"
#include "esp_log.h"

static const char * TAG = "PilotController";

static const uint8_t i2c_addr = 0x55;

static char StateNumberToChar(int n) {
    if (n >= 1) {
        return 'A' + (n - 1);
    }

    return 'I';
}

PilotController::PilotController() {
    memset(&pilotController_register, 0, sizeof(pilotController_register));
}

bool PilotController::writeRegister(uint8_t addr, uint8_t * value) {
    Wire.beginTransmission(i2c_addr);
    Wire.write(addr);
    Wire.write(*value);
    if (Wire.endTransmission() != 0) {
        ESP_LOGE(TAG, "Write data 0x%02X to register 0x%02X failed", *value, addr);
    }

    return true;
}

bool PilotController::readRegister(uint8_t addr, uint8_t * value) {
    Wire.beginTransmission(i2c_addr);
    Wire.write(addr);
    Wire.endTransmission(false);

    int len = Wire.requestFrom(i2c_addr, 1);
    if (len < 1) {
        ESP_LOGE(TAG, "Read data from register 0x%02X failed", addr);
        return false;
    }

    *value = Wire.read();

    return true;
}

bool PilotController::begin(MaxAvailableCurrent_t max_current) {
    this->max_current = max_current;

    Wire.begin();

    pilotController_register.CR.SWRST = 1; // Reset flag
    if (!this->writeRegister(CR_REGISTER, (uint8_t *)(&pilotController_register.CR))) {
        ESP_LOGE(TAG, "Software reset fail !");
        return false;
    }

    delay(20); // wait reboot

    pilotController_register.CR.MAC = max_current & 0x07; // Set Max Current
    pilotController_register.CR.STCIE = 1; // State Change Interrupt Enable
    pilotController_register.CR.DFDIE = 1; // Diode Failed Detect Interrupt Enable
    pilotController_register.CR.SWRST = 0; // No reset
    if (!this->writeRegister(CR_REGISTER, (uint8_t *)(&pilotController_register.CR))) {
        ESP_LOGE(TAG, "Configs register CR fail");
        return false;
    }

    return true; // Indicate successful initialization
}

void PilotController::onStateChange(OnStateChange_CB cb) {
    this->onStateChange_cb = cb;
}

bool PilotController::setMaxCurrent(MaxAvailableCurrent_t max_current) {
    if (!this->readRegister(CR_REGISTER, (uint8_t*)(&pilotController_register.CR))) {
        ESP_LOGE(TAG, "Read configs registor fail !");
        return false;
    }

    pilotController_register.CR.MAC = max_current & 0x07; // Set Max Current

    if (!this->writeRegister(CR_REGISTER, (uint8_t *)(&pilotController_register.CR))) {
        ESP_LOGE(TAG, "Configs register CR fail");
        return false;
    }

    this->max_current = max_current;

    return true;
}

PilotState_t PilotController::getLastState() {
    return (PilotState_t) pilotController_register.SR.ST;
}

bool PilotController::loop() {
    static unsigned long timer = 0;
    if ((millis() < timer) || (timer == 0) || ((millis() - timer) >= 100)) { // polling 100 mS
      timer = millis();

        PilotState_t last_state = (PilotState_t) pilotController_register.SR.ST;

        if (!this->readRegister(SR_REGISTER, (uint8_t*)(&pilotController_register.SR))) {
            ESP_LOGE(TAG, "Read status register fail !");
            return false;
        }

        // ESP_LOGV(TAG, "State is %c", StateNumberToChar((int) pilotController_register.SR.ST));

        if (pilotController_register.SR.STCIF) {
            ESP_LOGV(TAG, "State change from %c to %c", StateNumberToChar((int) last_state), StateNumberToChar((int) pilotController_register.SR.ST));

            if (this->onStateChange_cb) {
                this->onStateChange_cb(last_state, (PilotState_t) pilotController_register.SR.ST);
            }
        }
    }

    return true;
}
