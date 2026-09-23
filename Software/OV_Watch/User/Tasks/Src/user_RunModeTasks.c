/* ============================================================
 * 运行模式任务（Run Mode Tasks）
 * ------------------------------------------------------------
 * 本文件包含两个负责"电源状态管理"的 FreeRTOS 任务：
 *
 *   1. IdleEnterTask —— 空闲检测任务
 *      检测用户多久没有操作（无按键、无触摸）。
 *      空闲超过设定时间 → 先把背光调暗；再久 → 进入 STOP 低功耗休眠。
 *
 *   2. StopEnterTask —— 休眠/唤醒任务
 *      收到休眠消息后：关闭外设 → 进入 STOP 模式（芯片低功耗）。
 *      被唤醒后：恢复时钟、重新初始化外设、回到运行状态。
 *
 * 核心变量：
 *   IdleTimerCount —— 空闲计时器（毫秒），在 TaskTickHook 里每 1ms 累加，
 *                     用户有按键/触摸时清零。
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include "lcd_init.h"
#include "power.h"
#include "CST816.h"
#include "MPU6050.h"
#include "key.h"
#include "WDOG.h"

#include "HWDataAccess.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* 空闲计时器（毫秒）：记录"距离上次用户操作过了多久"。
 * 在 TaskTickHook 里每 1ms 自增；用户按键/触摸时清零。
 */
uint32_t IdleTimerCount = 0;

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
	* @brief  Enter Idle state
	* @param  argument: Not used
	* @retval None
	*/
/* ==================== 空闲检测任务 ====================
 * 无限循环做三件事：
 *   1. 收到 Idle_MessageQueue  → 背光调到最暗（熄屏）
 *   2. 收到 IdleBreak_MessageQueue → 恢复背光、清零计时（用户操作唤醒）
 *   3. IdleTimerCount 累积到阈值 → 主动发消息触发熄屏/休眠
 */
void IdleEnterTask(void *argument)
{
	uint8_t Idlestr=0;
	uint8_t IdleBreakstr=0;
	uint8_t light_off_sent = 0;
	while(1)
	{
		//light get dark
		if(osMessageQueueGet(Idle_MessageQueue,&Idlestr,NULL,1)==osOK)
		{
			LCD_Set_Light(5);
			light_off_sent = 1;
		}
		//resume light if light got dark and idle state breaked by key pressing or screen touching
		//如果灯光变暗且因按键或屏幕触摸而中断空闲状态，则恢复灯光
		if(osMessageQueueGet(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1)==osOK)
		{
			IdleTimerCount = 0;
			light_off_sent = 0;
			LCD_Set_Light(ui_LightSliderValue);
		}

		/* 【调试用 - 已注释】让屏幕常亮、不自动休眠。
		 * 原逻辑：空闲超过 ui_LTimeValue 秒 → 背光调暗；
		 *          空闲超过 ui_TTimeValue 秒 → 进入 STOP 低功耗休眠。
		 * 试验完记得恢复下面两段 if 代码！
		 */
		// if((!light_off_sent) && (IdleTimerCount >= ((uint32_t)ui_LTimeValue * 1000U)))
		// {
		// 	uint8_t IdleMsg = 0;
		// 	osMessageQueuePut(Idle_MessageQueue, &IdleMsg, 0, 0);
		// }
		//
		// if(IdleTimerCount >= ((uint32_t)ui_TTimeValue * 1000U))
		// {
		// 	uint8_t Stopstr = 1;
		// 	IdleTimerCount = 0;
		// 	light_off_sent = 0;
		// 	osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 0);
		// }

		osDelay(10);
	}
}

/**
  * @brief  enter the stop mode and resume
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 休眠/唤醒任务 ====================
 * 收到 Stop_MessageQueue 消息后，让芯片进入 STOP 低功耗模式：
 *   1. 休眠前：关闭 UART、LCD、触摸、看门狗等外设
 *   2. 进入 STOP 模式（HAL_PWR_EnterSTOPMode）
 *   3. 被唤醒后：恢复时钟、重新初始化外设、回到运行状态
 * 注意：本任务是否被触发，取决于 IdleEnterTask 或 其他任务中 发的 Stop_MessageQueue，
 *       当前 IdleEnterTask 里的触发代码已被注释（调试屏幕常亮用）。
 */
void StopEnterTask(void *argument)
{
	uint8_t Stopstr;
	uint8_t HomeUpdataStr;
	uint8_t Wrist_Flag=0;
	while(1)
	{
		if(osMessageQueueGet(Stop_MessageQueue,&Stopstr,NULL,0)==osOK)
		{

			/****************** 睡眠之前的操作 *****************************/
			/****************** 关「业务外设」（串口、LCD、触摸） **********/
			sleep:                   
			IdleTimerCount = 0;       // 清零空闲计时

			//sensors

			//usart
			HAL_UART_MspDeInit(&huart1);    // 关掉串口（蓝牙）

			//lcd
			LCD_RES_Clr();
			LCD_Close_Light();
			//touch
			CST816_Sleep();               // 触摸屏进入睡眠
			
			/****************************** 进入睡眠 *****************************/
			/***********关「系统级」+ 真正进休眠（看门狗、systick、STOP）*********/

			vTaskSuspendAll();         // 暂停所有任务（临界区，防止被打断）
			
			WDOG_Disnable();           // 关看门狗（休眠时不能喂狗）
			
			CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);  // 关 systick 中断
			//enter stop mode
			HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON,PWR_STOPENTRY_WFI); // ← 进入 STOP，MCU 停住
			
			// 被唤醒后从这里继续往下跑
			//here is the sleep period
			
			/***********************************************************************************/
			/****************************** quit wakeup operations *****************************/

			//resume run mode and reset the sysclk
			// 被唤醒 —— 恢复系统
			SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);      // 恢复 systick 中断
			HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));  // 重配 systick
			SystemClock_Config();             // 重配主时钟（STOP 模式会关掉 PLL/HSE）
			WDOG_Feed();                      // 喂狗
			xTaskResumeAll();                 // 恢复所有任务

			/***********************************************************************************/

			/****************************** your wakeup operations *****************************/

			//MPU Check
			//抬腕亮屏动作的判断，判断是抬手还放手
			if(HWInterface.IMU.wrist_is_enabled)
			{
				uint8_t hor;
				hor = MPU_isHorizontal();
				if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)  // 水平 且 之前是放下
				{
					HWInterface.IMU.wrist_state = WRIST_UP;
					Wrist_Flag = 1;      // 标记"抬腕了，该亮屏"
					// 继续执行下面的代码
				}
				else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)  // 不水平 且 之前是抬起
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
					IdleTimerCount  = 0;
					goto sleep;          // ← 放腕了，跳回 sleep 继续睡
				}
			}

			//
			if(!KEY1 || KEY2 || ChargeCheck() || Wrist_Flag)
			{
				Wrist_Flag = 0;
				//有唤醒原因，继续执行代码
			}
			else
			{
				IdleTimerCount  = 0;
				goto sleep;    // ← 都没唤醒原因，跳回 sleep 继续睡
			}

			
			// 真正亮屏 —— 恢复外设
			//usart
			HAL_UART_MspInit(&huart1);
			//lcd
			LCD_Init();
			LCD_Set_Light(ui_LightSliderValue);
			//touch
			CST816_Wakeup();
			//check if is Charging
			if(ChargeCheck())
			{osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_CHARG);}
			//send the Home Updata message
			osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1); // 通知首页刷新数据

			/**************************************************************************************/

		}
		osDelay(100);
	}
}

