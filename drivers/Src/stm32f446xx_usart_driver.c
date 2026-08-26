/*
 * stm32f446xx_usart_driver.c
 *
 *  Created on: Aug 17, 2026
 *      Author: berkd
 */

#include "stm32f446xx_usart_driver.h"

#include "stm32f446xx_usart_driver.h"

static uint16_t AHB_PreScaler[8] = {2, 4, 8, 16, 64, 128, 256, 512};
static uint8_t APB1_PreScaler[4] = {2, 4, 8, 16};
static uint8_t APB2_PreScaler[4] = {2, 4, 8, 16};


static uint32_t RCC_GetPCLK1Value(void);
static uint32_t RCC_GetPCLK2Value(void);

/*************************************************************************************************************************
 * @fn                  - USART_PeriClockControl
 *
 * @brief               - This function enables or disables the peripheral clock for the given USART/UART peripheral.
 *
 * @param[in]           - pUSARTx : Base address of the USART/UART peripheral (e.g., USART1, USART2, USART3, UART4, UART5, USART6).
 * @param[in]           - EnorDi  : ENABLE or DISABLE macro to control the clock.
 *
 * @return              - None.
 *
 * @Note                - None.
 *
 *************************************************************************************************************************/
void USART_PeriClockControl(USART_RegDef_t *pUSARTx, uint8_t EnorDi)
{
    if(EnorDi == ENABLE)
    {
        if(pUSARTx == USART1)
        {
            USART1_PCLK_EN();
        }
        else if(pUSARTx == USART2)
        {
            USART2_PCLK_EN();
        }
        else if(pUSARTx == USART3)
        {
            USART3_PCLK_EN();
        }
        else if(pUSARTx == UART4)
        {
            UART4_PCLK_EN();
        }
        else if(pUSARTx == UART5)
        {
            UART5_PCLK_EN();
        }
        else if(pUSARTx == USART6)
        {
            USART6_PCLK_EN();
        }
    }
    else
    {
        if(pUSARTx == USART1)
        {
            USART1_PCLK_DI();
        }
        else if(pUSARTx == USART2)
        {
            USART2_PCLK_DI();
        }
        else if(pUSARTx == USART3)
        {
            USART3_PCLK_DI();
        }
        else if(pUSARTx == UART4)
        {
            UART4_PCLK_DI();
        }
        else if(pUSARTx == UART5)
        {
            UART5_PCLK_DI();
        }
        else if(pUSARTx == USART6)
        {
            USART6_PCLK_DI();
        }
    }
}

/*********************************************************************
 * @fn                - USART_Init
 *
 * @brief             - Initializes the given USART/UART peripheral according to the specified parameters in USART_Config.
 *
 * @param[in]         - pUSARTHandle : Pointer to the USART_Handle_t structure containing configuration info.
 *
 * @return            - None.
 *
 * @Note              - None.
 *********************************************************************/
