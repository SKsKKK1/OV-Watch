/* ============================================================
 * 充电检测任务（Charge Check Task）
 * ------------------------------------------------------------
 *   ChargPageEnterTask —— 充电页进入/退出任务
 *
 * 作用：盯着"充电事件"。插上充电器 → 自动跳到充电页；
 *       拔掉充电器 → 自动退出充电页。
 *
 * 完整信号链路（谁触发这个任务）：
 *   1. 插/拔充电器 → CHARGE_PIN 引脚电平变化 → 触发 EXTI2 外部中断
 *      （在 stm32f4xx_it.c 的 EXTI2_IRQHandler 里）
 *   2. 中断里调用 osEventFlagsSet 设置"事件标志" HARDINT_EVENT_CHARG
 *      （相当于按一下门铃）
 *   3. 本任务用 osEventFlagsWait 一直等这个门铃响
 *   4. 门铃响后，用 ChargeCheck() 读引脚确认真实状态：
 *      - 确实在充电 → 进充电页
 *      - 已经拔了   → 退出充电页
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_RunModeTasks.h"
#include "ui_HomePage.h"
#include "ui_ChargPage.h"
#include "main.h"
#include "HWDataAccess.h"
#include "stm32f4xx_it.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  charg page enter task
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 充电页进入任务 ====================
 * 用"事件标志"（Event Flags）等充电事件，而不是消息队列。
 *
 * 事件标志 vs 消息队列：
 *   - 消息队列 = 信箱：能往里放数据，再取出来
 *   - 事件标志 = 门铃：只有"响/没响"两种状态，不携带数据
 *
 * 这里充电检测只需要"通知一下充电状态变了"，不需要传数据，
 * 所以用事件标志（门铃）更合适、更省资源。
 *
 * 流程：
 *   1. osEventFlagsWait 永久等待（osWaitForever）充电门铃响
 *   2. 门铃响后，用 ChargeCheck() 读充电引脚确认当前真实状态
 *   3. 在充电 且 不在充电页 → 加载充电页 Page_Load(&Page_Charg)
 *   4. 没充电 且 在充电页   → 返回上一页 Page_Back()
 */
void ChargPageEnterTask(void *argument)
{
	while(1)
	{
		// 等待响应 osWaitForever = 一直等(阻塞态)不超时；没响之前任务就在这睡着，不占 CPU。
		uint32_t hardint_flags = osEventFlagsWait(HardIntEventHandle, HARDINT_EVENT_CHARG, osFlagsWaitAny, osWaitForever);

		// hardint_flags 会收到很多标志位 确认醒来是因为"充电"这个标志（用位与 & 判断特定位是否为 1）
		// 注：osEventFlagsWait 等到标志后会自动清除它（相当于门铃按完自动复位），
		// 所以下次插拔时门铃才能再次响。
		if((hardint_flags & HARDINT_EVENT_CHARG) != 0U)  
		{
			uint8_t IdleBreakstr = 0;
			// 先"打搅"一下 IdleEnterTask（发 IdleBreak 消息），
			// 别让屏幕在这会儿熄掉，因为下面可能要跳页面。
			osMessageQueuePut(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1);

			// 情况1：确实在充电，且当前不在充电页 → 跳转到充电页
			if((ChargeCheck()) && (Page_Get_NowPage()->page_obj != &ui_ChargPage))
			{
				Page_Load(&Page_Charg);
			}
			// 充电引脚是双边沿触发
			// 情况2：没在充电，且当前在充电页 → 返回上一页
			else if((!ChargeCheck()) && (Page_Get_NowPage()->page_obj == &ui_ChargPage))
			{
				Page_Back();
			}
			// 其余两种情况（在充且已在充电页 / 没充且不在充电页）什么都不做，本来就对。
		}
	}
}
