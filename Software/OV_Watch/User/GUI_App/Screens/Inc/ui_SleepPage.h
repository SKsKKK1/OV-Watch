#ifndef UI_SLEEPPAGE_H
#define UI_SLEEPPAGE_H

#include "../../ui.h"

#ifdef __cplusplus
extern "C" {
#endif

extern Page_t Page_Sleep;
extern lv_obj_t * ui_SleepPage;
extern lv_obj_t * ui_Slider1;
extern lv_obj_t * ui_Label1;
extern lv_obj_t * ui_Label2;
extern lv_obj_t * ui_Label3;
extern lv_obj_t * ui_Label4;
extern lv_obj_t * ui_Switch1;

void ui_SleepPage_screen_init(void);
void ui_SleepPage_screen_deinit(void);

#ifdef __cplusplus
}
#endif

#endif