void USART_Init(USART_Handle_t *pUSARTHandle)
{
    // Temporary variable
    uint32_t tempreg = 0;

    /******************************** Configuration of CR1 ******************************************/

    // 1. Enable the Clock for given USART peripheral
    USART_PeriClockControl(pUSARTHandle->pUSARTx, ENABLE);

    // 2. Enable USART Tx and Rx engines according to the USART_Mode configuration item
    if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_RX)
    {
        tempreg |= (1 << USART_CR1_RE);
    }
    else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_ONLY_TX)
    {
        tempreg |= (1 << USART_CR1_TE);
    }
    else if (pUSARTHandle->USART_Config.USART_Mode == USART_MODE_TXRX)
    {
        tempreg |= ((1 << USART_CR1_RE) | (1 << USART_CR1_TE));
    }

    // 3. Configure the Word length configuration item
    tempreg |= (pUSARTHandle->USART_Config.USART_WordLength << USART_CR1_M);

    // 4. Configuration of parity control bit fields
    if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_EVEN)
    {
        tempreg |= (1 << USART_CR1_PCE);
        // By default, PS bit is 0 (EVEN parity)
    }
    else if (pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_EN_ODD)
    {
        tempreg |= (1 << USART_CR1_PCE);
        tempreg |= (1 << USART_CR1_PS);
    }

    // Program the CR1 register
    pUSARTHandle->pUSARTx->CR1 = tempreg;

    /******************************** Configuration of CR2 ******************************************/

    tempreg = 0;

    // Configure the number of stop bits inserted during USART frame transmission
    tempreg |= (pUSARTHandle->USART_Config.USART_NoOfStopBits << USART_CR2_STOP);

    // Program the CR2 register
    pUSARTHandle->pUSARTx->CR2 = tempreg;

    /******************************** Configuration of CR3 ******************************************/

    tempreg = 0;

    // Configuration of USART hardware flow control
    if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS)
    {
        tempreg |= (1 << USART_CR3_CTSE);
    }
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_RTS)
    {
        tempreg |= (1 << USART_CR3_RTSE);
    }
    else if (pUSARTHandle->USART_Config.USART_HWFlowControl == USART_HW_FLOW_CTRL_CTS_RTS)
    {
        tempreg |= ((1 << USART_CR3_CTSE) | (1 << USART_CR3_RTSE));
    }

    // Program the CR3 register
    pUSARTHandle->pUSARTx->CR3 = tempreg;

    /******************************** Configuration of BRR (Baudrate register) **************************/
    //Implement the code to configure the baud rate
	USART_SetBaudRate(pUSARTHandle->pUSARTx, pUSARTHandle->USART_Config.USART_Baud);

}

/*********************************************************************
 * @fn                 - USART_DeInit
 *
 * @brief              - Resets the specified USART peripheral registers using RCC reset registers
 *
 * @param[in]          - pUSARTx : Base address of the USART peripheral (USART1, USART2, etc.)
 *
 * @return             - None
 *
 * @Note               - Sets and clears the corresponding reset bit in RCC APBxRSTR registers
 */
void USART_DeInit(USART_RegDef_t *pUSARTx)
{
	if(pUSARTx == USART1)
	{
		USART1_REG_RESET();
	}
	else if(pUSARTx == USART2)
	{
		USART2_REG_RESET();
	}
	else if(pUSARTx == USART3)
	{
		USART3_REG_RESET();
	}
	else if(pUSARTx == UART4)
	{
		UART4_REG_RESET();
	}
	else if(pUSARTx == UART5)
	{
		UART5_REG_RESET();
	}
	else if(pUSARTx == USART6)
	{
		USART6_REG_RESET();
	}
}

/*********************************************************************
 * @fn                - USART_SendData
 *
 * @brief             - Sends data over USART in blocking/polling mode
 *
 * @param[in]         - pUSARTHandle : Pointer to USART_Handle_t structure
 * @param[in]         - pTxBuffer    : Pointer to user transmission data buffer
 * @param[in]         - Len          : Number of bytes to transfer
 *
 * @return            - None
 *********************************************************************/
void USART_SendData(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len)
{
    uint16_t *pdata;

    // Loop over until "Len" number of bytes are transferred
    for(uint32_t i = 0 ; i < Len; i++)
    {
        // 1. Wait until TXE flag is set in the SR
        while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_TXE));

        // 2. Check the USART_WordLength item for 9BIT or 8BIT in a frame
        if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
        {
            // If 9BIT, load the DR with 2 bytes masking the bits other than first 9 bits
            pdata = (uint16_t*) pTxBuffer;
            pUSARTHandle->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

            // Check for USART_ParityControl
            if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity: 9 bits of user data -> increment pointer twice
                pTxBuffer++;
                pTxBuffer++;
            }
            else
            {
                // Parity used: 8 bits user data + 1 bit HW parity
                pTxBuffer++;
            }
        }
        else
        {
            // This is 8-bit data transfer
            pUSARTHandle->pUSARTx->DR = (*pTxBuffer & (uint8_t)0xFF);

            // Increment the buffer address
            pTxBuffer++;
        }
    }

    // 3. Wait till TC flag is set in the SR before exiting
    while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_TC));
}

