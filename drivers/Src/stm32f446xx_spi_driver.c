/*
 * stm32f446xx_spi_driver.c
 *
 *  Created on: Jul 22, 2026
 *      Author: berkd
 */


#include "stm32f446xx_spi_driver.h"

static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle);
static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle);

/*************************************************************************************************************************
 * @fn 					- SPI_PeriClockControl
 *
 * @brief				- This function enables or disables the peripheral clock for the given SPI peripheral.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral (e.g., SPI1, SPI2, SPI3, SPI4).
 * @param[in]			- EnorDi : ENABLE or DISABLE macro to control the clock.
 *
 * @return				- None.
 *
 * @Note				- None.
 *
 *************************************************************************************************************************/
void SPI_PeriClockControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pSPIx == SPI1)
		{
			SPI1_PCLK_EN();
		}else if(pSPIx == SPI2)
		{
			SPI2_PCLK_EN();
		}else if(pSPIx == SPI3)
		{
			SPI3_PCLK_EN();
		}else if(pSPIx == SPI4)
		{
			SPI4_PCLK_EN();
		}
	}
	else
	{
		//TODO
	}
}

/*************************************************************************************************************************
 * @fn 					- SPI_Init
 *
 * @brief				- This function initializes the given SPI peripheral with the user settings.
 *
 * @param[in]			- pSPIHandle : Pointer to the SPI_Handle_t structure containing configuration parameters.
 *
 * @return				- None.
 *
 * @Note				- Enables the SPI peripheral clock automatically inside the function.
 *
 *************************************************************************************************************************/
void SPI_Init(SPI_Handle_t *pSPIHandle)
{

	//peripheral clock enable
	SPI_PeriClockControl(pSPIHandle->pSPIx, ENABLE);

	//first lets configure the SPI_CR1 register

	uint32_t tempreg = 0;

	//1. configure the device mode
	tempreg |= pSPIHandle->SPIConfig.SPI_DeviceMode << SPI_CR1_MSTR;

	//2. configure the bus config
	if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_FD)
	{
		//BIDI mode should be cleared
		tempreg &= ~( 1 << SPI_CR1_BIDIMODE );
	}else if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_HD)
	{
		//BIDI mode should be set
		tempreg |= ( 1 << SPI_CR1_BIDIMODE );
	}else if(pSPIHandle->SPIConfig.SPI_BusConfig == SPI_BUS_CONFIG_SIMPLEX_RXONLY)
	{
		//BIDI mode should be cleared
		tempreg &= ~( 1 << SPI_CR1_BIDIMODE );
		//RXONLY bit must be set
		tempreg |= ( 1 << SPI_CR1_RXONLY );
	}

	//3. configure the SPI serial clock speed (baud rate)
	tempreg |= pSPIHandle->SPIConfig.SPI_SclkSpeed << SPI_CR1_BR;

	//4. configure the DFF
	tempreg |= pSPIHandle->SPIConfig.SPI_DFF << SPI_CR1_DFF;

	//5. configure the CPOL
	tempreg |= pSPIHandle->SPIConfig.SPI_CPOL << SPI_CR1_CPOL;

	//6. configure the CPHA
	tempreg |= pSPIHandle->SPIConfig.SPI_CPHA << SPI_CR1_CPHA;

	//7. configure the SSM
	tempreg |= pSPIHandle->SPIConfig.SPI_SSM << SPI_CR1_SSM;

	pSPIHandle->pSPIx->CR1 = tempreg;

}

/*************************************************************************************************************************
 * @fn 					- SPI_DeInit
 *
 * @brief				- Resets all the registers of the given SPI peripheral to their default state.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral (e.g., SPI1, SPI2, SPI3, SPI4).
 *
 * @return				- None.
 *
 * @Note				- Uses RCC peripheral reset macros to reset the hardware logic.
 *
 *************************************************************************************************************************/
