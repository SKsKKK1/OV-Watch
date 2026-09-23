/* ============================================================
 * 菜单页（Menu Page）
 * ------------------------------------------------------------
 * 手表的"主菜单 / 功能列表页"：一屏竖排的功能入口列表。
 * 点某个入口（日历、计算器、心率……）就跳到对应的功能页。
 *
 * 本文件做的事：
 *   1. 注册本页到页面管理器（Page_Menu）
 *   2. 定义 12 个菜单项的控件变量
 *   3. 写每个菜单项的"点击跳转"事件回调
 *   4. 在 init 里用 LVGL 把整个列表画出来，并注册事件
 * ============================================================ */

#include "../../ui.h"
#include "../../ui_helpers.h"
#include "../Inc/ui_HomePage.h"
#include "../Inc/ui_MenuPage.h"
#include "../Inc/ui_Megboxes.h"
#include "../Inc/ui_CalendarPage.h"
#include "../Inc/ui_ComputerPage.h"
#include "../Inc/ui_TimerPage.h"
#include "../Inc/ui_NFCCardPage.h"
//#include "ui_SprotsPage.h"
#include "../Inc/ui_HRPage.h"
#include "../Inc/ui_SPO2Page.h"
#include "../Inc/ui_EnvPage.h"
#include "../Inc/ui_CompassPage.h"
#include "../Inc/ui_GameSelectPage.h"
#include "../Inc/ui_SetPage.h"
#include "../Inc/ui_AboutPage.h"


/* ==================== 页面注册 ====================
 * 把菜单页登记进页面管理器（PageManager）。
 * Page_t 三个成员：初始化函数、反初始化函数、屏幕对象指针。
 * 首页等地方用 Page_Load(&Page_Menu) 就能进入本页。
 */
Page_t Page_Menu = {ui_MenuPage_screen_init, ui_MenuPage_screen_deinit, &ui_MenuPage};

/* ==================== 控件变量 ====================
 * 每个菜单项有 4 个控件，命名规律：
 *   ui_MenuXxxPanel   容器面板（一行，240×70）
 *   ui_MenuXxxButton  左边的圆形彩色按钮
 *   ui_MenuXxxicon    按钮上的图标
 *   ui_MenuXxxLabel   右边的文字
 */
lv_obj_t * ui_MenuPage;   // 本页的屏幕对象

lv_obj_t * ui_MenuCalPanel;
lv_obj_t * ui_MenuCalButton;
lv_obj_t * ui_MenuCalicon;
lv_obj_t * ui_MenuCalLabel;

lv_obj_t * ui_MenuComPanel;
lv_obj_t * ui_MenuComButton;
lv_obj_t * ui_MenuComicon;
lv_obj_t * ui_MenuComLabel;

lv_obj_t * ui_MenuTimPanel;
lv_obj_t * ui_MenuTimButton;
lv_obj_t * ui_MenuTimicon;
lv_obj_t * ui_MenuTimLabel;

lv_obj_t * ui_MenuCardPanel;
lv_obj_t * ui_MenuCardButton;
lv_obj_t * ui_MenuCardicon;
lv_obj_t * ui_MenuCardLabel;

lv_obj_t * ui_MenuSprPanel;
lv_obj_t * ui_MenuSprButton;
lv_obj_t * ui_MenuSpricon;
lv_obj_t * ui_MenuSprLabel;

lv_obj_t * ui_MenuHRPanel;
lv_obj_t * ui_MenuHRButton;
lv_obj_t * ui_MenuHRicon;
lv_obj_t * ui_MenuHRLabel;

lv_obj_t * ui_MenuO2Panel;
lv_obj_t * ui_MenuO2Button;
lv_obj_t * ui_MenuO2icon;
lv_obj_t * ui_MenuO2Label;

lv_obj_t * ui_MenuEnvPanel;
lv_obj_t * ui_MenuEnvButton;
lv_obj_t * ui_MenuEnvicon;
lv_obj_t * ui_MenuEnvLabel;

