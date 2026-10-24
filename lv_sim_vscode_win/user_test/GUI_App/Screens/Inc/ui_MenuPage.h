/* ============================================================
 * 菜单页（Menu Page）头文件
 * ------------------------------------------------------------
 * 菜单页是手表的"主菜单 / 功能列表页"：一屏竖排的功能入口列表，
 * 每个入口（日历、计算器、秒表、心率……）点击后跳到对应功能页。
 *
 * 本文件只做两件事：
 *   1. 用 XXX_PAGE_EN 宏（值 1/0）控制每个功能入口是否启用
 *   2. extern 声明菜单页的所有控件句柄，供其他文件引用
 * ============================================================ */

#ifndef _UI_MENUPAGE_H
#define _UI_MENUPAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../ui.h"

/* ==================== 功能入口开关 ====================
 * 每个宏对应一个菜单项：
 *   1 = 启用（显示在菜单里，点击能进入对应页面）
 *   0 = 禁用（不显示，点击也没反应）
 * 例如 SPROTS_PAGE_EN = 0，说明"运动"功能当前被关闭了。
 */
#define CALENDAR_PAGE_EN		1   // 日历
#define COMPUTER_PAGE_EN		1   // 计算器
#define TIMER_PAGE_EN			1   // 秒表
#define CARD_PAGE_EN            1   // 卡包
#define SPROTS_PAGE_EN			0   // 运动（已禁用）
#define HR_PAGE_EN				1   // 心率
#define O2_PAGE_EN				1   // 血氧
#define ENVIRONMENT_PAGE_EN	    1   // 环境
#define COMPASS_PAGE_EN			1   // 指南针
#define GAME_PAGE_EN            1   // 游戏
#define SET_PAGE_EN				1   // 设置
#define ABOUT_PAGE_EN			1   // 关于

/* ==================== 控件句柄声明 ====================
 * 每个菜单项由 4 个控件组成，命名规律：
 *   ui_MenuXxxPanel   容器面板（一行，240×70）
 *   ui_MenuXxxButton  左边的圆形彩色按钮
 *   ui_MenuXxxicon    按钮上的图标
 *   ui_MenuXxxLabel   右边的文字
 * 这些控件在 .c 文件里创建，这里 extern 声明供其他文件引用。
 */

/* 屏幕对象 */
extern lv_obj_t * ui_MenuPage;

/* 日历入口 */
extern lv_obj_t * ui_MenuCalPanel;
extern lv_obj_t * ui_MenuCalButton;
extern lv_obj_t * ui_MenuCalicon;
extern lv_obj_t * ui_MenuCalLabel;

/* 计算器入口 */
extern lv_obj_t * ui_MenuComPanel;
extern lv_obj_t * ui_MenuComButton;
extern lv_obj_t * ui_MenuComicon;
extern lv_obj_t * ui_MenuComLabel;

/* 秒表入口 */
extern lv_obj_t * ui_MenuTimPanel;
extern lv_obj_t * ui_MenuTimButton;
extern lv_obj_t * ui_MenuTimicon;
extern lv_obj_t * ui_MenuTimLabel;

/* 卡包入口 */
extern lv_obj_t * ui_MenuCardPanel;
extern lv_obj_t * ui_MenuCardButton;
extern lv_obj_t * ui_MenuCardicon;
extern lv_obj_t * ui_MenuCardLabel;

/* 运动入口 */
extern lv_obj_t * ui_MenuSprPanel;
extern lv_obj_t * ui_MenuSprButton;
extern lv_obj_t * ui_MenuSpricon;
extern lv_obj_t * ui_MenuSprLabel;

/* 心率入口 */
extern lv_obj_t * ui_MenuHRPanel;
extern lv_obj_t * ui_MenuHRButton;
extern lv_obj_t * ui_MenuHRicon;
extern lv_obj_t * ui_MenuHRLabel;

/* 血氧入口 */
extern lv_obj_t * ui_MenuO2Panel;
extern lv_obj_t * ui_MenuO2Button;
extern lv_obj_t * ui_MenuO2icon;
extern lv_obj_t * ui_MenuO2Label;

/* 环境入口 */
extern lv_obj_t * ui_MenuEnvPanel;
extern lv_obj_t * ui_MenuEnvButton;
extern lv_obj_t * ui_MenuEnvicon;
extern lv_obj_t * ui_MenuEnvLabel;

/* 指南针入口 */
extern lv_obj_t * ui_MenuCPPanel;
extern lv_obj_t * ui_MenuCPButton;
extern lv_obj_t * ui_MenuCPicon;
extern lv_obj_t * ui_MenuCPLabel;

/* 设置入口 */
extern lv_obj_t * ui_MenuSetPanel;
extern lv_obj_t * ui_MenuSetButton;
extern lv_obj_t * ui_MenuSeticon;
extern lv_obj_t * ui_MenuSetLabel;

/* 关于入口 */
extern lv_obj_t * ui_MenuAbPanel;
extern lv_obj_t * ui_MenuAbButton;
extern lv_obj_t * ui_MenuAbicon;
extern lv_obj_t * ui_MenuAbLabel;

/* 记录菜单列表上次滚动的位置，下次进入时恢复 */
extern int16_t ui_MenuScrollY;

/* 注册到 PageManager 的页面结构体 */
extern Page_t Page_Menu;

/* 页面的两个生命周期函数 */
void ui_MenuPage_screen_init(void);    // 进入页面时创建列表
void ui_MenuPage_screen_deinit(void);  // 离开页面时清理

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