void SPI_DeInit(SPI_RegDef_t *pSPIx)
{
	if(pSPIx == SPI1)
	{
		SPI1_REG_RESET();
	}
	else if(pSPIx == SPI2)
	{
		SPI2_REG_RESET();
	}
	else if(pSPIx == SPI3)
	{
		SPI3_REG_RESET();
	}
	else if(pSPIx == SPI4)
	{
		SPI4_REG_RESET();
	}
}

/*************************************************************************************************************************
 * @fn 					- SPI_GetFlagStatus
 *
 * @brief				- Checks whether a specific SPI Status Register (SR) flag is SET or RESET.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral (e.g., SPI1, SPI2, SPI3, SPI4).
 * @param[in]			- FlagName : The bitmask/flag to check (e.g., SPI_TXE_FLAG, SPI_RXNE_FLAG, SPI_BUSY_FLAG).
 *
 * @return				- FLAG_SET or FLAG_RESET.
 *
 * @Note				- None.
 *
 *************************************************************************************************************************/
uint8_t SPI_GetFlagStatus(SPI_RegDef_t *pSPIx, uint32_t FlagName)
{
	if(pSPIx->SR & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*************************************************************************************************************************
 * @fn 					- SPI_SendData
 *
 * @brief				- Sends data over SPI peripheral using blocking (polling) mode.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral.
 * @param[in]			- pTxBuffer : Pointer to the transmit data buffer.
 * @param[in]			- Len : Length of data in bytes to be transmitted.
 *
 * @return				- None.
 *
 * @Note				- This is a blocking API (polling on TXE flag).
 *
 *************************************************************************************************************************/
void SPI_SendData(SPI_RegDef_t *pSPIx, uint8_t *pTxBuffer, uint32_t Len)
{
	while(Len > 0)
	{
		//1. wait until TXE is set
		while(SPI_GetFlagStatus(pSPIx, SPI_FLAG_TXE) == FLAG_RESET );

		//2. check the DFF bit in CR1
		if( (pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
		{
			//16 bit DFF
			//1. load the data in to the DR
			pSPIx->DR = *((uint16_t*)pTxBuffer);
			Len -= 2;
			pTxBuffer += 2;
		}else
		{
			//8 bit DFF
			//1. load the data in to the DR
			pSPIx->DR = *pTxBuffer;
			Len--;
			pTxBuffer++;
		}
	}
}


/*************************************************************************************************************************
 * @fn 					- SPI_ReceiveData
 *
 * @brief				- Receives data over SPI peripheral using blocking (polling) mode.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral.
 * @param[in]			- pRxBuffer : Pointer to the receive data buffer.
 * @param[in]			- Len : Length of data in bytes to be received.
 *
 * @return				- None.
 *
 * @Note				- This is a blocking API (polling on RXNE flag).
 *
 *************************************************************************************************************************/
void SPI_ReceiveData(SPI_RegDef_t *pSPIx, uint8_t *pRxBuffer, uint32_t Len)
{
	while(Len > 0)
	{
		//1. wait until RXNE is set
		while(SPI_GetFlagStatus(pSPIx, SPI_FLAG_RXNE) == FLAG_RESET );

		//2. check the DFF bit in CR1
		if( (pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
		{
			//16 bit DFF
			//1. load the data from DR to Rxbuffer address
			*((uint16_t*)pRxBuffer) = pSPIx->DR;
			Len -= 2;
			pRxBuffer += 2;
		}else
		{
			//8 bit DFF
			//1. load the data from DR to Rxbuffer address
			*pRxBuffer = pSPIx->DR;
			Len--;
			pRxBuffer++;
		}
	}
}

/*************************************************************************************************************************
 * @fn 					- SPI_SendDataIT
 *
 * @brief				- Sends data over SPI peripheral using interrupt (non-blocking) mode.
 *
 * @param[in]			- pSPIHandle : Pointer to the SPI_Handle_t structure.
 * @param[in]			- pTxBuffer  : Pointer to the transmit data buffer.
 * @param[in]			- Len        : Length of data in bytes to be transmitted.
 *
 * @return				- Current application state (SPI_READY or SPI_BUSY_IN_TX).
 *
 * @Note				- This is a non-blocking API.
 *
 *************************************************************************************************************************/
uint8_t SPI_SendDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pTxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->TxState;

	if(state != SPI_BUSY_IN_TX)
	{
		//1. Save the Tx buffer address and Len information in some global variables
		pSPIHandle->pTxBuffer = pTxBuffer;
		pSPIHandle->TxLen = Len;

		//2. Mark the SPI state as busy in transmission so that
		// 	 no other code can take over same SPI peripheral until transmission is over
		pSPIHandle->TxState = SPI_BUSY_IN_TX;

		//3. Enable the TXEIE control bit to get interrupt whenever TXE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_TXEIE);

	}

	return state;
}

/*************************************************************************************************************************
 * @fn 					- SPI_ReceiveDataIT
 *
 * @brief				- Receives data over SPI peripheral using interrupt (non-blocking) mode.
 *
 * @param[in]			- pSPIHandle : Pointer to the SPI_Handle_t structure.
 * @param[in]			- pRxBuffer  : Pointer to the receive data buffer.
 * @param[in]			- Len        : Length of data in bytes to be received.
 *
 * @return				- Current application state (SPI_READY or SPI_BUSY_IN_RX).
 *
 * @Note				- This is a non-blocking API.
 *
 *************************************************************************************************************************/
uint8_t SPI_ReceiveDataIT(SPI_Handle_t *pSPIHandle, uint8_t *pRxBuffer, uint32_t Len)
{
	uint8_t state = pSPIHandle->RxState;

	if(state != SPI_BUSY_IN_RX)
	{
		//1. Save the Rx buffer address and Len information in some global variables
		pSPIHandle->pRxBuffer = pRxBuffer;
		pSPIHandle->RxLen = Len;

		//2. Mark the SPI state as busy in transmission so that
		// 	 no other code can take over same SPI peripheral until transmission is over
		pSPIHandle->RxState = SPI_BUSY_IN_RX;

		//3. Enable the RXNEIE control bit to get interrupt whenever TXE flag is set in SR
		pSPIHandle->pSPIx->CR2 |= ( 1 << SPI_CR2_RXNEIE);

	}

	return state;
}

/***********************************************************************************************
 * @fn 					- SPI_IRQInterruptConfig
 *
 * @brief				- Enables or disables the specified IRQ number in the ARM Cortex-M NVIC.
 *
 * @param[in]			- IRQNumber  : Interrupt Request number to be configured.
 * @param[in]			- EnorDi     : ENABLE or DISABLE macro.
 *
 * @return				- None.
 *
 * @Note				- Configures the NVIC_ISERx (Set Enable) or NVIC_ICERx (Clear Enable) registers.
 *
 ************************************************************************************************/
void SPI_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			// NVIC_ISER0 (IRQ 0 - 31)
			*NVIC_ISER0 |= ( 1 << IRQNumber );

		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// NVIC_ISER1 (IRQ 32 - 63)
			*NVIC_ISER1 |= ( 1 << (IRQNumber % 32) );

		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// NVIC_ISER2 (IRQ 64 - 95)
			*NVIC_ISER2 |= ( 1 << (IRQNumber % 32) );
		}
	}
	else
	{
		if(IRQNumber <= 31)
		{
			// NVIC_ICER0 (IRQ 0 - 31)
			*NVIC_ICER0 |= ( 1 << IRQNumber );
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// NVIC_ICER1 (IRQ 32 - 63)
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// NVIC_ICER2 (IRQ 64 - 95)
			*NVIC_ICER2 |= ( 1 << (IRQNumber % 32) );
		}
	}
}

/***********************************************************************************************
 * @fn 					- SPI_IRQPriorityConfig
 *
 * @brief				- Configures the priority level for a given IRQ number in the NVIC.
 *
 * @param[in]			- IRQNumber  : Interrupt Request number.
 * @param[in]			- IRQPriority: Priority level to be assigned (0 to 15 for STM32F4).
 *
 * @return				- None.
 *
 * @Note				- Clears the register section first, then shifts the priority value to
 * 						  match the MSB implemented bits of the IPR register.
 *
 ************************************************************************************************/
void SPI_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	// 1. Find out the IPR register and byte section
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;

	// 2. Calculate shift amount taking implemented bits (MSB) into account
	uint8_t shift_amount = ( 8 * iprx_section ) + ( 8 - NO_PR_BITS_IMPLEMENTED );

	// 3. Clear existing priority bits in the section
	NVIC_PR_BASE_ADDR[iprx] &= ~( 0xFF << (8 * iprx_section) );

	// 4. Set the new priority level
	NVIC_PR_BASE_ADDR[iprx] |= ( IRQPriority << shift_amount );
}

/*************************************************************************************************************************
 * @fn 					- SPI_IRQHandling
 *
 * @brief				- Receives data over SPI peripheral using interrupt (non-blocking) mode.
 *
 * @param[in]			- pSPIHandle : Pointer to the SPI_Handle_t structure.
 * @param[in]			- pRxBuffer  : Pointer to the receive data buffer.
 * @param[in]			- Len        : Length of data in bytes to be received.
 *
 * @return				- Current application state (SPI_READY or SPI_BUSY_IN_RX).
 *
 * @Note				- This is a non-blocking API.
 *
 *************************************************************************************************************************/
void SPI_IRQHandling(SPI_Handle_t *pSPIHandle)
{
	uint8_t temp1, temp2;
	//first lets check for RXNE
	temp1 = pSPIHandle->pSPIx->SR & ( 1 << SPI_SR_RXNE);
	temp2 = pSPIHandle->pSPIx->CR2 & ( 1 << SPI_CR2_RXNEIE);

	if(temp1 && temp2)
	{
		//handle RXNE
		spi_rxne_interrupt_handle(pSPIHandle);
	}

	//check for TXE
	temp1 = pSPIHandle->pSPIx->SR & ( 1 << SPI_SR_TXE);
	temp2 = pSPIHandle->pSPIx->CR2 & ( 1 << SPI_CR2_TXEIE);

	if(temp1 && temp2)
	{
		//handle TXE
		spi_txe_interrupt_handle(pSPIHandle);
	}

	//check for ovr flag
	temp1 = pSPIHandle->pSPIx->SR & ( 1 << SPI_SR_OVR);
	temp2 = pSPIHandle->pSPIx->CR2 & ( 1 << SPI_CR2_ERRIE);

	if(temp1 && temp2)
	{
		//handle ovr error
		spi_ovr_err_interrupt_handle(pSPIHandle);
	}

}

/*************************************************************************************************************************
 * @fn 					- SPI_PeripheralControl
 *
 * @brief				- Enables or disables the given SPI peripheral by controlling the SPE bit in CR1.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral (e.g., SPI1, SPI2, SPI3, SPI4).
 * @param[in]			- EnorDi : ENABLE or DISABLE macro.
 *
 * @return				- None.
 *
 * @Note				- SPI peripheral must be enabled after configuration and before data transfer!
 *
 *************************************************************************************************************************/
void SPI_PeripheralControl(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR1 |= (1 << SPI_CR1_SPE);
	}else
	{
		pSPIx->CR1 &= ~(1 << SPI_CR1_SPE);
	}
}

