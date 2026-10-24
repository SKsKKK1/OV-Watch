#include "../../ui.h"
#include "../Inc/ui_SleepPage.h"
#include "../../../Func/Inc/HWDataAccess.h"
#include "../../../Tasks/Inc/user_BleCommTask.h"
#include "../../../Tasks/Inc/user_SensUpdateTask.h"

/* ==================== 页面注册 ====================
 * 睡眠页：三挡滑块（左=清醒 / 中=准备入睡 / 右=睡眠）+ 心率检测开关。
 */
Page_t Page_Sleep = {ui_SleepPage_screen_init, ui_SleepPage_screen_deinit, &ui_SleepPage};

/* ==================== 控件变量 ==================== */
lv_obj_t * ui_SleepPage;
lv_obj_t * ui_Slider1;
lv_obj_t * ui_Label1;
lv_obj_t * ui_Label2;
lv_obj_t * ui_Label3;
lv_obj_t * ui_Label4;
lv_obj_t * ui_Switch1;

/* ==================== 事件回调 ====================
 * 睡眠三挡滑块：写入 g_sleep_flag（0=清醒 1=准备入睡 2=睡眠），并实时同步到蓝牙
 */
static void ui_SleepSlider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    if (val <= 0)      g_sleep_flag = 0;
    else if (val >= 2) g_sleep_flag = 2;
    else               g_sleep_flag = 1;
    BLE_SyncSleepFlag();   // 实时同步睡眠标志到蓝牙
}

/* ==================== 事件回调 ====================
 * 心率检测开关：打开 = 开始心率检测（等同于进入心率页），关闭 = 停止
 */
static void ui_SleepHRSwitch_event_cb(lv_event_t * e)
{
    lv_obj_t * sw = lv_event_get_target(e);
    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        g_hr_measure = 1;   // 开启心率检测
    } else {
        g_hr_measure = 0;   // 关闭
        HWInterface.HR_meter.Sleep();   // 显式关闭心率传感器（关 LED）
    }
}

/* ==================== 页面初始化 ==================== */
void ui_SleepPage_screen_init(void)
{
    ui_SleepPage = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_SleepPage, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    /* 三挡滑块 */
    ui_Slider1 = lv_slider_create(ui_SleepPage);
    lv_slider_set_range(ui_Slider1, 0, 2);
    lv_slider_set_value(ui_Slider1, g_sleep_flag, LV_ANIM_OFF);
    lv_obj_set_width(ui_Slider1, 138);
    lv_obj_set_height(ui_Slider1, 10);
    lv_obj_set_x(ui_Slider1, 4);
    lv_obj_set_y(ui_Slider1, 20);
    lv_obj_set_align(ui_Slider1, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(ui_Slider1, ui_SleepSlider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* 标签：准备入睡（中间） */
    ui_Label1 = lv_label_create(ui_SleepPage);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_Label1, 3);
    lv_obj_set_y(ui_Label1, -13);
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "准备入睡");
    lv_obj_set_style_text_font(ui_Label1, &ui_font_fzcyjt16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 标签：清醒（左边） */
    ui_Label2 = lv_label_create(ui_SleepPage);
    lv_obj_set_width(ui_Label2, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label2, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_Label2, -70);
    lv_obj_set_y(ui_Label2, -13);
    lv_obj_set_align(ui_Label2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label2, "清醒");
    lv_obj_set_style_text_font(ui_Label2, &ui_font_fzcyjt16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 标签：睡眠（右边） */
    ui_Label3 = lv_label_create(ui_SleepPage);
    lv_obj_set_width(ui_Label3, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label3, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_Label3, 76);
    lv_obj_set_y(ui_Label3, -13);
    lv_obj_set_align(ui_Label3, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label3, "睡眠");
    lv_obj_set_style_text_font(ui_Label3, &ui_font_fzcyjt16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 标签：心率 */
    ui_Label4 = lv_label_create(ui_SleepPage);
    lv_obj_set_width(ui_Label4, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_Label4, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_Label4, -70);
    lv_obj_set_y(ui_Label4, -101);
    lv_obj_set_align(ui_Label4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label4, "心率");
    lv_obj_set_style_text_font(ui_Label4, &ui_font_fzcyjt16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 心率检测开关 */
    ui_Switch1 = lv_switch_create(ui_SleepPage);
    lv_obj_set_width(ui_Switch1, 50);
    lv_obj_set_height(ui_Switch1, 25);
    lv_obj_set_x(ui_Switch1, -70);
    lv_obj_set_y(ui_Switch1, -69);
    lv_obj_set_align(ui_Switch1, LV_ALIGN_CENTER);
    if (g_hr_measure) {
        lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);   // 已开启则显示开
    }
    lv_obj_add_event_cb(ui_Switch1, ui_SleepHRSwitch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

/* ==================== 页面反初始化 ====================
 * 退出睡眠页时，清除本页的临时状态：
 *   心率开关关闭、睡眠标志恢复清醒，并同步到蓝牙。
 */
void ui_SleepPage_screen_deinit(void)
{
    g_hr_measure = 0;      // 退出关闭心率检测
    HWInterface.HR_meter.Sleep();   // 关闭心率传感器
    g_sleep_flag = 0;      // 退出恢复清醒
    BLE_SyncSleepFlag();   // 同步睡眠标志到蓝牙
}
