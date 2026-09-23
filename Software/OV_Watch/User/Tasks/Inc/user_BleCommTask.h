#ifndef __USER_BLECOMMTASK_H__
#define __USER_BLECOMMTASK_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void BleCommTask(void *argument);

// 蓝牙开关状态（UI 蓝牙按钮控制，BLE 任务读取）
extern uint8_t g_ble_on;       // 1=蓝牙开启（采集进行中） 0=关闭
extern uint8_t g_send_enable;  // 1=周期发送中 0=暂停（$GET/$END 控制）

// 睡眠相关全局变量（临时：由 BLE 任务读取，以后睡眠检测算法填充）
extern uint8_t g_sleep_flag;   // 0=清醒 1=准备入睡 2=睡眠
extern float   g_sleep_time;   // 睡眠时长（小时）


#ifdef __cplusplus
}
#endif

#endif