lv_obj_t * ui_MenuCPPanel;
lv_obj_t * ui_MenuCPButton;
lv_obj_t * ui_MenuCPicon;
lv_obj_t * ui_MenuCPLabel;

lv_obj_t * ui_MenuGamePanel;
lv_obj_t * ui_MenuGameButton;
lv_obj_t * ui_MenuGameicon;
lv_obj_t * ui_MenuGameLabel;

lv_obj_t * ui_MenuSetPanel;
lv_obj_t * ui_MenuSetButton;
lv_obj_t * ui_MenuSeticon;
lv_obj_t * ui_MenuSetLabel;

lv_obj_t * ui_MenuAbPanel;
lv_obj_t * ui_MenuAbButton;
lv_obj_t * ui_MenuAbicon;
lv_obj_t * ui_MenuAbLabel;


/* 记住菜单列表上次滚动到哪，下次进来恢复到原位置（见 init 末尾的 lv_obj_scroll_to）*/
int16_t ui_MenuScrollY = 0;


///////////////////// FUNCTIONS ////////////////////
/* 本页整体的事件回调：右滑返回首页；滚动结束时记录当前位置 */
void ui_event_MenuPage(lv_event_t * e)
{
   lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);

    if(event_code == LV_EVENT_GESTURE)
    {
        if(lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT)
        {
		    Page_Back();
        }
    }
		if(event_code == LV_EVENT_SCROLL_END)
    {
        ui_MenuScrollY = lv_obj_get_scroll_y(ui_MenuPage);
    }
}


/* ==================== 日历入口 ====================
 * 点"日历"菜单项时触发：
 *   Page_Load(&Page_Calender) 把日历页压入页面栈，进入日历页。
 * 注意：这里用 Page_Load 是"前进"；日历页返回时要用 Page_Back 是"后退"。
 */
void ui_event_MenuCalPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);
	if(event_code == LV_EVENT_CLICKED)
	{
		#if CALENDAR_PAGE_EN
			Page_Load(&Page_Calender);
		#else


		#endif
	}
}

/* ==================== 以下所有 ui_event_MenuXxxPanel ====================
 * 结构完全一样，只做一件事：
 *   点击对应的菜单项 → Page_Load(&Page_Xxx) 跳到对应功能页。
 * 每个都用 #if XXX_PAGE_EN 开关包着，开关为 0 时点击无反应。
 * ======================================================================== */
void ui_event_MenuComPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);
	if(event_code == LV_EVENT_CLICKED)
	{
			#if COMPUTER_PAGE_EN
					Page_Load(&Page_Computer);
			#else


			#endif

	}
}

void ui_event_MenuTimPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
	lv_obj_t * target = lv_event_get_target(e);
	if(event_code == LV_EVENT_CLICKED)
	{
			#if TIMER_PAGE_EN
					Page_Load(&Page_Timer);
			#else


			#endif
	}
}

void ui_event_MenuCardPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if CARD_PAGE_EN
            Page_Load(&Page_NFCCard);
        #else


        #endif

    }
}

void ui_event_MenuSprPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if SPROTS_PAGE_EN

        #else


        #endif
    }
}

void ui_event_MenuHRPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if HR_PAGE_EN
					Page_Load(&Page_HR);
                    //send the sensor wakeup message

        #else


        #endif
    }
}

void ui_event_MenuO2Panel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if O2_PAGE_EN
            Page_Load(&Page_SPO2);
        #else


        #endif
    }
}

void ui_event_MenuEnvPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if ENVIRONMENT_PAGE_EN
            Page_Load(&Page_Env);
        #else


        #endif
    }
}

void ui_event_MenuCPPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if COMPASS_PAGE_EN
			Page_Load(&Page_Compass);
        #else


        #endif
    }
}

void ui_event_MenuGamePanel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if GAME_PAGE_EN
			Page_Load(&Page_GameSelect);
        #else


        #endif

    }
}

void ui_event_MenuSetPanel(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if SET_PAGE_EN
            Page_Load(&Page_Set);
        #else


        #endif

    }
}

