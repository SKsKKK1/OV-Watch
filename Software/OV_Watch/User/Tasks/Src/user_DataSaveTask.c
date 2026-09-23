/* ============================================================
 * 数据保存任务（Data Save Task）
 * ------------------------------------------------------------
 * 把需要"断电不丢失"的数据保存到 EEPROM（BL24C02 存储芯片）。
 *
 * 保存两类数据：
 *   1. 用户设置：抬腕唤醒开关、APP 设置开关（地址 0x10 起）
 *   2. 计步数据：当天日期 + 当天步数（地址 0x20 起）
 *
 * 工作方式：平时休眠，收到 DataSave_MessageQueue 消息后才执行一次保存。
 * 触发时机：设置改变、日期改变、步数改变时。
 * 具体地址含义见下方"EEPROM 存储布局"。
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_DataSaveTask.h"
//APP SYS setting
#include "ui_DateTimeSetPage.h"
#include "ui_HomePage.h"

#include "main.h"
#include "rtc.h"
#include "DataSave.h"
#include "inv_mpu_dmp_motion_driver.h"

#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* ==================== EEPROM 存储布局 ====================
 * EEPROM（BL24C02，掉电不丢失的存储芯片）里各地址存的数据：
 *   门牌号
 *   [0x00] : 0x55 —— 校验标志 1（判断 EEPROM 是否初始化过）
 *   [0x01] : 0xAA —— 校验标志 2
 *
 *   [0x10] : 抬腕唤醒设置（HWInterface.IMU.wrist_is_enabled）
 *   [0x11] : APP 设置开关（ui_APPSy_EN）
 *
 *   [0x20] : 上次保存的日期（0-31）
 *   [0x21] : 当天步数的高字节
 *   [0x22] : 当天步数的低字节
 * ======================================================== */


/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
  * @brief  Data Save in the EEPROM
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 数据保存循环 ====================
 * 无限循环等待 DataSave_MessageQueue 消息，收到后执行一次保存：
 *
 *   1. 保存用户设置：抬腕开关 + APP 开关 → 存到 EEPROM 0x10 起 2 字节
 *
 *   2. 处理计步数据（带"跨天清零"逻辑）：
 *      - 读 EEPROM 里上次保存的日期
 *      - 如果和今天日期不同（说明跨天了）→ 计步清零，重新开始记
 *      - 如果是同一天 → 把当前步数写进 EEPROM（断电不丢）
 *
 * 说明：SettingSave / SettingGet 是 EEPROM 读写封装（见 DataSave.h），
 *       参数分别是 (数据数组, 起始地址, 字节长度)。
 */
void DataSaveTask(void *argument)
{

	while(1)
	{
		uint8_t Datastr=0;
		if(osMessageQueueGet(DataSave_MessageQueue,&Datastr,NULL,1)==osOK)
		{
			/****************
			Setting change
			date change
			Step change
			****************/
			uint8_t dat[3];                             // 3字节缓冲区，给 EEPROM 读写用
			dat[0] = HWInterface.IMU.wrist_is_enabled;  // 抬腕唤醒开关（0/1）
			dat[1] = ui_APPSy_EN;                       // APP 设置开关
			SettingSave(dat,0x10,2);                    // 存到 EEPROM 地址 0x10 起 2 字节

			//把两个设置值装进 dat，一次性写进 EEPROM。对应布局里的 [0x10] 和 [0x11]。
			
			RTC_DateTypeDef nowdate;     // 从 RTC 读今天的日期
			HAL_RTC_GetDate(&hrtc,&nowdate,RTC_FORMAT_BIN);

			SettingGet(dat,0x20,3);      // 读 0x20 起 3 字节：dat[0]=日期, 
			                             // dat[1]=步数高字节, dat[2]=步数低字节
			if(dat[0] != nowdate.Date)
			{
				if(!HWInterface.IMU.ConnectionError)   // 计步传感器正常才清零
					HWInterface.IMU.SetSteps(0);       // 步数清零，重新记今天

				dat[0] = nowdate.Date;
				dat[2] = 0;
				dat[1] = 0;
				SettingSave(dat,0x20,3);               // 存进 EEPROM
			}
			else
			{
				uint16_t temp = HWInterface.IMU.GetSteps();
				dat[0] = nowdate.Date;
				dat[2] = temp & 0xff;
				dat[1] = temp>>8 & 0xff;
				SettingSave(dat,0x20,3);
			}

		}
		osDelay(100);
	}
}


