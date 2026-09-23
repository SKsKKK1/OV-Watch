/* ============================================================
 * 传感器更新任务（Sensor Update Tasks）
 * ------------------------------------------------------------
 * 本文件包含三个跟"传感器"有关的 FreeRTOS 任务：
 *
 *   1. MPUCheckTask         —— 抬腕检测：判断手表是"抬起来"还是"放下"
 *   2. HRDataUpdateTask     —— 心率更新：在心率页测量并更新心率值
 *   3. SensorDataUpdateTask —— 传感器数据更新：更新首页的电量/步数/温湿度，
 *                              以及环境页/指南针页的实时数据
 *
 * 用到的传感器：
 *   MPU6050(计步/姿态)、EM7028(心率)、AHT21(温湿度)、
 *   LSM303(指南针)、SPL06(气压计)
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_SensUpdateTask.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"
#include "ui_SetPage.h"
#include "ui_HRPage.h"
#include "ui_SPO2Page.h"
#include "ui_ENVPage.h"
#include "ui_CompassPage.h"
#include "main.h"

#include "AHT21.h"
#include "LSM303.h"
#include "SPL06_001.h"
#include "em70x8.h"
#include "HrAlgorythm.h"

#include "HWDataAccess.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t user_HR_timecount=0;

/* Private function prototypes -----------------------------------------------*/
/* ==================== EM7028 官方库函数声明 ====================
 * "lib库"是什么：EM7028 心率传感器厂商把核心算法（心率/血压/血氧计算）
 * 编译成了 .lib 静态库文件提供，但没有公开源码（保护知识产权）。
 *
 * 我们能做的：通过下面 extern 声明这些函数，在代码里调用它们；
 * 不能做的：看不到也改不了它们的内部实现。
 *
 * 下面这些就是 .lib 里导出的函数接口（血压、血氧、心率算法）。
 */
extern uint8_t GET_BP_MAX (void);
extern uint8_t GET_BP_MIN (void);
extern void Blood_Process (void);
extern void Blood_50ms_process (void);
extern void Blood_500ms_process(void);
extern int em70xx_bpm_dynamic(int RECEIVED_BYTE, int g_sensor_x, int g_sensor_y, int g_sensor_z);
extern int em70xx_reset(int ref);


/**
  * @brief  MPU6050 Check the state
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 抬腕检测任务 ====================
 * 每 300ms 检查一次 MPU6050 的姿态（是否水平 = 是否抬起手腕）。
 *
 * 逻辑：如果开启了"抬腕唤醒"，判断手表当前是"抬起"还是"放下"，并置状态，供RunMode里判断手腕瞬间状态使用。
 *   当从"抬起"变为"放下"，且当前在首页/菜单/设置页时，
 *   发 Stop_MessageQueue 让手表进入休眠（省电）。
 *   
 */
void MPUCheckTask(void *argument)
{
	while(1)
	{
		if(HWInterface.IMU.wrist_is_enabled)
		{
			if(MPU_isHorizontal())   //是否水平
			{
				HWInterface.IMU.wrist_state = WRIST_UP;   //水平-->抬起手腕
			}
			else
			{
				if(WRIST_UP == HWInterface.IMU.wrist_state)  //抬起刚放下（这个瞬间）
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;   //手腕放下
					if( Page_Get_NowPage()->page_obj == &ui_HomePage ||
						Page_Get_NowPage()->page_obj == &ui_MenuPage ||
						Page_Get_NowPage()->page_obj == &ui_SetPage )   //如果处在这三个界面
					{
						uint8_t Stopstr;
						osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);//sleep休眠模式
					}
				}
				HWInterface.IMU.wrist_state = WRIST_DOWN;
			}
		}
		osDelay(300);
	}
}

/**
  * @brief  HR data renew task
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 心率更新任务 ====================
 * 当当前页面是心率页(ui_HRPage)时，每 50ms 执行一次：
 *   1. 唤醒心率传感器 EM7028
 *   2. 调用 EM7028 官方 lib 的心率算法(HR_Calculate)算出心率值
 *   3. 结果合理(50~120)时，更新 HWInterface.HR_meter.HrRate 供 UI 显示
 */
