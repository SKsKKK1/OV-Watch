/* ============================================================
 * BLE 通信任务（BLE Communication Task）
 * ------------------------------------------------------------
 * 负责手表与音箱之间的蓝牙数据收发：
 *
 *   【接收】音箱 → 手表：
 *     $GET,DATA   请求传感器数据（音箱进入数据页面时发）
 *     $END        暂停发送（音箱退出页面/休眠时发）
 *     $PING,序号  心跳测试
 *     $VOL,xx     音量同步（TODO）
 *     $LED,R,G,B  灯控（TODO）
 *
 *   【发送】手表 → 音箱：
 *     $DATA,...   传感器数据帧
 *
 *   【工作流程】
 *     用户点蓝牙按钮开蓝牙 → 开始采集（每 1 分钟采一次）
 *     音箱发 $GET,DATA → 立即发一帧 + 之后每 2 分钟周期发
 *     音箱发 $END → 暂停发送
 *     用户点蓝牙按钮关蓝牙 → 停止采集 + 停止发送
 *
 * 帧格式：$指令码,参数\r\n
 * ============================================================ */

/* Private includes -----------------------------------------------------------*/
#include "string.h"
#include "stdio.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "rtc.h"

#include "user_TasksInit.h"
#include "user_BleCommTask.h"

#include "HWDataAccess.h"
#include "kt6328.h"
#include "SPL06_001.h"

/* Private define ------------------------------------------------------------*/
#define BLE_RX_BUF_SIZE     128
#define COLLECT_PERIOD_SEC  60    // 采集周期：1 分钟
#define SEND_PERIOD_SEC     120   // 发送周期：2 分钟

/* Private typedef -----------------------------------------------------------*/
// 传感器数据快照结构体：每采集一次，把最新值填进来（覆盖旧的）
typedef struct
{
    float    temp;    // 温度 ℃
    float    humi;    // 湿度 %
    uint8_t  hr;      // 心率 次/分
    uint8_t  spo2;    // 血氧 %（暂未实现，占位 0）
    uint16_t step;    // 步数
    float    pres;    // 气压 hPa
    uint8_t  sleep;   // 睡眠标志 0=清醒 1=准备入睡 2=睡眠
    float    stime;   // 睡眠时长 小时
} SensorData_t;

/* Private variables ---------------------------------------------------------*/
// 累积接收缓冲区：把每次串口收到的字节攒起来，应对一帧被拆成几包(半包)的情况
static uint8_t ble_rx_buf[BLE_RX_BUF_SIZE];
static uint16_t ble_rx_len = 0;   // 累积缓冲区里当前有效数据的长度

// 全局状态变量（UI 层和其他任务共享）
uint8_t g_ble_on = 0;        // 蓝牙是否开启（1=开，采集进行中）
uint8_t g_send_enable = 0;   // 发送使能（1=周期发送中）
uint8_t g_sleep_flag = 0;    // 睡眠标志（临时，以后睡眠检测算法填充）
float   g_sleep_time = 0.0f; // 睡眠时长（小时，临时）

// 本文件内部状态
static SensorData_t g_sensor;           // 传感器数据快照
static uint32_t     g_collect_tick = 0; // 采集计时（秒）
static uint32_t     g_send_tick = 0;    // 发送计时（秒）
static uint8_t      g_last_ble_on = 0;  // 上一次蓝牙开关状态（用于检测"刚开启"的上升沿）

/* Private function prototypes -----------------------------------------------*/
static void BLE_HandlePing(char *content);
static void BLE_HandleGet(char *content);
static void BLE_HandleEnd(char *content);
static void BLE_HandleVolume(char *content);
static void BLE_HandleLed(char *content);
static void BLE_DispatchCmd(char *content);
static void BLE_CollectSensorData(void);
static void BLE_SendSensorData(void);

/* 命令分派：content 是 $ 和 \r\n 之间的内容，格式 = 指令码 或 指令码,参数 */
static void BLE_DispatchCmd(char *content)
{
    if (strncmp(content, "PING", 4) == 0)
        BLE_HandlePing(content);
    else if (strncmp(content, "GET", 3) == 0)
        BLE_HandleGet(content);
    else if (strncmp(content, "END", 3) == 0)
        BLE_HandleEnd(content);
    else if (strncmp(content, "VOL", 3) == 0)
        BLE_HandleVolume(content);
    else if (strncmp(content, "LED", 3) == 0)
        BLE_HandleLed(content);
    // DATA 是手表主动发给音箱的，手表不会收到，忽略
}