/*************************************************************************************************************************
 * @fn 					- SPI_SSIConfig
 *
 * @brief				- Configures the SSI (Internal Slave Select) bit in CR1 register for Software Slave Management.
 *
 * @param[in]			- pSPIx : Base address of the SPI peripheral (e.g., SPI1, SPI2, SPI3, SPI4).
 * @param[in]			- EnorDi : ENABLE (sets SSI to 1, pulling NSS internally high) or DISABLE.
 *
 * @return				- None.
 *
 * @Note				- Required when SSM (Software Slave Management) is enabled to prevent MODF (Mode Fault) errors in Master mode.
 *
 *************************************************************************************************************************/
void SPI_SSIConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR1 |= (1 << SPI_CR1_SSI);
	}else
	{
		pSPIx->CR1 &= ~(1 << SPI_CR1_SSI);
	}
}

/***********************************************************************************************
 * @fn 					- SPI_SSOEConfig
 *
 * @brief				- Enables or disables Slave Select Output Enable (SSOE) in CR2 register.
 *
 * @param[in]			- pSPIx  : Base address of the SPI peripheral.
 * @param[in]			- EnorDi : ENABLE or DISABLE macro.
 *
 * @return				- None.
 *
 * @Note				- When SSOE is enabled, NSS pin is automatically managed by hardware in Master mode.
 *
 ************************************************************************************************/
