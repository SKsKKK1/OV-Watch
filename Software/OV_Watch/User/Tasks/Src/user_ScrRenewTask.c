/* ============================================================
 * 屏幕刷新任务（Screen Renew Task）
 * ------------------------------------------------------------
 * 负责处理"物理按键"触发的页面返回操作。
 *
 * 手表上有两个物理按键（key1、key2），由 KeyTask 扫描，
 * 通过 Key_MessageQueue 把按键事件发到这里。
 *
 * 本任务做的事：
 *   key1 → Page_Back()        返回上一页（例如从功能页回到菜单页）
 *   key2 → Page_Back_Bottom() 直接回到首页
 *   返回菜单页时，顺便让心率/指南针等传感器进入睡眠省电。
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern osMessageQueueId_t Key_MessageQueue;

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Screen renew task
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 按键返回处理 ====================
 * 无限循环：从 Key_MessageQueue 读按键事件，执行对应的返回操作。
 *
 *   keystr == 1（key1）→ Page_Back()         返回上一页
 *   keystr == 2（key2）→ Page_Back_Bottom()   直接回首页
 *
 * 返回菜单页时，额外让心率(EM7028)/指南针(LSM303)传感器睡眠省电。
 * 注意：这里的 Page_Back 用的是物理按键触发，跟页面上的触摸手势无关。
 */
void ScrRenewTask(void *argument)
{
	uint8_t keystr=0;
	while(1)
	{
		if(osMessageQueueGet(Key_MessageQueue,&keystr,NULL,0)==osOK)
		{
			//key1 pressed
			if(keystr == 1)
			{

				Page_Back();
				if(Page_Get_NowPage()->page_obj == &ui_MenuPage) //判断现在是不是菜单页面
				{
					//HR sensor sleep
					EM7028_hrs_DisEnable();
					//sensor sleep
					LSM303DLH_Sleep();
					// SPL_Sleep();
				}
			}
			//key2 pressed
			else if(keystr == 2)
			{
				Page_Back_Bottom();
				//HR sensor sleep
				EM7028_hrs_DisEnable();
  				//sensor sleep
				LSM303DLH_Sleep();
				// SPL_Sleep();
			}
		}
		osDelay(10);
	}
}
