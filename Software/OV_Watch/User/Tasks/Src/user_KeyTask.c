/* ============================================================
 * 按键检测任务（Key Task）
 * ------------------------------------------------------------
 * 负责扫描手表的两个物理按键（key1、key2），
 * 把按键事件通过消息队列分发给其他任务。
 *
 * 两个按键的用途：
 *   key1 → 发 Key_MessageQueue（ScrRenewTask 收到后 Page_Back 返回上一页）
 *   key2 → 首页时发 Stop_MessageQueue（StopEnterTask 收到后进入休眠）
 *          其他页面时发 Key_MessageQueue（ScrRenewTask 收到后回首页）
 *
 * 另外，任何按键按下都会发 IdleBreak_MessageQueue，
 * 通知 IdleEnterTask"用户有操作了"，恢复亮屏、清零空闲计时。
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui_HomePage.h"
#include "main.h"
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 按键扫描循环 ====================
 * 无限循环调用 KeyScan(0) 扫描物理按键，返回值：
 *   1 = key1 按下 → 发 Key_MessageQueue（ScrRenewTask 收到后 Page_Back 返回上一页）
 *   2 = key2 按下 → 分两种情况：
 *       - 当前在首页：发 Stop_MessageQueue（StopEnterTask 收到后进入休眠）
 *       - 当前在其他页：发 Key_MessageQueue（ScrRenewTask 收到后 Page_Back_Bottom 回首页）
 * 每个按键按下都会额外发 IdleBreak_MessageQueue，唤醒屏幕。
 */
void KeyTask(void *argument)
{
	uint8_t keystr=0;
	uint8_t Stopstr=0;
	uint8_t IdleBreakstr=0;
	while(1)
	{
		switch(KeyScan(0))
		{
			case 1:
				keystr = 1;
			    osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1);  //按键1事件  返回上一页
				osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1); //唤醒
				break;

			case 2:
				if(Page_Get_NowPage()->page_obj == &ui_HomePage)     //判断是不是首页
				{
					osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);  //熄灭/休眠
				}
				else
				{
					keystr = 2;
					osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1);  //按键二事件  返回首页
					osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1); //唤醒
				}
				break;
		}
		osDelay(1);// ← 阻塞点：睡 1ms
	}
}
