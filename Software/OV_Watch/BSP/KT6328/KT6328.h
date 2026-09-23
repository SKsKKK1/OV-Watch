#ifndef __KT6328_H__
#define __KT6328_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void KT6328_GPIO_Init(void);
void KT6328_Enable(void);
void KT6328_Disable(void);
void KT6328_SendATCmd(const char *cmd);
void KT6328_SendText(const char *cmd, const char *params);

/* BLE 文本帧指令码 */
#define BLE_CMD_PING "PING"
#define BLE_CMD_BAT  "BAT"
#define BLE_CMD_VOL  "VOL"
#define BLE_CMD_LED  "LED"
#define BLE_CMD_TIME "TIME"
#define BLE_CMD_REQ  "REQ"


#ifdef __cplusplus
}
#endif
#endif

