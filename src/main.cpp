#include <Arduino.h>
#include <Preferences.h>
#include <lvgl.h>
#include <ATD3.5-S3.h>
#include "PilotController.h"
#include <MCM_BL0940.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "gui/ui.h"

static const char * TAG = "Main";

#define POWER_OUT_PIN (1)
#define BL0940_RX_PIN (6)
#define BL0940_TX_PIN (7)
#define STOP_SW_PIN   (41)
#define STOP_SW_ACTIVE (LOW)
#define DS18B20_PIN   (4)

#define MAX_CHARGE_CURRENT_DEFAULT (MAX_CURRENT_6A) // Maximum current in Amperes
#define OVER_TEMP_THRESHOLD (60.0f)

Preferences configs;
PilotController pilotController;
BL0940 bl0940;
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

#define SETTINGS_KEY_MAX_CURRENT "max_current"

static bool user_confirm_start_flag = false;
static bool power_out = false;
static float energy_at_start = -1.0f;
static MaxAvailableCurrent_t max_current = MAX_CHARGE_CURRENT_DEFAULT;
static float temp_C = DEVICE_DISCONNECTED_C;

extern void Animation_play() ;
extern void Animation_stop() ;

void onStateChangeCallback(PilotState_t from, PilotState_t to) {
  if (to == STATE_A) {
    lv_safe_update([](void*) {
      lv_label_set_text(ui_heading_label, "เชื่อมต่อหัวชาร์จ");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "เพื่อเริ่มต้นการชาร์จ");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
    });

    user_confirm_start_flag = false;
    energy_at_start = -1.0f;
  } else if (to == STATE_B) {
    if (from == STATE_A) {
      lv_safe_update([](void*) {
        lv_label_set_text(ui_heading_label, "กำลังติดต่อกับรถ");
        lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_subtitle_label, "อาจใช้เวลาซักครู่หนึ่ง");
        lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
      });
    } else if (from == STATE_C) {
      lv_safe_update([](void*) {
        lv_label_set_text(ui_heading_label, "ชาร์จเสร็จสิ้น");
        lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_subtitle_label, "ปลดล็อกรถและถอดหัวชาร์จ");
        lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
      });
    }
  } else if (to == STATE_C) {
    
  } else if (to == STATE_F) {
    lv_safe_update([](void*) {
      lv_label_set_text(ui_heading_label, "เกิดข้อผิดพลาด");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "โปรดเชื่อมต่อคอมพิวเตอร์");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
    });
  }
}

void startBtnClickHandle(lv_event_t * event) {
  user_confirm_start_flag = true;
  lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
}

void maxCurrentDropdownValueChangeHandle(lv_event_t * event) {
  max_current = (MaxAvailableCurrent_t) lv_dropdown_get_selected(ui_max_current_dropdown);
  configs.putUInt(SETTINGS_KEY_MAX_CURRENT, (uint32_t) max_current);
  pilotController.setMaxCurrent(max_current);
}

float voltage;
float current; // unit A
float activePower; // unit W
float activeEnergy; // uint kW

void powerValueUpdate(lv_timer_t *) {
  // Voltage
  if (bl0940.getVoltage(&voltage)) {
    lv_label_set_text_fmt(ui_voltage_value_label, "%.01f", voltage);
  } else {
    lv_label_set_text(ui_voltage_value_label, "-");
  }

  // Current
  if (bl0940.getCurrent(&current)) {
    /* if (current < 0.5) { // if less then 0.5A
      current = 0; // maybe is noise so show zero
    } */
    lv_label_set_text_fmt(ui_current_value_label, "%.02f", current);
  } else {
    lv_label_set_text(ui_current_value_label, "-");
  }

  // Power
  if (bl0940.getActivePower(&activePower)) {
    lv_label_set_text_fmt(ui_power_value_label, "%.01f", activePower / 1000.0f);
  } else {
    lv_label_set_text(ui_power_value_label, "-");
  }
  
  // Energy
  if (energy_at_start >= 0) {
    if (bl0940.getActiveEnergy(&activeEnergy)) {
      activeEnergy -= energy_at_start;
      lv_label_set_text_fmt(ui_energy_value_label, "%.01f", activeEnergy);
    } else {
      lv_label_set_text(ui_energy_value_label, "-");
    }
  } else {
    lv_label_set_text(ui_energy_value_label, "-");
  }
}