void SPI_SSOEConfig(SPI_RegDef_t *pSPIx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pSPIx->CR2 |= ( 1 << SPI_CR2_SSOE );
	}
	else
	{
		pSPIx->CR2 &= ~( 1 << SPI_CR2_SSOE );
	}
}

/*
 * Some helper function implementations
 */
static void spi_txe_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	//check the DFF bit in CR1
	if( (pSPIHandle->pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
	{
		//16 bit DFF
		//1. load the data in to the DR
		pSPIHandle->pSPIx->DR = *((uint16_t*)pSPIHandle->pTxBuffer);
		pSPIHandle->TxLen -= 2;
		pSPIHandle->pTxBuffer += 2;
	}else
	{
		//8 bit DFF
		//1. load the data in to the DR
		pSPIHandle->pSPIx->DR = *pSPIHandle->pTxBuffer;
		pSPIHandle->TxLen--;
		pSPIHandle->pTxBuffer++;
	}

	if(! pSPIHandle->TxLen)
	{
		//Txlen is zero, so close the spi transmission and inform the application that
		//Tx is over.

		//this prevents interrupts from setting up of TXE flag
		SPI_CloseTransmisson(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_TX_CMPLT);
	}

}

static void spi_rxne_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	//do rxing as per the dff
	if( (pSPIHandle->pSPIx->CR1 & ( 1 << SPI_CR1_DFF) ) )
	{
		//16 bit DFF
		*((uint16_t*)pSPIHandle->pRxBuffer) = (uint16_t)pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen -= 2;
		pSPIHandle->pRxBuffer += 2;
	}else
	{
		//8 bit DFF
		*(pSPIHandle->pRxBuffer) = (uint8_t)pSPIHandle->pSPIx->DR;
		pSPIHandle->RxLen--;
		pSPIHandle->pRxBuffer++;
	}

	if(! pSPIHandle->RxLen)
	{
		//reception is complete
		SPI_CloseReception(pSPIHandle);
		SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_RX_CMPLT);
	}
}

