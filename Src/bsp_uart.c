/*
 * bsp_uart.c
 *
 *  Created on: Aug 26, 2026
 *      Author: berkd
 */

#include "bsp_uart.h"

static USART_Handle_t usart2_handle;

static void BSP_UART2_GPIOInit(void)
{
    GPIO_Handle_t usart_gpios;

    usart_gpios.pGPIOx = GPIOA;
    usart_gpios.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTFN;
    usart_gpios.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_PP;
    usart_gpios.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU;
    usart_gpios.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_FAST;
    usart_gpios.GPIO_PinConfig.GPIO_PinAltFunMode = 7;

    // PA2: TX
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_2;
    GPIO_Init(&usart_gpios);

    // PA3: RX
    usart_gpios.GPIO_PinConfig.GPIO_PinNumber = GPIO_PIN_NO_3;
    GPIO_Init(&usart_gpios);
}

void BSP_UART2_Init(void)
{
    BSP_UART2_GPIOInit();

    usart2_handle.pUSARTx = USART2;
    usart2_handle.USART_Config.USART_Baud = USART_STD_BAUD_115200;
    usart2_handle.USART_Config.USART_HWFlowControl = USART_HW_FLOW_CTRL_NONE;
    usart2_handle.USART_Config.USART_Mode = USART_MODE_TXRX;
    usart2_handle.USART_Config.USART_NoOfStopBits = USART_STOPBITS_1;
    usart2_handle.USART_Config.USART_WordLength = USART_WORDLEN_8BITS;
    usart2_handle.USART_Config.USART_ParityControl = USART_PARITY_DISABLE;

    USART_Init(&usart2_handle);
    USART_PeripheralControl(USART2, ENABLE);
}

void BSP_UART2_Write(uint8_t *pBuffer, uint32_t len)
{
    USART_SendData(&usart2_handle, pBuffer, len);
}
