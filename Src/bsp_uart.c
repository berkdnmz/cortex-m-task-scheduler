/*
 * bsp_uart.c
 *
 *  Created on: Aug 26, 2026
 *      Author: berkd
 */

#include "bsp_uart.h"

static USART_Handle_t usart3_handle;

static void BSP_UART3_GPIOInit(void)
{
    GPIO_Handle_t usart_gpios;

    usart_gpios.pGPIOx = GPIOB;
    usart_gpios.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    usart_gpios.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    usart_gpios.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    usart_gpios.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    usart_gpios.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    // PB10: TX
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_10;
    GPIO_Init(&usart_gpios);

    // PB11: RX
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_11;
    GPIO_Init(&usart_gpios);
}

void BSP_UART3_Init(void)
{
    BSP_UART3_GPIOInit();

    usart3_handle.pUSARTx = USART3;
    usart3_handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
    usart3_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
    usart3_handle.USART_Config.USART_Mode = USART_MODE_TXRX;
    usart3_handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
    usart3_handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
    usart3_handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;

    USART_Init(&usart3_handle);
    USART_PeripheralControl(USART3, ENABLE);
}

void BSP_UART3_Write(uint8_t *pBuffer, uint32_t len)
{
    USART_SendData(&usart3_handle, pBuffer, len);
}

int __io_putchar(int ch)
{
    BSP_UART3_Write((uint8_t *)&ch, 1);
    return ch;
}

int _write(int file, char *ptr, int len)
{
    BSP_UART3_Write((uint8_t *)ptr, len);
    return len;
}