void ui_event_MenuAbPanel(lv_event_t * e)
{
	lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    if(event_code == LV_EVENT_CLICKED)
    {
        #if ABOUT_PAGE_EN
            Page_Load(&Page_About);
        #else


        #endif
    }
}

/* ==================== 页面初始化 ====================
 * 用 LVGL 把 12 个菜单项一行行画出来。
 * 每个菜单项 = 一个 Panel 容器（里面放 Button 按钮 + icon 图标 + Label 文字）。
 * 下面"日历"这一项是最完整的样板，其余 11 项结构完全相同，
 * 只是 y 坐标（每个 +70）、图标、文字、颜色不同。
 */
void ui_MenuPage_screen_init(void)
{
    ui_MenuPage = lv_obj_create(NULL);

    /* ---- 第 1 项：日历（样板，其余 11 项结构相同）---- */
    ui_MenuCalPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuCalPanel, 240);
    lv_obj_set_height(ui_MenuCalPanel, 70);
    lv_obj_set_align(ui_MenuCalPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuCalPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCalPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuCalPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuCalPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCalPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuCalPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    /* 左边的圆形彩色按钮（40×40）*/
    ui_MenuCalButton = lv_btn_create(ui_MenuCalPanel);
    lv_obj_set_width(ui_MenuCalButton, 40);
    lv_obj_set_height(ui_MenuCalButton, 40);
    lv_obj_set_align(ui_MenuCalButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuCalButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuCalButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCalButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCalButton, lv_color_hex(0xFF8080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCalButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 按钮上的图标：用 iconfont 图标字体画的（不是图片）*/
    ui_MenuCalicon = lv_label_create(ui_MenuCalButton);
    lv_obj_set_width(ui_MenuCalicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCalicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuCalicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuCalicon, "");
    lv_obj_set_style_text_font(ui_MenuCalicon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 右边的文字"日 历" */
    ui_MenuCalLabel = lv_label_create(ui_MenuCalPanel);
    lv_obj_set_width(ui_MenuCalLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCalLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuCalLabel, 60);
    lv_obj_set_y(ui_MenuCalLabel, 0);
    lv_obj_set_align(ui_MenuCalLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuCalLabel, "日 历");
    lv_obj_set_style_text_font(ui_MenuCalLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* ---- 第 2 项：计算器（结构同上，y 坐标 +70，仅颜色/图标/文字不同）---- */
    ui_MenuComPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuComPanel, 240);
    lv_obj_set_height(ui_MenuComPanel, 70);
    lv_obj_set_x(ui_MenuComPanel, 0);
    lv_obj_set_y(ui_MenuComPanel, 70);
    lv_obj_set_align(ui_MenuComPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuComPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuComPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuComPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuComPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuComPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuComPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuComPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuComPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuComButton = lv_btn_create(ui_MenuComPanel);
    lv_obj_set_width(ui_MenuComButton, 40);
    lv_obj_set_height(ui_MenuComButton, 40);
    lv_obj_set_align(ui_MenuComButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuComButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuComButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuComButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuComButton, lv_color_hex(0xC86400), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuComButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuComicon = lv_label_create(ui_MenuComButton);
    lv_obj_set_width(ui_MenuComicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuComicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuComicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuComicon, "");
    lv_obj_set_style_text_font(ui_MenuComicon, &ui_font_iconfont28, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuComLabel = lv_label_create(ui_MenuComPanel);
    lv_obj_set_width(ui_MenuComLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuComLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuComLabel, 60);
    lv_obj_set_y(ui_MenuComLabel, 0);
    lv_obj_set_align(ui_MenuComLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuComLabel, "计 算 器");
    lv_obj_set_style_text_font(ui_MenuComLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuTimPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuTimPanel, 240);
    lv_obj_set_height(ui_MenuTimPanel, 70);
    lv_obj_set_x(ui_MenuTimPanel, 0);
    lv_obj_set_y(ui_MenuTimPanel, 140);
    lv_obj_set_align(ui_MenuTimPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuTimPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuTimPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuTimPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuTimPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuTimPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuTimPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuTimPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuTimPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuTimButton = lv_btn_create(ui_MenuTimPanel);
    lv_obj_set_width(ui_MenuTimButton, 40);
    lv_obj_set_height(ui_MenuTimButton, 40);
    lv_obj_set_align(ui_MenuTimButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuTimButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuTimButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuTimButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuTimButton, lv_color_hex(0xDC80E6), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuTimButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuTimicon = lv_label_create(ui_MenuTimButton);
    lv_obj_set_width(ui_MenuTimicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuTimicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuTimicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuTimicon, "");
    lv_obj_set_style_text_font(ui_MenuTimicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuTimLabel = lv_label_create(ui_MenuTimPanel);
    lv_obj_set_width(ui_MenuTimLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuTimLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuTimLabel, 60);
    lv_obj_set_y(ui_MenuTimLabel, 0);
    lv_obj_set_align(ui_MenuTimLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuTimLabel, "秒 表");
    lv_obj_set_style_text_font(ui_MenuTimLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCardPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuCardPanel, 240);
    lv_obj_set_height(ui_MenuCardPanel, 70);
    lv_obj_set_x(ui_MenuCardPanel, 0);
    lv_obj_set_y(ui_MenuCardPanel, 210);
    lv_obj_set_align(ui_MenuCardPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuCardPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCardPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCardPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCardPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuCardPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuCardPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCardPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuCardPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuCardButton = lv_btn_create(ui_MenuCardPanel);
    lv_obj_set_width(ui_MenuCardButton, 40);
    lv_obj_set_height(ui_MenuCardButton, 40);
    lv_obj_set_align(ui_MenuCardButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuCardButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuCardButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCardButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCardButton, lv_color_hex(0x80641E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCardButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCardicon = lv_label_create(ui_MenuCardButton);
    lv_obj_set_width(ui_MenuCardicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCardicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuCardicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuCardicon, "");
    lv_obj_set_style_text_font(ui_MenuCardicon, &ui_font_iconfont32, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCardLabel = lv_label_create(ui_MenuCardPanel);
    lv_obj_set_width(ui_MenuCardLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCardLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuCardLabel, 60);
    lv_obj_set_y(ui_MenuCardLabel, 0);
    lv_obj_set_align(ui_MenuCardLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuCardLabel, "卡 包");
    lv_obj_set_style_text_font(ui_MenuCardLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSprPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuSprPanel, 240);
    lv_obj_set_height(ui_MenuSprPanel, 70);
    lv_obj_set_x(ui_MenuSprPanel, 0);
    lv_obj_set_y(ui_MenuSprPanel, 280);
    lv_obj_set_align(ui_MenuSprPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuSprPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuSprPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSprPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuSprPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuSprPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuSprPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSprPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuSprPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuSprButton = lv_btn_create(ui_MenuSprPanel);
    lv_obj_set_width(ui_MenuSprButton, 40);
    lv_obj_set_height(ui_MenuSprButton, 40);
    lv_obj_set_align(ui_MenuSprButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuSprButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuSprButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuSprButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSprButton, lv_color_hex(0x009680), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuSprButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSpricon = lv_label_create(ui_MenuSprButton);
    lv_obj_set_width(ui_MenuSpricon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuSpricon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuSpricon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuSpricon, "");
    lv_obj_set_style_text_font(ui_MenuSpricon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSprLabel = lv_label_create(ui_MenuSprPanel);
    lv_obj_set_width(ui_MenuSprLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuSprLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuSprLabel, 60);
    lv_obj_set_y(ui_MenuSprLabel, 0);
    lv_obj_set_align(ui_MenuSprLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuSprLabel, "运 动");
    lv_obj_set_style_text_font(ui_MenuSprLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuHRPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuHRPanel, 240);
    lv_obj_set_height(ui_MenuHRPanel, 70);
    lv_obj_set_x(ui_MenuHRPanel, 0);
    lv_obj_set_y(ui_MenuHRPanel, 350);
    lv_obj_set_align(ui_MenuHRPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuHRPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuHRPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuHRPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuHRPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuHRPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuHRPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuHRPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuHRPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuHRButton = lv_btn_create(ui_MenuHRPanel);
    lv_obj_set_width(ui_MenuHRButton, 40);
    lv_obj_set_height(ui_MenuHRButton, 40);
    lv_obj_set_align(ui_MenuHRButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuHRButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuHRButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuHRButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuHRButton, lv_color_hex(0xC80000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuHRButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuHRicon = lv_label_create(ui_MenuHRButton);
    lv_obj_set_width(ui_MenuHRicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuHRicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuHRicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuHRicon, "");
    lv_obj_set_style_text_font(ui_MenuHRicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuHRLabel = lv_label_create(ui_MenuHRPanel);
    lv_obj_set_width(ui_MenuHRLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuHRLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuHRLabel, 60);
    lv_obj_set_y(ui_MenuHRLabel, 0);
    lv_obj_set_align(ui_MenuHRLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuHRLabel, "心 率");
    lv_obj_set_style_text_font(ui_MenuHRLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuO2Panel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuO2Panel, 240);
    lv_obj_set_height(ui_MenuO2Panel, 70);
    lv_obj_set_x(ui_MenuO2Panel, 0);
    lv_obj_set_y(ui_MenuO2Panel, 420);
    lv_obj_set_align(ui_MenuO2Panel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuO2Panel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuO2Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuO2Panel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuO2Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuO2Panel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuO2Panel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuO2Panel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuO2Panel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuO2Button = lv_btn_create(ui_MenuO2Panel);
    lv_obj_set_width(ui_MenuO2Button, 40);
    lv_obj_set_height(ui_MenuO2Button, 40);
    lv_obj_set_align(ui_MenuO2Button, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuO2Button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuO2Button, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuO2Button, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuO2Button, lv_color_hex(0x0080FF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuO2Button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuO2icon = lv_label_create(ui_MenuO2Button);
    lv_obj_set_width(ui_MenuO2icon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuO2icon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuO2icon, 2);
    lv_obj_set_y(ui_MenuO2icon, 0);
    lv_obj_set_align(ui_MenuO2icon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuO2icon, "");
    lv_obj_set_style_text_font(ui_MenuO2icon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuO2Label = lv_label_create(ui_MenuO2Panel);
    lv_obj_set_width(ui_MenuO2Label, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuO2Label, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuO2Label, 60);
    lv_obj_set_y(ui_MenuO2Label, 0);
    lv_obj_set_align(ui_MenuO2Label, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuO2Label, "血 氧");
    lv_obj_set_style_text_font(ui_MenuO2Label, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuEnvPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuEnvPanel, 240);
    lv_obj_set_height(ui_MenuEnvPanel, 70);
    lv_obj_set_x(ui_MenuEnvPanel, 0);
    lv_obj_set_y(ui_MenuEnvPanel, 490);
    lv_obj_set_align(ui_MenuEnvPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuEnvPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuEnvPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuEnvPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuEnvPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuEnvPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuEnvPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuEnvPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuEnvPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuEnvButton = lv_btn_create(ui_MenuEnvPanel);
    lv_obj_set_width(ui_MenuEnvButton, 40);
    lv_obj_set_height(ui_MenuEnvButton, 40);
    lv_obj_set_align(ui_MenuEnvButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuEnvButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuEnvButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuEnvButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuEnvButton, lv_color_hex(0x009632), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuEnvButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuEnvicon = lv_label_create(ui_MenuEnvButton);
    lv_obj_set_width(ui_MenuEnvicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuEnvicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuEnvicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuEnvicon, "");
    lv_obj_set_style_text_font(ui_MenuEnvicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuEnvLabel = lv_label_create(ui_MenuEnvPanel);
    lv_obj_set_width(ui_MenuEnvLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuEnvLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuEnvLabel, 60);
    lv_obj_set_y(ui_MenuEnvLabel, 0);
    lv_obj_set_align(ui_MenuEnvLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuEnvLabel, "环 境");
    lv_obj_set_style_text_font(ui_MenuEnvLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCPPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuCPPanel, 240);
    lv_obj_set_height(ui_MenuCPPanel, 70);
    lv_obj_set_x(ui_MenuCPPanel, 0);
    lv_obj_set_y(ui_MenuCPPanel, 560);
    lv_obj_set_align(ui_MenuCPPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuCPPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCPPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCPPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCPPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuCPPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuCPPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCPPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuCPPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuCPButton = lv_btn_create(ui_MenuCPPanel);
    lv_obj_set_width(ui_MenuCPButton, 40);
    lv_obj_set_height(ui_MenuCPButton, 40);
    lv_obj_set_align(ui_MenuCPButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuCPButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuCPButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuCPButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuCPButton, lv_color_hex(0x800080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuCPButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCPicon = lv_label_create(ui_MenuCPButton);
    lv_obj_set_width(ui_MenuCPicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCPicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuCPicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuCPicon, "");
    lv_obj_set_style_text_font(ui_MenuCPicon, &ui_font_iconfont34, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuCPLabel = lv_label_create(ui_MenuCPPanel);
    lv_obj_set_width(ui_MenuCPLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuCPLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuCPLabel, 60);
    lv_obj_set_y(ui_MenuCPLabel, 0);
    lv_obj_set_align(ui_MenuCPLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuCPLabel, "指 南 针");
    lv_obj_set_style_text_font(ui_MenuCPLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuGamePanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuGamePanel, 240);
    lv_obj_set_height(ui_MenuGamePanel, 70);
    lv_obj_set_x(ui_MenuGamePanel, 0);
    lv_obj_set_y(ui_MenuGamePanel, 630);
    lv_obj_set_align(ui_MenuGamePanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuGamePanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuGamePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuGamePanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuGamePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuGamePanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuGamePanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuGamePanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuGamePanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);


    ui_MenuGameButton = lv_btn_create(ui_MenuGamePanel);
    lv_obj_set_width(ui_MenuGameButton, 40);
    lv_obj_set_height(ui_MenuGameButton, 40);
    lv_obj_set_align(ui_MenuGameButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuGameButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuGameButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuGameButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuGameButton, lv_color_hex(0xC07010), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuGameButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuGameicon = lv_label_create(ui_MenuGameButton);
    lv_obj_set_width(ui_MenuGameicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuGameicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuGameicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuGameicon, "");
    lv_obj_set_style_text_font(ui_MenuGameicon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuGameLabel = lv_label_create(ui_MenuGamePanel);
    lv_obj_set_width(ui_MenuGameLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuGameLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuGameLabel, 60);
    lv_obj_set_y(ui_MenuGameLabel, 0);
    lv_obj_set_align(ui_MenuGameLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuGameLabel, "游 戏");
    lv_obj_set_style_text_font(ui_MenuGameLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSetPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuSetPanel, 240);
    lv_obj_set_height(ui_MenuSetPanel, 70);
    lv_obj_set_x(ui_MenuSetPanel, 0);
    lv_obj_set_y(ui_MenuSetPanel, 700);
    lv_obj_set_align(ui_MenuSetPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuSetPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSetPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuSetPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuSetPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSetPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuSetPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuSetButton = lv_btn_create(ui_MenuSetPanel);
    lv_obj_set_width(ui_MenuSetButton, 40);
    lv_obj_set_height(ui_MenuSetButton, 40);
    lv_obj_set_align(ui_MenuSetButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuSetButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuSetButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuSetButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuSetButton, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuSetButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSeticon = lv_label_create(ui_MenuSetButton);
    lv_obj_set_width(ui_MenuSeticon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuSeticon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuSeticon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuSeticon, "");
    lv_obj_set_style_text_font(ui_MenuSeticon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuSetLabel = lv_label_create(ui_MenuSetPanel);
    lv_obj_set_width(ui_MenuSetLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuSetLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuSetLabel, 60);
    lv_obj_set_y(ui_MenuSetLabel, 0);
    lv_obj_set_align(ui_MenuSetLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuSetLabel, "设 置");
    lv_obj_set_style_text_font(ui_MenuSetLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuAbPanel = lv_obj_create(ui_MenuPage);
    lv_obj_set_width(ui_MenuAbPanel, 240);
    lv_obj_set_height(ui_MenuAbPanel, 70);
    lv_obj_set_x(ui_MenuAbPanel, 0);
    lv_obj_set_y(ui_MenuAbPanel, 770);
    lv_obj_set_align(ui_MenuAbPanel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_MenuAbPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuAbPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuAbPanel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuAbPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_MenuAbPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_MenuAbPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuAbPanel, lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(ui_MenuAbPanel, 100, LV_PART_MAIN | LV_STATE_PRESSED);

    ui_MenuAbButton = lv_btn_create(ui_MenuAbPanel);
    lv_obj_set_width(ui_MenuAbButton, 40);
    lv_obj_set_height(ui_MenuAbButton, 40);
    lv_obj_set_align(ui_MenuAbButton, LV_ALIGN_LEFT_MID);
    lv_obj_add_flag(ui_MenuAbButton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);     /// Flags
    lv_obj_clear_flag(ui_MenuAbButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_radius(ui_MenuAbButton, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_MenuAbButton, lv_color_hex(0x646464), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_MenuAbButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuAbicon = lv_label_create(ui_MenuAbButton);
    lv_obj_set_width(ui_MenuAbicon, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuAbicon, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_align(ui_MenuAbicon, LV_ALIGN_CENTER);
    lv_label_set_text(ui_MenuAbicon, "");
    lv_obj_set_style_text_font(ui_MenuAbicon, &ui_font_iconfont30, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_MenuAbLabel = lv_label_create(ui_MenuAbPanel);
    lv_obj_set_width(ui_MenuAbLabel, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_MenuAbLabel, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_MenuAbLabel, 60);
    lv_obj_set_y(ui_MenuAbLabel, 0);
    lv_obj_set_align(ui_MenuAbLabel, LV_ALIGN_LEFT_MID);
    lv_label_set_text(ui_MenuAbLabel, "关 于");
    lv_obj_set_style_text_font(ui_MenuAbLabel, &ui_font_Cuyuan20, LV_PART_MAIN | LV_STATE_DEFAULT);


	//scroll back
	lv_obj_scroll_to(ui_MenuPage,0,ui_MenuScrollY,LV_ANIM_OFF);

	//events
	lv_obj_add_event_cb(ui_MenuPage, ui_event_MenuPage, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuCalPanel, ui_event_MenuCalPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuComPanel, ui_event_MenuComPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuTimPanel, ui_event_MenuTimPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuCardPanel, ui_event_MenuCardPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuSprPanel, ui_event_MenuSprPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuHRPanel, ui_event_MenuHRPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuO2Panel, ui_event_MenuO2Panel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuEnvPanel, ui_event_MenuEnvPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuCPPanel, ui_event_MenuCPPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuGamePanel, ui_event_MenuGamePanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuSetPanel, ui_event_MenuSetPanel, LV_EVENT_ALL, NULL);
	lv_obj_add_event_cb(ui_MenuAbPanel, ui_event_MenuAbPanel, LV_EVENT_ALL, NULL);


}

/* ==================== 页面反初始化 ====================
 * 本页没有需要手动释放的资源（无定时器等），所以为空。
 * （init 末尾的 lv_obj_scroll_to 恢复滚动位置、lv_obj_add_event_cb 注册事件，
 *   都属于本页自身资源，不需要在这里额外清理。）
 */
void ui_MenuPage_screen_deinit(void)
{}