/*********************************************************************
 * @fn                - USART_ReceiveData
 *
 * @brief             - Receives data over USART in blocking/polling mode
 *
 * @param[in]         - pUSARTHandle : Pointer to USART_Handle_t structure
 * @param[in]         - pRxBuffer    : Pointer to user reception buffer
 * @param[in]         - Len          : Number of bytes to receive
 *
 * @return            - None
 *********************************************************************/
void USART_ReceiveData(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    // Loop over until "Len" number of bytes are transferred
    for(uint32_t i = 0 ; i < Len; i++)
    {
        // 1. Wait until RXNE flag is set in the SR
        while(! USART_GetFlagStatus(pUSARTHandle->pUSARTx, USART_FLAG_RXNE));

        // 2. Check the USART_WordLength (9BIT or 8BIT)
        if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
        {
            // We are going to receive 9bit data in a frame
            if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity: all 9 bits are user data
                *((uint16_t*) pRxBuffer) = (pUSARTHandle->pUSARTx->DR & (uint16_t)0x01FF);

                // Increment pointer twice
                pRxBuffer++;
                pRxBuffer++;
            }
            else
            {
                // Parity used: 8 bits user data, 1 bit parity
                *pRxBuffer = (pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
                pRxBuffer++;
            }
        }
        else
        {
            // We are going to receive 8bit data in a frame
            if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
            {
                // No parity: all 8 bits are user data
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
            }
            else
            {
                // Parity used: 7 bits user data + 1 bit parity
                *pRxBuffer = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0x7F);
            }

            // Increment the buffer address
            pRxBuffer++;
        }
    }
}

/*********************************************************************
 * @fn                - USART_SendDataIT
 *
 * @brief             - Initiates non-blocking transmission over USART using interrupts
 *
 * @param[in]         - pUSARTHandle : Pointer to USART_Handle_t structure
 * @param[in]         - pTxBuffer    : Pointer to user transmission data buffer
 * @param[in]         - Len          : Number of bytes to transfer
 *
 * @return            - Current transmission state (USART_READY or USART_BUSY_IN_TX)
 *********************************************************************/
uint8_t USART_SendDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pTxBuffer, uint32_t Len)
{
    uint8_t txstate = pUSARTHandle->TxBusyState;

    if(txstate != USART_BUSY_IN_TX)
    {
        pUSARTHandle->TxLen = Len;
        pUSARTHandle->pTxBuffer = pTxBuffer;
        pUSARTHandle->TxBusyState = USART_BUSY_IN_TX;

        // Implement the code to enable interrupt for TXE (CR1 -> TXEIE)
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_TXEIE);

        // Implement the code to enable interrupt for TC (CR1 -> TCIE)
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_TCIE);
    }

    return txstate;
}

/*********************************************************************
 * @fn                - USART_ReceiveDataIT
 *
 * @brief             - Initiates non-blocking reception over USART using interrupts
 *
 * @param[in]         - pUSARTHandle : Pointer to USART_Handle_t structure
 * @param[in]         - pRxBuffer    : Pointer to user reception buffer
 * @param[in]         - Len          : Number of bytes to receive
 *
 * @return            - Current reception state (USART_READY or USART_BUSY_IN_RX)
 *********************************************************************/
uint8_t USART_ReceiveDataIT(USART_Handle_t *pUSARTHandle, uint8_t *pRxBuffer, uint32_t Len)
{
    uint8_t rxstate = pUSARTHandle->RxBusyState;

    if(rxstate != USART_BUSY_IN_RX)
    {
        pUSARTHandle->RxLen = Len;
        pUSARTHandle->pRxBuffer = pRxBuffer;
        pUSARTHandle->RxBusyState = USART_BUSY_IN_RX;

        // Implement the code to enable interrupt for RXNE (CR1 -> RXNEIE)
        pUSARTHandle->pUSARTx->CR1 |= (1 << USART_CR1_RXNEIE);
    }

    return rxstate;
}

