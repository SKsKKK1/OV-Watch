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

/* Private define ------------------------------------------------------------*/
#define BLE_RX_BUF_SIZE 128

/* Private variables ---------------------------------------------------------*/
// 累积接收缓冲区：把每次串口收到的字节攒起来，用来应对一帧被拆成几包(半包)的情况
static uint8_t ble_rx_buf[BLE_RX_BUF_SIZE];
static uint16_t ble_rx_len = 0;   // 累积缓冲区里当前有效数据的长度

/* Private function prototypes -----------------------------------------------*/
static void BLE_HandlePing(char *content);
static void BLE_HandleVolume(char *content);
static void BLE_HandleLed(char *content);
static void BLE_DispatchCmd(char *content);
static void BLE_TestSendTick(void);

/* 命令分派：content 是 $ 和 \r\n 之间的内容，格式 = 指令码 或 指令码,参数 */
static void BLE_DispatchCmd(char *content)
{
    if (strncmp(content, "PING", 4) == 0)
        BLE_HandlePing(content);
    else if (strncmp(content, "VOL", 3) == 0)
        BLE_HandleVolume(content);
    else if (strncmp(content, "LED", 3) == 0)
        BLE_HandleLed(content);
    // BAT/TIME 是手表主动发给音箱的，手表不会收到，忽略
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

/* 测试用：每秒主动发一帧 $PING,递增序号（临时，测试完删） */
static void BLE_TestSendTick(void)
{
    static uint8_t count = 0;
    char params[8];
    sprintf(params, "%d", count);
    KT6328_SendText("PING", params);
    count++;
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
        // 等接收事件，最多等 1000ms（超时则走到下面发测试帧，实现每秒发一次）
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

        // 每秒发一次测试帧（收到数据或超时都会走到这里）
        BLE_TestSendTick();
    }
}
