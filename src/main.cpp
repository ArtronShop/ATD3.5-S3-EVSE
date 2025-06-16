#include <Arduino.h>
#include <lvgl.h>
#include <ATD3.5-S3.h>
#include <PilotController.h>
#include "gui/ui.h"

#define POWER_OUT_PIN (1)
#define CP_TX_PIN (42)
#define CP_RX_PIN (2)

#define MAX_CHARGE_CURRENT (6) // Maximum current in Amperes

PilotController pilotController;

volatile bool user_confirm_start_flag = false;

extern void Animation_play() ;
extern void Animation_stop() ;

bool mainPowerIsReady() {
  // TODO: check in main power voltage
  return true;
}

bool onPowerOnReq() {
  if (user_confirm_start_flag) {
    digitalWrite(POWER_OUT_PIN, HIGH);
    lv_safe_update([](void*) {
      Animation_play();
    });
  } else {
    lv_safe_update([](void*) {
      if (lv_obj_has_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN)) {
        lv_label_set_text(ui_heading_label, "กดเริ่มต้นเพื่อชาร์จ");
        lv_obj_add_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      }
    });
  }

  return user_confirm_start_flag;
}

void onPowerOffReq() {
  digitalWrite(POWER_OUT_PIN, LOW);
  lv_safe_update([](void*) {
    Animation_stop();
  });
}

void onStateChange(PilotState_t from, PilotState_t to) {
  if (to == STATE_A) {
    lv_safe_update([](void*) {
      lv_label_set_text(ui_heading_label, "เชื่อมต่อหัวชาร์จ");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "เพื่อเริ่มต้นการชาร์จ");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
    });
  } else if (to == STATE_B) {
    if (from == STATE_A) {
      lv_safe_update([](void*) {
        lv_label_set_text(ui_heading_label, "กำลังติดต่อกับรถ");
        lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(ui_subtitle_label, "อาจใช้เวลาซักครู่หนึ่ง");
        lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
      });
    } else if (from == STATE_C) {
      lv_label_set_text(ui_heading_label, "ชาร์จเสร็จสิ้น");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "ปลดล็อกรถและถอดหัวชาร์จ");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);

      user_confirm_start_flag = false;
    }
  } else if (to == STATE_C) {
    lv_safe_update([](void*) {
      lv_label_set_text(ui_heading_label, "กำลังชาร์จ");
      lv_obj_add_flag(ui_start_btn, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(ui_subtitle_label, "โปรดล็อครถของคุณ");
      lv_obj_clear_flag(ui_subtitle_label, LV_OBJ_FLAG_HIDDEN);
    });
  } else if (to == STATE_ERROR) {
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

  pilotController.begin(CP_TX_PIN, CP_RX_PIN, MAX_CHARGE_CURRENT);
}

void loop() {
  Display.loop(); // Keep GUI work
  delay(5);
}
