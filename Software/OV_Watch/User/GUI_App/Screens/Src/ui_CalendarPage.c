/* ============================================================
 * 日历页面（Calendar Page）
 * ------------------------------------------------------------
 * 作用：显示一个"月历"，可以用左右箭头翻月查看日期。
 *       用户从【菜单页】点"日历"图标进入本页。
 *
 * 数据来源：通过 HWInterface（硬件接口中间层）读取 RTC 实时时钟，
 *           拿到"今天"的年/月/日，让日历首次打开时自动定位到今天。
 *
 * 页面生命周期（由 PageManager 页面管理器控制）：
 *   - 进入本页时  → PageManager 调用 ui_CalendarPage_screen_init()
 *   - 离开本页时  → PageManager 调用 ui_CalendarPage_screen_deinit()
 *
 * 【说明 - 返回方式】
 *   本页返回菜单页靠【物理按键 key1】（见 user_ScrRenewTask.c 里的 Page_Back），
 *   不需要触摸右滑手势，所以本页没有 lv_obj_add_event_cb 是正常的。
 * ============================================================ */

#include "../../ui.h"                        // 引入所有 LVGL 控件与字体的声明
#include "../../ui_helpers.h"                // SquareLine Studio 生成的辅助宏/函数
#include "../Inc/ui_CalendarPage.h"          // 本页自己的头文件（声明本页函数和控件）
#include "../../../Func/Inc/HWDataAccess.h"  // 硬件数据中间层：提供 HWInterface 统一接口

/* ==================== 页面注册 ====================
 * 把一个"页面"登记进 PageManager 的页面栈里。
 * Page_t 的三个成员分别是：初始化函数、反初始化函数、屏幕对象指针。
 * 菜单页点"日历"时会执行 Page_Load(&Page_Calender)，从而调用这里的 init。
 */
Page_t Page_Calender = {
    ui_CalendarPage_screen_init,     // init  ：进入页面时创建控件
    ui_CalendarPage_screen_deinit,   // deinit：离开页面时清理
    &ui_CalendarPage                 // 指向本页的屏幕对象
};

/* ==================== 页面控件变量 ====================
 * 这两个是 LVGL 的"对象"（控件）句柄，其他函数通过它们操作控件。
 */
lv_obj_t * ui_CalendarPage;           // 本页的"屏幕"（最外层容器）
lv_obj_t * ui_CalendarPageCalendar;   // 屏幕上的"日历"控件

/* ==================== 页面初始化 ==================== */
void ui_CalendarPage_screen_init(void)
{
    /* 1. 从 RTC 实时时钟读取"今天"的年/月/日 */
    HW_DateTimeTypeDef DateTime;
    HWInterface.RealTimeClock.GetTimeDate(&DateTime);
    // 注意：DateTime.Year 只存年份的后两位（如 23 表示 2023），
    //       所以下面要 +2000 还原成完整年份。

    /* 2. 创建一个空屏幕（页面） */
    ui_CalendarPage = lv_obj_create(NULL);
    // 关掉屏幕的"可滚动"属性：日历页要固定显示，不需要整屏滚动
    lv_obj_clear_flag(ui_CalendarPage, LV_OBJ_FLAG_SCROLLABLE);

    /* 3. 在屏幕上创建一个日历控件 */
    ui_CalendarPageCalendar = lv_calendar_create(ui_CalendarPage);

    /* 4. 告诉日历"今天是哪一天"（用于在日历里高亮今天） */
    lv_calendar_set_today_date(ui_CalendarPageCalendar,
                               2000 + DateTime.Year, DateTime.Month, DateTime.Date);

    /* 5. 让日历"显示哪个月"（首次打开直接定位到今天所在的月） */
    lv_calendar_set_showed_date(ui_CalendarPageCalendar,
                                2000 + DateTime.Year, DateTime.Month);

    /* 6. 给日历加一个"左右箭头"的头部，用来翻上一个月 / 下一个月 */
    lv_obj_t * ui_CalendarPageCalendar_header =
        lv_calendar_header_arrow_create(ui_CalendarPageCalendar);

    /* 7. 调整日历控件的大小和位置 */
    lv_obj_set_width(ui_CalendarPageCalendar, 240);                  // 宽 240 像素
    lv_obj_set_height(ui_CalendarPageCalendar, 280);                 // 高 280 像素
    lv_obj_set_align(ui_CalendarPageCalendar, LV_ALIGN_CENTER);      // 居中显示

    /* 8. 去掉日历外框的圆角和边框（让日历铺满屏幕、看起来更干净） */
    lv_obj_set_style_radius(ui_CalendarPageCalendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_CalendarPageCalendar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 说明：本页不需要注册触摸右滑返回手势。
     * 返回菜单页靠物理按键 key1（user_ScrRenewTask.c 里的 Page_Back）。
     */
}

/* ==================== 页面反初始化 ==================== */
void ui_CalendarPage_screen_deinit(void)
{
    // 本页目前没有需要手动释放的资源（比如定时器），所以保持为空。
    // 说明：其他页面如首页有一个 500ms 刷新定时器，离开时要用 lv_timer_del 删掉；
    //       本页暂时没有定时器，因此什么都不用做。
    //       （等以后给本页加了定时器或选中日期逻辑，这里再补相应的清理。）
}
