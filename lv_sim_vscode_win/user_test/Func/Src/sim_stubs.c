#include <stdint.h>

/* 仿真环境桩文件
 * 这些变量在 Keil 工程里定义在 BLE 任务（user_BleCommTask.c）中，
 * 但仿真器没有 FreeRTOS / Tasks 层，所以在这里提供同名的全局变量定义，
 * 让 ui_HomePage.c 等引用它们的 UI 代码能链接通过。
 */
uint8_t g_ble_on = 0;
uint8_t g_send_enable = 0;
uint8_t g_sleep_flag = 0;
uint8_t g_hr_measure = 0;
float   g_sleep_time = 0.0f;

/* 睡眠标志实时同步：真机在 user_BleCommTask.c 里实现（更新快照+发送），
 * 仿真环境无蓝牙硬件，这里提供空实现，仅保证 ui_SleepPage.c 能链接通过。 */
void BLE_SyncSleepFlag(void)
{
}
