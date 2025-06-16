#include <lvgl.h>
#include "gui/ui_helpers.h"
#include "gui/ui.h"

lv_anim_t * out_anim1;
lv_anim_t * out_anim2;
lv_anim_t * out_anim3;
lv_anim_t * out_anim4;

void Animation_play() {
    ui_anim_user_data_t * PropertyAnimation_0_user_data = (ui_anim_user_data_t *) lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_0_user_data->target = ui_bar;
    PropertyAnimation_0_user_data->val = -1;
    lv_anim_t PropertyAnimation_0;
    lv_anim_init(&PropertyAnimation_0);
    lv_anim_set_time(&PropertyAnimation_0, 1000);
    lv_anim_set_user_data(&PropertyAnimation_0, PropertyAnimation_0_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_0, _ui_anim_callback_set_x);
    lv_anim_set_values(&PropertyAnimation_0, -60, 230);
    lv_anim_set_path_cb(&PropertyAnimation_0, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_0, 0);
    lv_anim_set_deleted_cb(&PropertyAnimation_0, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_0, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_0, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_0, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_0, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_0, false);
    out_anim1 = lv_anim_start(&PropertyAnimation_0);

    ui_anim_user_data_t * PropertyAnimation_1_user_data = (ui_anim_user_data_t *) lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_1_user_data->target = ui_car_backdrop;
    PropertyAnimation_1_user_data->val = -1;
    lv_anim_t PropertyAnimation_1;
    lv_anim_init(&PropertyAnimation_1);
    lv_anim_set_time(&PropertyAnimation_1, 1000);
    lv_anim_set_user_data(&PropertyAnimation_1, PropertyAnimation_1_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_1, _ui_anim_callback_set_opacity);
    lv_anim_set_values(&PropertyAnimation_1, 255, 0);
    lv_anim_set_path_cb(&PropertyAnimation_1, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_1, 800);
    lv_anim_set_deleted_cb(&PropertyAnimation_1, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_1, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_1, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_1, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_1, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_1, false);
    out_anim2 = lv_anim_start(&PropertyAnimation_1);

    ui_anim_user_data_t * PropertyAnimation_2_user_data = (ui_anim_user_data_t *) lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_2_user_data->target = ui_car_backdrop;
    PropertyAnimation_2_user_data->val = -1;
    lv_anim_t PropertyAnimation_2;
    lv_anim_init(&PropertyAnimation_2);
    lv_anim_set_time(&PropertyAnimation_2, 1000);
    lv_anim_set_user_data(&PropertyAnimation_2, PropertyAnimation_2_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_2, _ui_anim_callback_set_width);
    lv_anim_set_values(&PropertyAnimation_2, 50, 200);
    lv_anim_set_path_cb(&PropertyAnimation_2, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_2, 800);
    lv_anim_set_deleted_cb(&PropertyAnimation_2, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_2, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_2, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_2, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_2, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_2, false);
    out_anim3 = lv_anim_start(&PropertyAnimation_2);

    ui_anim_user_data_t * PropertyAnimation_3_user_data = (ui_anim_user_data_t *) lv_mem_alloc(sizeof(ui_anim_user_data_t));
    PropertyAnimation_3_user_data->target = ui_car_backdrop;
    PropertyAnimation_3_user_data->val = -1;
    lv_anim_t PropertyAnimation_3;
    lv_anim_init(&PropertyAnimation_3);
    lv_anim_set_time(&PropertyAnimation_3, 1000);
    lv_anim_set_user_data(&PropertyAnimation_3, PropertyAnimation_3_user_data);
    lv_anim_set_custom_exec_cb(&PropertyAnimation_3, _ui_anim_callback_set_height);
    lv_anim_set_values(&PropertyAnimation_3, 50, 200);
    lv_anim_set_path_cb(&PropertyAnimation_3, lv_anim_path_linear);
    lv_anim_set_delay(&PropertyAnimation_3, 800);
    lv_anim_set_deleted_cb(&PropertyAnimation_3, _ui_anim_callback_free_user_data);
    lv_anim_set_playback_time(&PropertyAnimation_3, 0);
    lv_anim_set_playback_delay(&PropertyAnimation_3, 0);
    lv_anim_set_repeat_count(&PropertyAnimation_3, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_repeat_delay(&PropertyAnimation_3, 1000);
    lv_anim_set_early_apply(&PropertyAnimation_3, false);
    out_anim4 = lv_anim_start(&PropertyAnimation_3);
}

void Animation_stop() {
    lv_anim_del(out_anim1, NULL);
    lv_obj_set_x(ui_bar, -60);

    lv_anim_del(out_anim2, NULL);
    lv_anim_del(out_anim3, NULL);
    lv_anim_del(out_anim4, NULL);
    lv_obj_set_style_opa(ui_car_backdrop, 0, 0);
}

