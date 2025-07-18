#include <Arduino.h>
#include <lvgl.h>
#include <ATD3.5-S3.h>
#include <PilotController.h>
#include "gui/ui.h"

#define POWER_OUT_PIN (1)

#define MAX_CHARGE_CURRENT (MAX_CURRENT_6A) // Maximum current in Amperes

PilotController pilotController;

volatile bool user_confirm_start_flag = false;

extern void Animation_play() ;
extern void Animation_stop() ;

static bool power_out = false;

void onStateChangeCallback(PilotState_t from, PilotState_t to) {
  if (to == STATE_A) {
    lv_safe_update([](void*) {
      lv_label_set_text(ui_heading_label, "เชื่อมต่อหัวชาร์จ");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "เพื่อเริ่มต้นการชาร์จ");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
    });

    user_confirm_start_flag = false;
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

void setup() {
  Serial.begin(115200);

  pinMode(POWER_OUT_PIN, OUTPUT);
  digitalWrite(POWER_OUT_PIN, LOW); // Set power output pin to LOW initially

  // Setup peripherals
  Display.begin(0); // rotation number 0
  Touch.begin();
  Sound.begin();
  // Card.begin(); // uncomment if you want to Read/Write/Play/Load file in MicroSD Card

  // Map peripheral to LVGL
  Display.useLVGL(); // Map display to LVGL
  Touch.useLVGL(); // Map touch screen to LVGL
  Sound.useLVGL(); // Map speaker to LVGL
  // Card.useLVGL(); // Map MicroSD Card to LVGL File System

  // Display.enableAutoSleep(120); // Auto off display if not touch in 2 min
  
  // Add load your UI function
  ui_init();

  // Init UI
  lv_label_set_text(ui_heading_label, "");
  lv_label_set_text(ui_subtitle_label, "");

  lv_label_set_text(ui_power_value_label, "-");
  lv_label_set_text(ui_energy_value_label, "-");
  lv_label_set_text(ui_voltage_value_label, "-");
  lv_label_set_text(ui_current_value_label, "-");

  // Add event handle
  lv_obj_add_event_cb(ui_start_btn, startBtnClickHandle, LV_EVENT_CLICKED, NULL);

  pilotController.begin(MAX_CHARGE_CURRENT);
  pilotController.onStateChange(onStateChangeCallback);
}

void loop() {
  Display.loop(); // Keep GUI work
  pilotController.loop();

  if (pilotController.getLastState() == STATE_C) {
    if (!power_out) {
      if (user_confirm_start_flag) {
        digitalWrite(POWER_OUT_PIN, HIGH); // Power out ON
        power_out = true;
        Serial.println("Power out ON !");

        // UI update
        lv_safe_update([](void*) {
          lv_label_set_text(ui_heading_label, "กำลังชาร์จ");
          lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
          lv_label_set_text(ui_subtitle_label, "โปรดล็อครถของคุณ");
          lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);

          Animation_play();
        });
      } else {
        // UI update
        lv_safe_update([](void*) {
          if (lv_obj_has_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN)) {
            lv_label_set_text(ui_heading_label, "กดเริ่มต้นเพื่อชาร์จ");
            lv_obj_add_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
          }
        });
      }
    }
  } else {
    if (power_out) {
      digitalWrite(POWER_OUT_PIN, LOW); // Power out OFF
      power_out = false;
      Serial.println("Power out OFF !");

      // UI update
      lv_safe_update([](void*) {
        Animation_stop();
      });
    }
  }

  delay(5);
}
