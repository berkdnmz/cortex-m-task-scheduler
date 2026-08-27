/*
 * bsp_uart.h
 *
 *  Created on: Aug 26, 2026
 *      Author: berkd
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_

#include "stm32f446xx.h"

void BSP_UART3_Init(void);
void BSP_UART3_Write(uint8_t *pBuffer, uint32_t len);

#endif /* BSP_UART_H_ */
