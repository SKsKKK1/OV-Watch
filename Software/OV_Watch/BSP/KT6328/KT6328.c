#include "KT6328.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

void KT6328_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

 // 发送一条 AT 指令（自动补 \r\n）
void KT6328_SendATCmd(const char *cmd)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 0xffff);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 0xffff);
}

 // 发送一条文本帧：$指令码,参数\r\n（参数为 NULL 或空串时发 $指令码\r\n）
void KT6328_SendText(const char *cmd, const char *params)
{
    char buf[32];
    int len;

    if (params != NULL && params[0] != '\0')
        len = sprintf(buf, "$%s,%s\r\n", cmd, params);   // 有参数
    else
        len = sprintf(buf, "$%s\r\n", cmd);              // 无参数

    HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 0xffff);  // 从串口发出，经蓝牙透传
}


void KT6328_Enable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}


void KT6328_Disable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