/* PING 心跳：回显同样的序号，验证双向连通 */
static void BLE_HandlePing(char *content)
{
    char *params = strchr(content, ',');
    if (params != NULL)
        KT6328_SendText("PING", params + 1);   // 回 $PING,序号
    else
        KT6328_SendText("PING", NULL);         // 回 $PING
}

/* GET：音箱进入数据页面时发 $GET,DATA，请求传感器数据 */
static void BLE_HandleGet(char *content)
{
    g_send_enable = 1;        // 进入周期发送状态
    g_send_tick = 0;          // 重置发送计时
    BLE_SendSensorData();     // 立即发一帧
}

/* END：音箱退出页面/休眠时发 $END，暂停发送 */
static void BLE_HandleEnd(char *content)
{
    g_send_enable = 0;        // 暂停发送
}

/* VOL 音量同步：解析并存储音量值（后续接 UI 显示） */
static void BLE_HandleVolume(char *content)
{
    // TODO: 解析逗号后的音量值 0~100，存储供 UI 显示
    // content 形如 "VOL,60"
}

/* LED 灯控：解析 R,G,B（后续接灯控） */
static void BLE_HandleLed(char *content)
{
    // TODO: 解析 "LED,R,G,B"，控制 LED
}

/* 采集传感器数据 → 填入 g_sensor 快照（新数据覆盖旧数据） */
static void BLE_CollectSensorData(void)
{
    // 温湿度：主动采集（一次 I2C 读两个值）
    if (!HWInterface.AHT21.ConnectionError)
    {
        float h = 0, t = 0;
        HWInterface.AHT21.GetHumiTemp(&h, &t);
        if (t > -10.0f && t < 50.0f && h > 0.0f && h < 100.0f)
        {
            g_sensor.temp = t;
            g_sensor.humi = h;
        }
        else
        {
            g_sensor.temp = 0;   // 数值不合理 → 置 0 表示无效
            g_sensor.humi = 0;
        }
    }
    else
    {
        g_sensor.temp = 0;
        g_sensor.humi = 0;
    }

    // 心率：读缓存（后台 HRDataUpdateTask 持续测的最新值）
    if (!HWInterface.HR_meter.ConnectionError)
        g_sensor.hr = HWInterface.HR_meter.HrRate;
    else
        g_sensor.hr = 0;

    // 血氧：暂未实现，固定占位 0
    g_sensor.spo2 = 0;

    // 步数：主动读（更新并返回最新步数）
    if (!HWInterface.IMU.ConnectionError)
        g_sensor.step = HWInterface.IMU.GetSteps();
    else
        g_sensor.step = 0;

    // 气压：主动采集，Pa(帕) 换算成 hPa(百帕)
    if (!HWInterface.Barometer.ConnectionError)
    {
        SPL_Wakeup();
        float pa = Pressure_Calculate();
        float hpa = pa / 100.0f;
        if (hpa > 300.0f && hpa < 1100.0f)
            g_sensor.pres = hpa;
        else
            g_sensor.pres = 0;
    }
    else
    {
        g_sensor.pres = 0;
    }

    // 睡眠标志/时长：读全局变量（临时，以后睡眠检测算法填充）
    g_sensor.sleep = g_sleep_flag;
    g_sensor.stime = g_sleep_time;
}

/* 组帧 $DATA 并发送 */
static void BLE_SendSensorData(void)
{
    char params[96];
    sprintf(params, "T=%.1f,H=%.0f,HR=%u,SPO2=%u,STEP=%u,P=%.1f,SLEEP=%u,ST=%.1f",
            g_sensor.temp, g_sensor.humi,
            (unsigned int)g_sensor.hr, (unsigned int)g_sensor.spo2,
            (unsigned int)g_sensor.step, g_sensor.pres,
            (unsigned int)g_sensor.sleep, g_sensor.stime);
    KT6328_SendText("DATA", params);
}