/*
 * IRQ Configuration and ISR handling
 */
/*********************************************************************
 * @fn      		  - USART_IRQInterruptConfig
 *
 * @brief             - Enables or disables the given IRQ number in the ARM Cortex NVIC registers
 *
 * @param[in]         - IRQNumber : NVIC IRQ position number
 * @param[in]         - EnorDi    : ENABLE or DISABLE
 *
 * @return            - None
 *********************************************************************/
void USART_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			// Program ISER0 register
			*NVIC_ISER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// Program ISER1 register
			*NVIC_ISER1 |= (1 << (IRQNumber % 32));
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// Program ISER2 register
			*NVIC_ISER2 |= (1 << (IRQNumber % 64));
		}
	}
	else
	{
		if(IRQNumber <= 31)
		{
			// Program ICER0 register
			*NVIC_ICER0 |= (1 << IRQNumber);
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// Program ICER1 register
			*NVIC_ICER1 |= (1 << (IRQNumber % 32));
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// Program ICER2 register
			*NVIC_ICER2 |= (1 << (IRQNumber % 64));
		}
	}
}

/*********************************************************************
 * @fn      		  - USART_IRQPriorityConfig
 *
 * @brief             - Configures the priority of the given IRQ number in NVIC IPR registers
 *
 * @param[in]         - IRQNumber   : NVIC IRQ position number
 * @param[in]         - IRQPriority : Priority value (0 to 15)
 *
 * @return            - None
 *********************************************************************/
void USART_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	// 1. Find the IPR register
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;

	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);

	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

/*********************************************************************
 * @fn                - USART_IRQHandling
 *
 * @brief             - Handles USART interrupts and manages TX/RX state machines
 *
 * @param[in]         - pUSARTHandle : Pointer to the USART_Handle_t structure
 *
 * @return            - None
 *********************************************************************/