void tempUpdateTask(void*) {
  ds18b20.begin();

  while(1) {
    ds18b20.requestTemperatures();
    temp_C = ds18b20.getTempCByIndex(0);
    if (temp_C != DEVICE_DISCONNECTED_C) {
      ESP_LOGV(TAG, "Temperature inbox is %.01f *C", temp_C);
    } else {
      ESP_LOGW(TAG, "Temperature sensor read failed");
    }
    delay(2000);
  }
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);

  pinMode(POWER_OUT_PIN, OUTPUT);
  digitalWrite(POWER_OUT_PIN, LOW); // Set power output pin to LOW initially

  pinMode(STOP_SW_PIN, INPUT_PULLUP);

  // Setup peripherals
  Display.begin(0); // rotation number 0
  Touch.begin();
  Sound.begin();
  // Card.begin(); // uncomment if you want to Read/Write/Play/Load file in MicroSD Card
  bl0940.begin(Serial1, BL0940_RX_PIN, BL0940_TX_PIN); // RX pin - TX pin 
  configs.begin("EVSE");

  // Map peripheral to LVGL
  Display.useLVGL(); // Map display to LVGL
  Touch.useLVGL(); // Map touch screen to LVGL
  Sound.useLVGL(); // Map speaker to LVGL
  // Card.useLVGL(); // Map MicroSD Card to LVGL File System

  // Display.enableAutoSleep(120); // Auto off display if not touch in 2 min
  
  // Add load your UI function
  ui_init();

  // Init UI
  /*
  lv_label_set_text(ui_heading_label, "");
  lv_label_set_text(ui_subtitle_label, "");
  */
  lv_label_set_text(ui_heading_label, "เชื่อมต่อหัวชาร์จ");
  lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(ui_subtitle_label, "เพื่อเริ่มต้นการชาร์จ");
  lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);

  lv_label_set_text(ui_power_value_label, "-");
  lv_label_set_text(ui_energy_value_label, "-");
  lv_label_set_text(ui_voltage_value_label, "-");
  lv_label_set_text(ui_current_value_label, "-");

  if (configs.isKey(SETTINGS_KEY_MAX_CURRENT)) {
    max_current = (MaxAvailableCurrent_t) configs.getUInt(SETTINGS_KEY_MAX_CURRENT);
    if (max_current > MAX_CURRENT_30A) {
      max_current = MAX_CHARGE_CURRENT_DEFAULT;
    }
  }
  lv_dropdown_set_selected(ui_max_current_dropdown, (uint16_t) max_current);

  // Add event handle
  lv_obj_add_event_cb(ui_start_btn, startBtnClickHandle, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(ui_max_current_dropdown, maxCurrentDropdownValueChangeHandle, LV_EVENT_VALUE_CHANGED, NULL);

  bl0940.Reset();
  bl0940.setFrequency(50); // 50 Hz
  bl0940.setUpdateRate(400); // 400 mS
  bl0940.setCurrentOffset(-52);
  bl0940.setActivePowerOffset(80);

  pilotController.begin(max_current);
  pilotController.onStateChange(onStateChangeCallback);

  // Add timer
  lv_timer_create(powerValueUpdate, 2000, NULL);

  // Task
  xTaskCreate(tempUpdateTask, "Temp Update Task", 4 * 1024, NULL, 5, NULL);
}

void loop() {
  Display.loop(); // Keep GUI work
  pilotController.loop();

  bool protection_active = false;

  // Temperature protection
  if ((temp_C != DEVICE_DISCONNECTED_C) && (temp_C >= OVER_TEMP_THRESHOLD)) { // Check over temperature
    protection_active = true; // Over Temp Protect
  }

  // Over currant protection
  if (
    ((max_current == MAX_CURRENT_6A) && (current > 8.0f)) ||
    ((max_current == MAX_CURRENT_12A) && (current > 14.0f)) ||
    ((max_current == MAX_CURRENT_18A) && (current > 20.0f)) ||
    ((max_current == MAX_CURRENT_24A) && (current > 26.0f)) ||
    ((max_current == MAX_CURRENT_30A) && (current > 32.0f))
  ) {
    protection_active = true; // Over Currant Protect
  }

  // Stop detect
  if (digitalRead(STOP_SW_PIN) == STOP_SW_ACTIVE) { // if Stop Switch are press
    if (lv_obj_has_flag(ui_emergency_dialog, LV_OBJ_FLAG_HIDDEN)) { // if Emergency Dialog not show
      lv_obj_clear_flag(ui_emergency_dialog, LV_OBJ_FLAG_HIDDEN); // show Emergency Dialog
    }
    protection_active = true; // Stop Switch Active
  } else { // but if Stop Switch not press
    if (!lv_obj_has_flag(ui_emergency_dialog, LV_OBJ_FLAG_HIDDEN)) { // if Emergency Dialog show
      lv_obj_add_flag(ui_emergency_dialog, LV_OBJ_FLAG_HIDDEN); // hide Emergency Dialog
    }
  }

  if (protection_active) {
    if (power_out) {
      digitalWrite(POWER_OUT_PIN, LOW); // Power out OFF
      power_out = false;
      Serial.println("Power out OFF by protection active !");
    }
    if (user_confirm_start_flag) {
      user_confirm_start_flag = false;
    }
    lv_label_set_text(ui_heading_label, "ระบบป้องกันทำงาน");
    lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_subtitle_label, "โปรดถอดหัวชาร์จ และตรวจสอบหัวชาร์จ");
    lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
  }

  if (pilotController.getLastState() == STATE_C) {
    if (!power_out) {
      if (user_confirm_start_flag) {
        digitalWrite(POWER_OUT_PIN, HIGH); // Power out ON
        power_out = true;
        Serial.println("Power out ON !");

        // UI update
        lv_label_set_text(ui_heading_label, "กำลังชาร์จ");
        lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_subtitle_label, "โปรดล็อครถของคุณ");
        lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);

        Animation_play();
      } else {
        // UI update
        if (lv_obj_has_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN)) {
          lv_label_set_text(ui_heading_label, "กดเริ่มต้นเพื่อชาร์จ");
          lv_obj_add_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
          lv_obj_clear_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
        }
      }
    }
    if (energy_at_start < 0) {
      float activeEnergy; // uint kW
      if (bl0940.getActiveEnergy(&activeEnergy)) {
        energy_at_start = activeEnergy;
      } else {
        ESP_LOGE(TAG, "Get energy at start fail");
      }
    }
  } else {
    if (power_out) {
      digitalWrite(POWER_OUT_PIN, LOW); // Power out OFF
      power_out = false;
      Serial.println("Power out OFF !");

      // UI update
      Animation_stop();
    }
  }

  delay(5);
}