static void spi_ovr_err_interrupt_handle(SPI_Handle_t *pSPIHandle)
{
	uint8_t temp;
	//1. clear the ovr flag
	if(pSPIHandle->TxState != SPI_BUSY_IN_TX)
	{
		temp = pSPIHandle->pSPIx->DR;
		temp = pSPIHandle->pSPIx->SR;
	}
	(void)temp;
	//2. inform the application
	SPI_ApplicationEventCallback(pSPIHandle, SPI_EVENT_OVR_ERR);
}

void SPI_CloseTransmisson(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~( 1 << SPI_CR2_TXEIE);
	pSPIHandle->pTxBuffer = NULL;
	pSPIHandle->TxLen = 0;
	pSPIHandle->TxState = SPI_READY;
}

void SPI_CloseReception(SPI_Handle_t *pSPIHandle)
{
	pSPIHandle->pSPIx->CR2 &= ~( 1 << SPI_CR2_RXNEIE);
	pSPIHandle->pRxBuffer = NULL;
	pSPIHandle->RxLen = 0;
	pSPIHandle->RxState = SPI_READY;
}

void SPI_ClearOVRFlag(SPI_RegDef_t *pSPIx)
{
	uint8_t temp;
	temp = pSPIx->DR;
	temp = pSPIx->SR;
	(void)temp;
}

__weak void SPI_ApplicationEventCallback(SPI_Handle_t *pSPIHandle, uint8_t AppEv)
{
	//This is a weak implementation. the user application may overide this function.
}