void USART_IRQHandling(USART_Handle_t *pUSARTHandle)
{
    uint32_t temp1, temp2, temp3;
    uint16_t *pdata;

    /************************* Check for TC flag ********************************************/

    // 1. Check the state of TC bit in the SR
    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_TC);

    // 2. Check the state of TCIE bit in CR1
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1 << USART_CR1_TCIE);

    if(temp1 && temp2)
    {
        // This interrupt is caused by Transmission Complete (TC)
        if(pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
        {
            // Check the TxLen. If zero, close transmission
            if(pUSARTHandle->TxLen == 0)
            {
                // Clear the TC flag
                pUSARTHandle->pUSARTx->SR &= ~(1 << USART_SR_TC);

                // Clear the TCIE control bit (disable interrupt)
                pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_TCIE);

                // Reset the application state
                pUSARTHandle->TxBusyState = USART_READY;

                // Reset Buffer address to NULL
                pUSARTHandle->pTxBuffer = NULL;
                pUSARTHandle->TxLen = 0;

                // Call application callback
                USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_TX_CMPLT);
            }
        }
    }

    /************************* Check for TXE flag ********************************************/

    // 1. Check the state of TXE bit in the SR
    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_TXE);

    // 2. Check the state of TXEIE bit in CR1
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1 << USART_CR1_TXEIE);

    if(temp1 && temp2)
    {
        if(pUSARTHandle->TxBusyState == USART_BUSY_IN_TX)
        {
            if(pUSARTHandle->TxLen > 0)
            {
                // 9-Bit frame
                if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
                {
                    pdata = (uint16_t*) pUSARTHandle->pTxBuffer;
                    pUSARTHandle->pUSARTx->DR = (*pdata & (uint16_t)0x01FF);

                    if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        // 9-bit data: increment buffer twice and decrement length by 2
                        pUSARTHandle->pTxBuffer++;
                        pUSARTHandle->pTxBuffer++;
                        pUSARTHandle->TxLen -= 2;
                    }
                    else
                    {
                        // 8-bit data + 1 parity bit: increment buffer once and decrement length by 1
                        pUSARTHandle->pTxBuffer++;
                        pUSARTHandle->TxLen--;
                    }
                }
                else
                {
                    // 8-bit frame
                    pUSARTHandle->pUSARTx->DR = (*(pUSARTHandle->pTxBuffer) & (uint8_t)0xFF);
                    pUSARTHandle->pTxBuffer++;
                    pUSARTHandle->TxLen--;
                }
            }

            if(pUSARTHandle->TxLen == 0)
            {
                // Clear TXEIE bit to disable further TXE interrupts
                pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_TXEIE);
            }
        }
    }

    /************************* Check for RXNE flag ********************************************/

    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_RXNE);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1 << USART_CR1_RXNEIE);

    if(temp1 && temp2)
    {
        if(pUSARTHandle->RxBusyState == USART_BUSY_IN_RX)
        {
            if(pUSARTHandle->RxLen > 0)
            {
                // 9-Bit frame
                if(pUSARTHandle->USART_Config.USART_WordLength == USART_WORDLEN_9BITS)
                {
                    if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        // 9-bit user data
                        *((uint16_t*) pUSARTHandle->pRxBuffer) = (pUSARTHandle->pUSARTx->DR & (uint16_t)0x01FF);
                        pUSARTHandle->pRxBuffer++;
                        pUSARTHandle->pRxBuffer++;
                        pUSARTHandle->RxLen -= 2;
                    }
                    else
                    {
                        // 8-bit user data + 1 parity bit
                        *(pUSARTHandle->pRxBuffer) = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
                        pUSARTHandle->pRxBuffer++;
                        pUSARTHandle->RxLen--;
                    }
                }
                else
                {
                    // 8-Bit frame
                    if(pUSARTHandle->USART_Config.USART_ParityControl == USART_PARITY_DISABLE)
                    {
                        *(pUSARTHandle->pRxBuffer) = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0xFF);
                    }
                    else
                    {
                        *(pUSARTHandle->pRxBuffer) = (uint8_t)(pUSARTHandle->pUSARTx->DR & (uint8_t)0x7F);
                    }

                    pUSARTHandle->pRxBuffer++;
                    pUSARTHandle->RxLen--;
                }
            }

            if(pUSARTHandle->RxLen == 0)
            {
                // Disable RXNE interrupt
                pUSARTHandle->pUSARTx->CR1 &= ~(1 << USART_CR1_RXNEIE);
                pUSARTHandle->RxBusyState = USART_READY;
                USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_RX_CMPLT);
            }
        }
    }

    /************************* Check for CTS flag ********************************************/

    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_CTS);
    temp2 = pUSARTHandle->pUSARTx->CR3 & (1 << USART_CR3_CTSE);
    temp3 = pUSARTHandle->pUSARTx->CR3 & (1 << USART_CR3_CTSIE);

    if(temp1 && temp2 && temp3)
    {
        // Clear CTS flag
        pUSARTHandle->pUSARTx->SR &= ~(1 << USART_SR_CTS);

        // Notify application
        USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_CTS);
    }

    /************************* Check for IDLE detection flag ********************************************/

    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_IDLE);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1 << USART_CR1_IDLEIE);

    if(temp1 && temp2)
    {
        // Clear IDLE flag by reading SR followed by DR
        temp1 = pUSARTHandle->pUSARTx->SR;
        temp1 = pUSARTHandle->pUSARTx->DR;
        (void)temp1; // Suppress unused variable warning

        // Notify application
        USART_ApplicationEventCallback(pUSARTHandle, USART_EVENT_IDLE);
    }

    /************************* Check for Overrun detection flag ********************************************/

    temp1 = pUSARTHandle->pUSARTx->SR & (1 << USART_SR_ORE);
    temp2 = pUSARTHandle->pUSARTx->CR1 & (1 << USART_CR1_RXNEIE);

    if(temp1 && temp2)
    {
        // Notify application about ORE
        USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_ORE);
    }

    /************************* Check for Error Flag (Multibuffer / DMA mode) ********************************************/

    temp2 = pUSARTHandle->pUSARTx->CR3 & (1 << USART_CR3_EIE);

    if(temp2)
    {
        temp1 = pUSARTHandle->pUSARTx->SR;
        if(temp1 & (1 << USART_SR_FE))
        {
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_FE);
        }

        if(temp1 & (1 << USART_SR_NF))
        {
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_NE);
        }

        if(temp1 & (1 << USART_SR_ORE))
        {
            USART_ApplicationEventCallback(pUSARTHandle, USART_ERR_ORE);
        }
    }
}


