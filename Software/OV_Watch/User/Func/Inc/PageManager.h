#ifndef PAGE_STACK_H
#define PAGE_STACK_H

#include "../../GUI_App/ui.h"

// 页面栈深度
#define MAX_DEPTH 6

// 页面结构体
typedef struct {
    void (*init)(void);    //初始化函数指针
    void (*deinit)(void);  //反初始化函数指针
    lv_obj_t **page_obj;   //屏幕对象指针的指针
} Page_t;

// Page_t 是这个 UI 框架里用来描述「一个页面」的统一结构体——你可以把它理解
// 成每个页面的「身份证/说明书」。页面管理器（PageManager）不关心每个
// 页面内部长什么样，只靠这张「身份证」上的三条信息来统一调度所有页面。

// 页面堆栈结构体
typedef struct {
    Page_t* pages[MAX_DEPTH];
    uint8_t top;
} PageStack_t;

extern PageStack_t PageStack;


Page_t* Page_Get_NowPage(void);
void Page_Back(void);
void Page_Back_Bottom(void);
void Page_Load(Page_t *newPage);
void Pages_init(void);


#endif // PAGE_STACK_H