void HRDataUpdateTask(void *argument)
{
	uint8_t IdleBreakstr=0;
	uint16_t dat=0;
	uint8_t hr_temp=0;
	while(1)
	{
		if(Page_Get_NowPage()->page_obj == &ui_HRPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//sensor wake up
			EM7028_hrs_Enable();
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.HR_meter.ConnectionError)
			{
				//Hr messure
				vTaskSuspendAll();
				hr_temp = HR_Calculate(EM7028_Get_HRS1(),user_HR_timecount);
				xTaskResumeAll();
				if(HWInterface.HR_meter.HrRate != hr_temp && hr_temp>50 && hr_temp<120)
				{
					HWInterface.HR_meter.HrRate = hr_temp;
				}
			}
		}
		osDelay(50);
	}
}


/**
  * @brief  Sensor data update task
  * @param  argument: Not used
  * @retval None
  */
/* ==================== 传感器数据更新任务 ====================
 * 无限循环做两件事：
 *
 *   1. 收到 HomeUpdata_MessageQueue → 更新首页数据（电量/步数/温湿度），
 *      最后发 DataSave_MessageQueue 触发保存到 EEPROM。
 *
 *   2. 根据当前页面更新对应传感器数据：
 *      - 血氧页(SPO2)     ：暂未实现（if(0) 占位）
 *      - 环境页(Env)      ：更新温湿度
 *      - 指南针页(Compass)：更新方位角 + 海拔
 */
void SensorDataUpdateTask(void *argument)
{
	uint8_t value_strbuf[6];
	uint8_t IdleBreakstr=0;
	while(1)
	{
		// Update the sens data showed in Home
		uint8_t HomeUpdataStr;
		if(osMessageQueueGet(HomeUpdata_MessageQueue, &HomeUpdataStr, NULL, 0)==osOK)
		{
			//bat
			uint8_t value_strbuf[5];

			HWInterface.Power.power_remain = HWInterface.Power.BatCalculate();
			if(HWInterface.Power.power_remain>0 && HWInterface.Power.power_remain<=100)
			{}
			else
			{HWInterface.Power.power_remain = 0;}

			//steps
			if(!(HWInterface.IMU.ConnectionError))
			{
				HWInterface.IMU.Steps = HWInterface.IMU.GetSteps();
			}

			//temp and humi
			if(!(HWInterface.AHT21.ConnectionError))
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp);
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					// ui_EnvTempValue = (int8_t)temp;
					// ui_EnvHumiValue = (int8_t)humi;
					HWInterface.AHT21.humidity = humi;
					HWInterface.AHT21.temperature = temp;
				}
			}

			//send data save message queue
			uint8_t Datastr = 3;
			osMessageQueuePut(DataSave_MessageQueue, &Datastr, 0, 1);

		}


		// SPO2 Page
		if(Page_Get_NowPage()->page_obj == &ui_SPO2Page)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//sensor wake up

			//receive the sensor wakeup message, sensor wakeup
			if(0)
			{
				//SPO2 messure
			}
		}
		// Env Page
		else if(Page_Get_NowPage()->page_obj == &ui_EnvPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			if(!HWInterface.AHT21.ConnectionError)
			{
				//temp and humi messure
				float humi,temp;
				HWInterface.AHT21.GetHumiTemp(&humi,&temp);
				//check
				if(temp>-10 && temp<50 && humi>0 && humi<100)
				{
					HWInterface.AHT21.temperature = (int8_t)temp;
					HWInterface.AHT21.humidity = (int8_t)humi;
				}
			}

		}
		// Compass page
		else if(Page_Get_NowPage()->page_obj == &ui_CompassPage)
		{
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
			//receive the sensor wakeup message, sensor wakeup
			LSM303DLH_Wakeup();
			//SPL_Wakeup();
			//if the sensor is no problem
			if(!HWInterface.Ecompass.ConnectionError)
			{
				//messure
				int16_t Xa,Ya,Za,Xm,Ym,Zm;
				LSM303_ReadAcceleration(&Xa,&Ya,&Za);
				LSM303_ReadMagnetic(&Xm,&Ym,&Zm);
				float temp = Azimuth_Calculate(Xa,Ya,Za,Xm,Ym,Zm)+0;//0 offset
				if(temp<0)
				{temp+=360;}
				//check
				if(temp>=0 && temp<=360)
				{
					HWInterface.Ecompass.direction = (uint16_t)temp;
				}
			}
			//if the sensor is no problem
			if(!HWInterface.Barometer.ConnectionError)
			{
				//messure
				float alti = Altitude_Calculate();
				//check
				if(1)
				{
					HWInterface.Barometer.altitude = (int16_t)alti;
				}
			}
		}

		osDelay(500);
	}
}