/*
 * Other Peripheral Control APIs
 */
/*********************************************************************
 * @fn      		  - USART_PeripheralControl
 *
 * @brief             - Enables or disables the USART peripheral by setting/clearing CR1 UE bit
 *
 * @param[in]         - pUSARTx : Base address of the USART peripheral
 * @param[in]         - EnOrDi  : ENABLE or DISABLE
 *
 * @return            - None
 *********************************************************************/
void USART_PeripheralControl(USART_RegDef_t *pUSARTx, uint8_t EnOrDi)
{
	if(EnOrDi == ENABLE)
	{
		pUSARTx->CR1 |= (1 << USART_CR1_UE);
	}
	else
	{
		pUSARTx->CR1 &= ~(1 << USART_CR1_UE);
	}
}

/*********************************************************************
 * @fn      		  - USART_GetFlagStatus
 *
 * @brief             - Returns the status of the specified USART SR flag
 *
 * @param[in]         - pUSARTx : Base address of the USART peripheral
 * @param[in]         - FlagName : Flag to check (USART_FLAG_TXE, USART_FLAG_RXNE, etc.)
 *
 * @return            - FLAG_SET or FLAG_RESET
 *********************************************************************/
uint8_t USART_GetFlagStatus(USART_RegDef_t *pUSARTx, uint32_t FlagName)
{
	if(pUSARTx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*********************************************************************
 * @fn      		  - USART_ClearFlag
 *
 * @brief             - Clears the specified Status Register flag
 *
 * @param[in]         - pUSARTx        : Base address of the USART peripheral
 * @param[in]         - StatusFlagName : Flag to clear (e.g. USART_FLAG_CTS)
 *
 * @return            - None
 *********************************************************************/
void USART_ClearFlag(USART_RegDef_t *pUSARTx, uint16_t StatusFlagName)
{
	pUSARTx->SR &= ~(StatusFlagName);
}

/*********************************************************************
 * @fn      		  - USART_SetBaudRate
 *
 * @brief             - Calculates and programs the BRR register for the given Baud Rate
 *
 * @param[in]         - pUSARTx : Base address of the USART peripheral
 * @param[in]         - BaudRate : Desired baud rate value (e.g. 9600, 115200)
 *
 * @return            - None
 *********************************************************************/
void USART_SetBaudRate(USART_RegDef_t *pUSARTx, uint32_t BaudRate)
{
	//Variable to hold the APB clock
	uint32_t PCLKx;

	uint32_t usartdiv;

	//variables to hold Mantissa and Fraction values
	uint32_t M_part,F_part;

	uint32_t tempreg=0;

	//Get the value of APB bus clock in to the variable PCLKx
	if(pUSARTx == USART1 || pUSARTx == USART6)
	{
		//USART1 and USART6 are hanging on APB2 bus
		PCLKx = RCC_GetPCLK2Value();
	}else
	{
		PCLKx = RCC_GetPCLK1Value();
	}

	//Check for OVER8 configuration bit
	if(pUSARTx->CR1 & (1 << USART_CR1_OVER8))
	{
		//OVER8 = 1 , over sampling by 8
		usartdiv = ((25 * PCLKx) / (2 *BaudRate));
	}else
	{
		//over sampling by 16
		usartdiv = ((25 * PCLKx) / (4 *BaudRate));
	}

	//Calculate the Mantissa part
	M_part = usartdiv/100;

	//Place the Mantissa part in appropriate bit position . refer USART_BRR
	tempreg |= M_part << 4;

	//Extract the fraction part
	F_part = (usartdiv - (M_part * 100));

	//Calculate the final fractional
	if(pUSARTx->CR1 & ( 1 << USART_CR1_OVER8))
	{
		//OVER8 = 1 , over sampling by 8
		F_part = ((( F_part * 8)+ 50) / 100)& ((uint8_t)0x07);
	}else
	{
		//over sampling by 16
		F_part = ((( F_part * 16)+ 50) / 100) & ((uint8_t)0x0F);
	}

	//Place the fractional part in appropriate bit position . refer USART_BRR
	tempreg |= F_part;

	//copy the value of tempreg in to BRR register
	pUSARTx->BRR = tempreg;
}

/*
 * Application callback
 */
/*********************************************************************
 * @fn      		  - USART_ApplicationEventCallback
 *
 * @brief             - Weak implementation of the user application event callback
 *
 * @param[in]         - pUSARTHandle : Handle pointer
 * @param[in]         - AppEv        : Event code
 *
 * @return            - None
 * @Note              - Marked as weak so user can override it in main.c
 *********************************************************************/
__attribute__((weak)) void USART_ApplicationEventCallback(USART_Handle_t *pUSARTHandle, uint8_t AppEv)
{
	// This is a weak implementation. The user application may override this function.
}

/*
 * Some helper function implementations
 */
/*********************************************************************
 * @fn                 - RCC_GetPCLK1Value
 *
 * @brief              - Calculates and returns the APB1 bus clock frequency (PCLK1).
 *
 * @return             - APB1 clock frequency (in Hz)
 *
 * @Note               - Used only within this file (static helper).
 */
static uint32_t RCC_GetPCLK1Value(void)
{
	uint32_t pclk1, SystemClk;
	uint8_t clksrc, temp, ahbp, apb1p;

	clksrc = ((RCC->CFGR >> 2) & 0x03);

	if(clksrc == 0)
	{
		SystemClk = 16000000; // HSI
	}
	else if(clksrc == 1)
	{
		SystemClk = 8000000;  // HSE (depends on your board crystal)
	}
	else if(clksrc == 2)
	{
		// If PLL is used, call the PLL calculation function here
		SystemClk = 16000000;
	}

	// AHB Prescaler
	temp = ((RCC->CFGR >> 4) & 0x0F);
	if(temp < 8)
	{
		ahbp = 1;
	}
	else
	{
		ahbp = AHB_PreScaler[temp - 8];
	}

	// APB1 Prescaler
	temp = ((RCC->CFGR >> 10) & 0x07);
	if(temp < 4)
	{
		apb1p = 1;
	}
	else
	{
		apb1p = APB1_PreScaler[temp - 4];
	}

	pclk1 = (SystemClk / ahbp) / apb1p;

	return pclk1;
}

/*********************************************************************
 * @fn                 - RCC_GetPCLK2Value
 *
 * @brief              - Calculates and returns the APB2 bus clock frequency (PCLK2).
 *
 * @return             - APB2 clock frequency (in Hz)
 *
 * @Note               - Used only within this file (static helper).
 */
static uint32_t RCC_GetPCLK2Value(void)
{
	uint32_t pclk2, SystemClk = 0;
	uint8_t clksrc, temp, ahbp, apb2p;

	clksrc = ((RCC->CFGR >> 2) & 0x03);

	if(clksrc == 0)
	{
		SystemClk = 16000000; // HSI
	}
	else if(clksrc == 1)
	{
		SystemClk = 8000000;  // HSE
	}

	// AHB Prescaler
	temp = ((RCC->CFGR >> 4) & 0x0F);
	if(temp < 8)
	{
		ahbp = 1;
	}
	else
	{
		ahbp = AHB_PreScaler[temp - 8];
	}

	// APB2 Prescaler
	temp = ((RCC->CFGR >> 13) & 0x07);
	if(temp < 4)
	{
		apb2p = 1;
	}
	else
	{
		apb2p = APB2_PreScaler[temp - 4];
	}

	pclk2 = (SystemClk / ahbp) / apb2p;

	return pclk2;
}
