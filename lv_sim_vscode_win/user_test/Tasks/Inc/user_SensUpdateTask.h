#ifndef __USER_SENSUPDATETASK_H__
#define __USER_SENSUPDATETASK_H__

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t user_HR_timecount;
extern uint8_t g_hr_measure;   // 心率检测开关（睡眠页开关控制，1=检测中）

void SensorDataUpdateTask(void *argument);
void HRDataUpdateTask(void *argument);
void MPUCheckTask(void *argument);

#ifdef __cplusplus
}
#endif

#endif