/* 睡眠标志同步：UI 滑块改变时调用，只更新快照（不立即发送）
 * 发送交给周期任务，睡眠标志随下一帧 $DATA 一起发出 */
void BLE_SyncSleepFlag(void)
{
    g_sensor.sleep = g_sleep_flag;   // 只更新睡眠标志快照
}

/**
  * @brief  BLE 帧收发处理任务
  * @param  argument: Not used
  * @retval None
  */
void BleCommTask(void *argument)
{
    while(1)
    {
        // 等接收事件，最多等 1000ms（超时则走下面做采集/发送计时）
        uint32_t flags = osEventFlagsWait(HardIntEventHandle, HARDINT_EVENT_UART, osFlagsWaitAny, 1000);

        // 收到数据才处理（超时返回 osFlagsErrorTimeout=0xFFFFFFFF，要排除，否则会误判成收到数据）
        if ((flags & HARDINT_EVENT_UART) != 0U && flags != 0xFFFFFFFFu)
        {
            // 有数据进来，通知系统活跃
            uint8_t IdleBreakstr = 0;
            osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, NULL, 1);

            // 把本次串口收到的数据，追加到累积缓冲区
            if (HardInt_receive_len > 0)     // >0 才处理，挡住上电时那次空触发(长度为0)
            {
                if (ble_rx_len + HardInt_receive_len <= BLE_RX_BUF_SIZE)
                {
                    // 拼到累积缓冲区末尾
                    memcpy(&ble_rx_buf[ble_rx_len], HardInt_receive_str, HardInt_receive_len);
                    ble_rx_len += HardInt_receive_len;
                }
                else
                {
                    ble_rx_len = 0;  // 缓冲区满了，丢旧数据重新来
                }
                memset(HardInt_receive_str, 0, sizeof(HardInt_receive_str));  // 清空，准备收下一批
                HardInt_receive_len = 0;
            }

            // 循环解析累积缓冲区里的文本帧
            while (ble_rx_len > 0)
            {
                // 找帧头 $（0x24）
                if (ble_rx_buf[0] != '$')
                {
                    // 不是帧头，丢弃 1 字节继续找
                    memmove(ble_rx_buf, &ble_rx_buf[1], ble_rx_len - 1);
                    ble_rx_len--;
                    continue;
                }

                // 找帧尾 \r\n
                int crlf_pos = -1;
                for (uint16_t i = 1; i < ble_rx_len - 1; i++)
                {
                    if (ble_rx_buf[i] == '\r' && ble_rx_buf[i + 1] == '\n')
                    {
                        crlf_pos = i;
                        break;
                    }
                }

                if (crlf_pos < 0)
                    break;  // 还没收到完整帧尾，等下一包

                // 提取 $ 和 \r\n 之间的内容（指令码和参数）
                uint16_t content_len = crlf_pos - 1;
                char content[32];
                if (content_len >= sizeof(content))
                    content_len = sizeof(content) - 1;  // 防溢出
                memcpy(content, &ble_rx_buf[1], content_len);
                content[content_len] = '\0';

                // 分派处理
                BLE_DispatchCmd(content);

                // 把这一帧（含 \r\n）移出缓冲区
                uint16_t frame_len = crlf_pos + 2;
                memmove(ble_rx_buf, &ble_rx_buf[frame_len], ble_rx_len - frame_len);
                ble_rx_len -= frame_len;
            }
        }

        // 采集：蓝牙刚开启时立即采一次，之后每 1 分钟采一次
        if (g_ble_on)
        {
            if (!g_last_ble_on)
            {
                // 上升沿：蓝牙刚开启，立即采一次（避免音箱发 $GET 时拿到全 0 数据）
                BLE_CollectSensorData();
                g_collect_tick = 0;
            }
            else
            {
                g_collect_tick++;
                if (g_collect_tick >= COLLECT_PERIOD_SEC)
                {
                    BLE_CollectSensorData();
                    g_collect_tick = 0;
                }
            }
        }
        g_last_ble_on = g_ble_on;

        // 发送计时：每 2 分钟发一次（仅发送使能时）
        if (g_send_enable)
        {
            g_send_tick++;
            if (g_send_tick >= SEND_PERIOD_SEC)
            {
                BLE_SendSensorData();
                g_send_tick = 0;
            }
        }
    }
}
