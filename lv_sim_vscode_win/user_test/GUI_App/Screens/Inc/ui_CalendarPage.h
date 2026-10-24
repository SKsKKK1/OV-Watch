#ifndef _UI_CALENDARPAGE_H
#define _UI_CALENDARPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* 本页的控件句柄：在 .c 文件里创建，其他文件（如需）通过这里 extern 引用 */
extern lv_obj_t * ui_CalendarPage;            // 日历页的"屏幕"对象
extern lv_obj_t * ui_CalendarPageCalendar;    // 屏幕上的"日历"控件

/* 注册到 PageManager 的页面结构体：菜单页就是用它把本页压入页面栈 */
extern Page_t Page_Calender;

/* 页面的两个生命周期函数：由 PageManager 在进出页面时调用 */
void ui_CalendarPage_screen_init(void);    // 进入页面时创建控件
void ui_CalendarPage_screen_deinit(void);  // 离开页面时清理

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
