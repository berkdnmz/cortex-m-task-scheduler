/*
 * stm32f446xx_i2c_driver.c
 *
 *  Created on: Jul 29, 2026
 *      Author: berkd
 */

#include"stm32f446xx_i2c_driver.h"

uint16_t AHB_PreScaler[8] = {2, 4, 8, 16, 64, 128, 256, 512};
uint16_t APB1_PreScaler[4] = {2, 4, 8, 16};

static uint32_t RCC_GetPLLOutputClock(void);
static uint32_t RCC_GetPLCK1Value(void);
static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx);
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr);
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle);
static void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx);
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle);
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle);

/*************************************************************************************************************************
 * @fn 					- I2C_PeriClockControl
 *
 * @brief				- This function enables or disables the peripheral clock for the given I2C peripheral.
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral (e.g., I2C1, I2C2, I2C3).
 * @param[in]			- EnorDi : ENABLE or DISABLE macro to control the clock.
 *
 * @return				- None.
 *
 * @Note				- None.
 *
 *************************************************************************************************************************/
void I2C_PeriClockControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(pI2Cx == I2C1)
		{
			I2C1_PCLK_EN();
		}else if(pI2Cx == I2C2)
		{
			I2C2_PCLK_EN();
		}else if(pI2Cx == I2C3)
		{
			I2C3_PCLK_EN();
		}
	}
	else
	{
		if (pI2Cx == I2C1)
		{
			I2C1_PCLK_DI();
		}
		else if (pI2Cx == I2C2)
		{
			I2C2_PCLK_DI();
		}
		else if (pI2Cx == I2C3)
		{
			I2C3_PCLK_DI();
		}
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_Init
 *
 * @brief				- This function initializes the given I2C peripheral with the user settings.
 *
 * @param[in]			- pI2CHandle : Pointer to the I2C_Handle_t structure containing configuration parameters.
 *
 * @return				- None.
 *
 * @Note				- Enables the I2C peripheral clock automatically inside the function.
 *
 *************************************************************************************************************************/
void I2C_Init(I2C_Handle_t *pI2CHandle)
{
	//peripheral clock enable
	I2C_PeriClockControl(pI2CHandle->pI2Cx, ENABLE);

	uint32_t tempreg = 0;

	//ACK control bit
	tempreg |= (pI2CHandle->I2C_Config.I2C_ACKControl << I2C_CR1_ACK);
	pI2CHandle->pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
	pI2CHandle->pI2Cx->CR1 |= tempreg;

	//configure the FREQ field of CR2
	tempreg = 0;
	tempreg = RCC_GetPLCK1Value() / 1000000U;
	pI2CHandle->pI2Cx->CR2 &= ~(0x3F);
	pI2CHandle->pI2Cx->CR2 |= (tempreg & 0x3F);

	//program the device own address
	tempreg = 0;
	tempreg = pI2CHandle->I2C_Config.I2C_DeviceAddress << 1;
	tempreg |= (1 << 14);
	pI2CHandle->pI2Cx->OAR1 = tempreg;

	//CCR calculations
	uint16_t ccr_value = 0;
	tempreg = 0;
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		//mode is standard mode
		ccr_value = RCC_GetPLCK1Value() / ( 2 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		tempreg |= (ccr_value & 0xFFFF);
	}else
	{
		//mode is fast mode
		tempreg |= ( 1 << 15 );
		tempreg |= (pI2CHandle->I2C_Config.I2C_FMDutyCycle << 14);
		if(pI2CHandle->I2C_Config.I2C_FMDutyCycle == I2C_FM_DUTY_2)
		{
			ccr_value = RCC_GetPLCK1Value() / ( 3 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		}else
		{
			ccr_value = RCC_GetPLCK1Value() / ( 25 * pI2CHandle->I2C_Config.I2C_SCLSpeed);
		}
		tempreg |= (ccr_value & 0xFFF);
	}
	pI2CHandle->pI2Cx->CCR |= tempreg;

	//TRISE Configuration
	tempreg = 0;
	if(pI2CHandle->I2C_Config.I2C_SCLSpeed <= I2C_SCL_SPEED_SM)
	{
		// Standard mode: TRISE = (FREQ + 1)
		tempreg = (RCC_GetPLCK1Value() / 1000000U) + 1;
	}else
	{
		// Fast mode: TRISE = ((PCLK1 * 300ns) / 1s) + 1
		tempreg = ((RCC_GetPLCK1Value() * 300U) / 1000000000U) + 1;
	}
	pI2CHandle->pI2Cx->TRISE = (tempreg & 0x3F);
}

/*************************************************************************************************************************
 * @fn 					- I2C_DeInit
 *
 * @brief				- Resets all the registers of the given I2C peripheral to their default state.
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral (e.g., I2C1, I2C2, I2C3, I2C4).
 *
 * @return				- None.
 *
 * @Note				- Uses RCC peripheral reset macros to reset the hardware logic.
 *
 *************************************************************************************************************************/
void I2C_DeInit(I2C_RegDef_t *pI2Cx)
{
	if(pI2Cx == I2C1)
	{
		I2C1_REG_RESET();
	}
	else if(pI2Cx == I2C2)
	{
		I2C2_REG_RESET();
	}
	else if(pI2Cx == I2C3)
	{
		I2C3_REG_RESET();
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterSendData
 *
 * @brief				- Sends data to a slave device in blocking mode.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C_Handle_t structure.
 * @param[in]			- pTxBuffer  : Pointer to user Tx buffer.
 * @param[in]			- Len        : Number of bytes to send.
 * @param[in]			- SlaveAddr  : Target 7-bit slave address.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_MasterSendData(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr)
{
	//1. Generate the START condition
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2. confirm that start generation is completed by checking the SB flag in the SR1
	//	Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB)  );

	//3. Send the address of the slave with r/nw bit set to w(0) (total 8 bits)
	I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, SlaveAddr);

	//4. Confirm that address phase is completed by checking the ADDR flag in the SR1
	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR)  );

	//5. Clear the ADDR flag according to its software sequence
	//	Note: Until ADDR is cleared SCL will be stretched (pulled to LOW)
	I2C_ClearADDRFlag(pI2CHandle);

	//6. send the data until len becomes 0

	while(Len > 0)
	{
		while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE) ); //Wait till TXE is set
		pI2CHandle->pI2Cx->DR = *pTxBuffer;
		pTxBuffer++;
		Len--;
	}

	//7. when Len becomes zero wait for TXE=1 and BTF=1 before generating the STOP condition
	//	 Note: TXE=1, BTF=1, means that both SR and DR are empty and next transmission should begin
	//	 when BTF=1 SCL will be stretched (pulled to LOW)

	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_TXE) );

	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_BTF) );


	//8. Generate STOP condition and master need not to wait for the completion of stop condition.
	//	 Note: generating STOp, automatically clears the BTF
	I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterReceiveData
 *
 * @brief				- Receives data from a slave device in blocking mode.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C_Handle_t structure.
 * @param[in]			- pRxBuffer  : Pointer to user Rx buffer.
 * @param[in]			- Len        : Number of bytes to receive.
 * @param[in]			- SlaveAddr  : Target 7-bit slave address.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_MasterReceiveData(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t Len, uint8_t SlaveAddr)
{
	//1. Generate the START condition
	I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

	//2. confirm that start generation is completed by checking the SB flag in the SR1
	//	 Note: Until SB is cleared SCL will be stretched (pulled to LOW)
	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_SB)  );

	//3. Sent the address of the slave with r/nw bit set to R(1) (total 8 bits)
	I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, SlaveAddr);

	//4. wait until address phase is completed by checking the ADDR flag in the SR1
	while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_ADDR)  );


	//procedure to read only 1 byte from slave
	if(Len == 1)
	{
		//Disable Acking
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

		//clear the ADDR flag
		I2C_ClearADDRFlag(pI2CHandle);

		//generate STOP condition
		I2C_GenerateStopCondition(pI2CHandle->pI2Cx);

		//wait until RXNE becomes 1
		while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE) );

		//read data in to buffer
		*pRxBuffer = pI2CHandle->pI2Cx->DR;

		return;
	}

	//procedure to read data from slave when Len > 1
	if(Len > 1)
	{
		//clear the ADDR flag
		I2C_ClearADDRFlag(pI2CHandle);

		//read the data until Len becomes zero
		for(uint32_t i = Len; i > 0; i--)
		{
			//wait until RXNE becomes 1
			while( ! I2C_GetFlagStatus(pI2CHandle->pI2Cx, I2C_FLAG_RXNE) );

			if(i == 2) //if last 2 bytes are remaining
			{
				//Disable Acking
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

				//generate STOP condition
				I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
			}

			//read the data from data register in to buffer
			*pRxBuffer = pI2CHandle->pI2Cx->DR;

			//increment the buffer address
			pRxBuffer++;
		}
	}

	//re-enable ACKing
	if(pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterSendDataIT
 *
 * @brief				- Initiates I2C Master Transmission in Interrupt mode (Non-Blocking).
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 * @param[in]			- pTxBuffer  : Pointer to user Tx buffer.
 * @param[in]			- Len        : Number of bytes to send.
 * @param[in]			- SlaveAddr  : Target slave address.
 * @param[in]			- Sr         : Repeated Start option (ENABLE/DISABLE).
 *
 * @return				- Current bus state (I2C_READY, I2C_BUSY_IN_TX, I2C_BUSY_IN_RX).
 *
 *************************************************************************************************************************/
uint8_t I2C_MasterSendDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pTxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if( (busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		//1. Save transaction state into handle
		pI2CHandle->pTxBuffer = pTxBuffer;
		pI2CHandle->TxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_TX;
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		//2. Generate START condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//3. Enable ITBUFEN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);

		//4. Enable ITEVFEN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);

		//5. Enable ITERREN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
	}

	return busystate;
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterReceiveDataIT
 *
 * @brief				- Initiates I2C Master Reception in Interrupt mode (Non-Blocking).
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 * @param[in]			- pRxBuffer  : Pointer to user Rx buffer.
 * @param[in]			- Len        : Number of bytes to receive.
 * @param[in]			- SlaveAddr  : Target slave address.
 * @param[in]			- Sr         : Repeated Start option (ENABLE/DISABLE).
 *
 * @return				- Current bus state (I2C_READY, I2C_BUSY_IN_TX, I2C_BUSY_IN_RX).
 *
 *************************************************************************************************************************/
uint8_t I2C_MasterReceiveDataIT(I2C_Handle_t *pI2CHandle, uint8_t *pRxBuffer, uint32_t Len, uint8_t SlaveAddr, uint8_t Sr)
{
	uint8_t busystate = pI2CHandle->TxRxState;

	if( (busystate != I2C_BUSY_IN_TX) && (busystate != I2C_BUSY_IN_RX))
	{
		//1. Save transaction state into handle
		pI2CHandle->pRxBuffer = pRxBuffer;
		pI2CHandle->RxLen = Len;
		pI2CHandle->TxRxState = I2C_BUSY_IN_RX;
		pI2CHandle->RxSize = Len; //Rxsize is used in the ISR code to manage the data reception
		pI2CHandle->DevAddr = SlaveAddr;
		pI2CHandle->Sr = Sr;

		//2. Generate START condition
		I2C_GenerateStartCondition(pI2CHandle->pI2Cx);

		//3. Enable ITBUFEN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITBUFEN);

		//4. Enable ITEVFEN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITEVTEN);

		//5. Enable ITERREN control bit
		pI2CHandle->pI2Cx->CR2 |= (1 << I2C_CR2_ITERREN);
	}

	return busystate;
}

/*************************************************************************************************************************
 * @fn 					- I2C_CloseSendData
 *
 * @brief				- Closes I2C Master Transmission by disabling interrupts and resetting handle states.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_CloseSendData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to disable ITBUFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);

	//Implement the code to disable ITEVFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pTxBuffer = NULL;
	pI2CHandle->TxLen = 0;
}

/*************************************************************************************************************************
 * @fn 					- I2C_CloseReceiveData
 *
 * @brief				- Closes I2C Master Reception by disabling interrupts and resetting handle states.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_CloseReceiveData(I2C_Handle_t *pI2CHandle)
{
	//Implement the code to disable ITBUFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);

	//Implement the code to disable ITEVFEN Control Bit
	pI2CHandle->pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);

	pI2CHandle->TxRxState = I2C_READY;
	pI2CHandle->pRxBuffer = NULL;
	pI2CHandle->RxLen = 0;
	pI2CHandle->RxSize = 0;

	if(pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
	{
		I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_SlaveSendData
 *
 * @brief				- Sends a byte of data in I2C Slave mode by writing to the Data Register (DR).
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral.
 * @param[in]			- data  : 8-bit data byte to be transmitted.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_SlaveSendData(I2C_RegDef_t *pI2Cx, uint8_t data)
{
	pI2Cx->DR = data;
}

/*************************************************************************************************************************
 * @fn 					- I2C_SlaveReceiveData
 *
 * @brief				- Receives a byte of data in I2C Slave mode by reading from the Data Register (DR).
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral.
 *
 * @return				- uint8_t : Received 8-bit data byte.
 *
 *************************************************************************************************************************/
uint8_t I2C_SlaveReceiveData(I2C_RegDef_t *pI2Cx)
{
	return (uint8_t)pI2Cx->DR;
}

/*
 * IRQ Configuration and ISR handling
 */
/*************************************************************************************************************************
 * @fn 					- I2C_IRQInterruptConfig
 *
 * @brief				- Enables or Disables specified IRQ number in Cortex-M NVIC registers.
 *
 * @param[in]			- IRQNumber : IRQ position number in NVIC table.
 * @param[in]			- EnorDi    : ENABLE or DISABLE.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_IRQInterruptConfig(uint8_t IRQNumber, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		if(IRQNumber <= 31)
		{
			// Program ISER0 register
			*NVIC_ISER0 |= ( 1 << IRQNumber );
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// Program ISER1 register
			*NVIC_ISER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// Program ISER2 register
			*NVIC_ISER2 |= ( 1 << (IRQNumber % 32) );
		}
	}
	else
	{
		if(IRQNumber <= 31)
		{
			// Program ICER0 register
			*NVIC_ICER0 |= ( 1 << IRQNumber );
		}
		else if(IRQNumber > 31 && IRQNumber < 64)
		{
			// Program ICER1 register
			*NVIC_ICER1 |= ( 1 << (IRQNumber % 32) );
		}
		else if(IRQNumber >= 64 && IRQNumber < 96)
		{
			// Program ICER2 register
			*NVIC_ICER2 |= ( 1 << (IRQNumber % 32) );
		}
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_IRQPriorityConfig
 *
 * @brief				- Configures priority level for specified IRQ number.
 *
 * @param[in]			- IRQNumber  : IRQ position number.
 * @param[in]			- IRQPriority: Priority level value.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_IRQPriorityConfig(uint8_t IRQNumber, uint32_t IRQPriority)
{
	uint8_t iprx = IRQNumber / 4;
	uint8_t iprx_section = IRQNumber % 4;

	uint8_t shift_amount = (8 * iprx_section) + (8 - NO_PR_BITS_IMPLEMENTED);
	*(NVIC_PR_BASE_ADDR + iprx) |= (IRQPriority << shift_amount);
}

/*************************************************************************************************************************
 * @fn 					- I2C_EV_IRQHandling
 *
 * @brief				- Handles I2C Event Interrupts (SB, ADDR, BTF, STOPF, TXE, RXNE).
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_EV_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	//Interrupt handling for both master and slave mode of a device
	uint32_t temp1, temp2, temp3;

	temp1 = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITEVTEN);
	temp2 = pI2CHandle->pI2Cx->CR2 & ( 1 << I2C_CR2_ITBUFEN);

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_SB);

	//1. Handle For interrupt generated by SB event
	//	Note : SB flag is only applicable in Master mode
	if(temp1 && temp3)
	{
		//The interrupt is generated because of SB event
		//This block will not be executed in slave mode because for slave SB is always zero
		//In this block lets executed the address phase
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_ExecuteAddressPhaseWrite(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		}else if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_ExecuteAddressPhaseRead(pI2CHandle->pI2Cx, pI2CHandle->DevAddr);
		}
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_ADDR);
	//2. Handle For interrupt generated by ADDR event
	//  Note : When master mode : Address is sent
	//		   When Slave mode  : Address matched with own address
	if(temp1 && temp3)
	{
		//interrupt is generated because of ADDR event
		I2C_ClearADDRFlag(pI2CHandle);
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_BTF);
	//3. Handle For interrupt generated by BTF(Byte Transfer Finished) event
	if(temp1 && temp3)
	{
		//BTF flag is set
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			//make sure that TXE is also set
			if(pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_TXE) )
			{
				//BTF, TXE = 1
				if(pI2CHandle->TxLen == 0)
				{

					//1. generate the STOP condition
					if(pI2CHandle->Sr == I2C_DISABLE_SR)
					{
						I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
					}

					//2. reset all the member elements of the handle structure
					I2C_CloseSendData(pI2CHandle);

					//3. notify the application about transmission complete
					I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_TX_CMPLT);
				}
			}

		}else if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{

		}
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_STOPF);
	//4. Handle For interrupt generated by STOPF event
	//  Note : Stop detection flag is applicable only slave mode . For master this flag will never be set
	if(temp1 && temp3)
	{
		//STOPF flag is set
		//Clear the STOPF ( i.e 1) read SR1 2) Write to CR1 )

		uint32_t dummy_read = pI2CHandle->pI2Cx->SR1; // Read SR1
		(void)dummy_read;
		pI2CHandle->pI2Cx->CR1 |= 0x0000;

		//Notify the application that STOP is detected
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_STOP);
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_TXE);
	//5. Handle For interrupt generated by TXE event
	if(temp1 && temp2 && temp3)
	{
		// Check for Master mode
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_TX)
		{
			I2C_MasterHandleTXEInterrupt(pI2CHandle);
		}
		else
		{
			// Device is in Slave mode
			// Check if Slave is in Transmitter mode (SR2 TRA bit is set)
			if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_TRA))
			{
				I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_REQ);
			}
		}
	}

	temp3 = pI2CHandle->pI2Cx->SR1 & ( 1 << I2C_SR1_RXNE);
	//6. Handle For interrupt generated by RXNE event
	if(temp1 && temp2 && temp3)
	{
		// Check for Master mode
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			I2C_MasterHandleRXNEInterrupt(pI2CHandle);
		}
		else
		{
			// Device is in Slave mode
			// Check if Slave is in Receiver mode (SR2 TRA bit is cleared)
			if( !(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_TRA)) )
			{
				I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_DATA_RCV);
			}
		}
	}

}

/*************************************************************************************************************************
 * @fn 					- I2C_ER_IRQHandling
 *
 * @brief				- Handles I2C error interrupts and notifies the application
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 * @Note                - Executed when ITERREN bit is set in CR2
 *
 *************************************************************************************************************************/
void I2C_ER_IRQHandling(I2C_Handle_t *pI2CHandle)
{
	uint32_t temp1, temp2;

	temp2 = (pI2CHandle->pI2Cx->CR2) & (1 << I2C_CR2_ITERREN);

	//1. Check for Bus error (BERR)
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1 << I2C_SR1_BERR);
	if(temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_BERR);

		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_BERR);
	}

	//2. Check for Arbitration Lost Error (ARLO)
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1 << I2C_SR1_ARLO);
	if(temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_ARLO);

		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_ARLO);
	}

	//3. Check for ACK Failure Error (AF)
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1 << I2C_SR1_AF);
	if(temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_AF);

		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_AF);
	}

	//4. Check for Overrun/Underrun Error (OVR)
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1 << I2C_SR1_OVR);
	if(temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_OVR);

		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_OVR);
	}

	//5. Check for Timeout Error (TIMEOUT)
	temp1 = (pI2CHandle->pI2Cx->SR1) & (1 << I2C_SR1_TIMEOUT);
	if(temp1 && temp2)
	{
		pI2CHandle->pI2Cx->SR1 &= ~(1 << I2C_SR1_TIMEOUT);

		I2C_ApplicationEventCallback(pI2CHandle, I2C_ERROR_TIMEOUT);
	}

}


/*
 * Other Peripheral Control APIs
 */
uint8_t I2C_GetFlagStatus(I2C_RegDef_t *pI2Cx, uint32_t FlagName)
{
	if(pI2Cx->SR1 & FlagName)
	{
		return FLAG_SET;
	}
	return FLAG_RESET;
}

/*************************************************************************************************************************
 * @fn 					- I2C_PeripheralControl
 *
 * @brief				- Enables or disables the given I2C peripheral by toggling the PE bit in CR1.
 *
 * @param[in]			- pI2Cx  : Base address of the I2C peripheral (e.g., I2C1, I2C2, I2C3).
 * @param[in]			- EnorDi : ENABLE or DISABLE macro.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
void I2C_PeripheralControl(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == ENABLE)
	{
		pI2Cx->CR1 |= (1 << I2C_CR1_PE);
	}
	else
	{
		pI2Cx->CR1 &= ~(1 << I2C_CR1_PE);
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_ManageAcking
 *
 * @brief				- Enables or disables automatic ACKing for the specified I2C peripheral.
 *
 * @param[in]			- pI2Cx  : Base address of the I2C peripheral (e.g., I2C1, I2C2, I2C3).
 * @param[in]			- EnorDi : I2C_ACK_ENABLE or I2C_ACK_DISABLE (or ENABLE/DISABLE).
 *
 * @return				- None.
 *
 * @note				- Toggles the ACK bit in the I2C_CR1 register.
 *************************************************************************************************************************/
void I2C_ManageAcking(I2C_RegDef_t *pI2Cx, uint8_t EnorDi)
{
	if(EnorDi == I2C_ACK_ENABLE)
	{
		//enable the ack
		pI2Cx->CR1 |= (1 << I2C_CR1_ACK);
	}else
	{
		//disable the ack
		pI2Cx->CR1 &= ~(1 << I2C_CR1_ACK);
	}
}

/*
 * Application callback
 */
/*************************************************************************************************************************
 * @fn 					- I2C_ApplicationEventCallback
 *
 * @brief				- Weak implementation of application event callback. Notifies application about I2C events/errors.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 * @param[in]			- AppEv      : Application event macro (e.g., I2C_EV_TX_CMPLT, I2C_ERROR_AF).
 *
 * @return				- None.
 *
 * @note				- Weak implementation. Should be overridden by user application if needed.
 *************************************************************************************************************************/
__weak void I2C_ApplicationEventCallback(I2C_Handle_t *pI2CHandle, uint8_t AppEv)
{
	//This is a weak implementation. the user application may overide this function.
}

/*************************************************************************************************************************
 * @fn 					- I2C_SlaveEnableDisableCallbackEvents
 *
 * @brief				- Enables or disables I2C event, buffer, and error interrupts for slave mode operation.
 *
 * @param[in]			- pI2Cx  : Base address of the I2C peripheral (e.g., I2C1, I2C2, I2C3).
 * @param[in]			- EnOrDi : ENABLE or DISABLE macro.
 *
 * @return				- None.
 *
 * @note				- Configures ITEVTEN, ITBUFEN, and ITERREN control bits in the I2C_CR2 register.
 *************************************************************************************************************************/
void I2C_SlaveEnableDisableCallbackEvents(I2C_RegDef_t *pI2Cx, uint8_t EnOrDi)
{
    if(EnOrDi == ENABLE)
    {
        pI2Cx->CR2 |= ( 1 << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 |= ( 1 << I2C_CR2_ITBUFEN);
        pI2Cx->CR2 |= ( 1 << I2C_CR2_ITERREN);
    }
    else
    {
        pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITEVTEN);
        pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITBUFEN);
        pI2Cx->CR2 &= ~( 1 << I2C_CR2_ITERREN);
    }
}

/*
 * Some helper function implementations
 */
uint32_t RCC_GetPLLOutputClock(void)
{
	//TODO
	return 0;
}

/*************************************************************************************************************************
 * @fn 					- RCC_GetPLCK1Value
 *
 * @brief				- Calculates and returns the APB1 bus clock frequency in Hz.
 *
 * @return				- PCLK1 clock frequency in Hz.
 *
 *************************************************************************************************************************/
uint32_t RCC_GetPLCK1Value(void)
{
	uint32_t pclk1, SystemClk = 0;
	uint8_t clksrc, temp, ahbp, apb1p;

	clksrc = ((RCC->CFGR >> 2) & 0x3);

	if(clksrc == 0)      { SystemClk = 16000000; } // HSI
	else if(clksrc == 1) { SystemClk = 8000000;  } // HSE
	else if(clksrc == 2) { SystemClk = RCC_GetPLLOutputClock(); }

	// AHB Prescaler
	temp = ((RCC->CFGR >> 4) & 0xF);
	if(temp < 8) { ahbp = 1; }
	else         { ahbp = AHB_PreScaler[temp - 8]; }

	// APB1 Prescaler
	temp = ((RCC->CFGR >> 10) & 0x7);
	if(temp < 4) { apb1p = 1; }
	else         { apb1p = APB1_PreScaler[temp - 4]; }

	pclk1 = (SystemClk / ahbp) / apb1p;

	return pclk1;
}

/*************************************************************************************************************************
 * @fn 					- I2C_GenerateStartCondition
 *
 * @brief				- Generates START condition on the I2C bus.
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_GenerateStartCondition(I2C_RegDef_t *pI2Cx)
{
	pI2Cx->CR1 |= ( 1 << I2C_CR1_START);
}

/*************************************************************************************************************************
 * @fn 					- I2C_ExecuteAddressPhaseWrite
 *
 * @brief				- Sends target slave address with Read/Write bit set to Write (0).
 *
 * @param[in]			- pI2Cx     : Base address of the I2C peripheral.
 * @param[in]			- SlaveAddr : 7-bit slave address.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_ExecuteAddressPhaseWrite(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr &= ~( 1 << 0);	//SlaveAddr is Slave address + r/nw bit=0;
	pI2Cx->DR = SlaveAddr;
}

/*************************************************************************************************************************
 * @fn 					- I2C_ExecuteAddressPhaseRead
 *
 * @brief				- Sends target slave address with Read/Write bit set to Read (1).
 *
 * @param[in]			- pI2Cx     : Base address of the I2C peripheral.
 * @param[in]			- SlaveAddr : 7-bit slave address.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_ExecuteAddressPhaseRead(I2C_RegDef_t *pI2Cx, uint8_t SlaveAddr)
{
	SlaveAddr = SlaveAddr << 1;
	SlaveAddr |= (1 << 0);	//SlaveAddr is Slave address + r/nw bit=1;
	pI2Cx->DR = SlaveAddr;
}

/*************************************************************************************************************************
 * @fn 					- I2C_ClearADDRFlag
 *
 * @brief				- Clears ADDR flag by reading SR1 followed by SR2 register.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_ClearADDRFlag(I2C_Handle_t *pI2CHandle)
{
	uint32_t dummy_read;
	//check for device mode
	if(pI2CHandle->pI2Cx->SR2 & ( 1 << I2C_SR2_MSL))
	{
		//device is in master mode
		if(pI2CHandle->TxRxState == I2C_BUSY_IN_RX)
		{
			if(pI2CHandle->RxSize == 1)
			{
				//first disable the ack
				I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);

				//clear the ADDR flag ( read SR1, read SR2)
				dummy_read = pI2CHandle->pI2Cx->SR1;
				dummy_read = pI2CHandle->pI2Cx->SR2;
				(void)dummy_read;

				if(pI2CHandle->Sr == I2C_DISABLE_SR)
				{
					I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
				}
			}else
			{
				if(pI2CHandle->I2C_Config.I2C_ACKControl == I2C_ACK_ENABLE)
				{
					I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_ENABLE);
				}
				//clear the ADDR flag ( read SR1, read SR2)
				dummy_read = pI2CHandle->pI2Cx->SR1;
				dummy_read = pI2CHandle->pI2Cx->SR2;
				(void)dummy_read;
			}
		}else
		{
			//clear the ADDR flag ( read SR1, read SR2)
			dummy_read = pI2CHandle->pI2Cx->SR1;
			dummy_read = pI2CHandle->pI2Cx->SR2;
			(void)dummy_read;
		}
	}else
	{
		//device is in slave mode
		//clear the ADDR flag ( read SR1, read SR2)
		dummy_read = pI2CHandle->pI2Cx->SR1;
		dummy_read = pI2CHandle->pI2Cx->SR2;
		(void)dummy_read;
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_GenerateStopCondition
 *
 * @brief				- Generates STOP condition on the I2C bus.
 *
 * @param[in]			- pI2Cx : Base address of the I2C peripheral.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_GenerateStopCondition(I2C_RegDef_t *pI2Cx)
{
	pI2Cx->CR1 |= ( 1 << I2C_CR1_STOP);
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterHandleTXEInterrupt
 *
 * @brief				- Handles the I2C TXE (Transmit Buffer Empty) interrupt during data transmission.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_MasterHandleTXEInterrupt(I2C_Handle_t *pI2CHandle)
{
	if(pI2CHandle->TxLen > 0)
	{
		//1. load the data in to DR
		pI2CHandle->pI2Cx->DR = *(pI2CHandle->pTxBuffer);

		//2. decrement the TxLen
		pI2CHandle->TxLen--;

		//3. Increment the buffer address
		pI2CHandle->pTxBuffer++;
	}
}

/*************************************************************************************************************************
 * @fn 					- I2C_MasterHandleRXNEInterrupt
 *
 * @brief				- Handles the I2C RXNE (Receive Buffer Not Empty) interrupt during data reception.
 *
 * @param[in]			- pI2CHandle : Pointer to I2C Handle structure.
 *
 * @return				- None.
 *
 *************************************************************************************************************************/
static void I2C_MasterHandleRXNEInterrupt(I2C_Handle_t *pI2CHandle)
{
	//We have to do the data reception
	if(pI2CHandle->RxSize == 1)
	{
		*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
		pI2CHandle->RxLen--;
		pI2CHandle->pRxBuffer++;
	}

	if(pI2CHandle->RxSize > 1)
	{
		if(pI2CHandle->RxLen == 2)
		{
			//clear the ack bit
			I2C_ManageAcking(pI2CHandle->pI2Cx, I2C_ACK_DISABLE);
		}

		if(pI2CHandle->RxLen == 1)
		{
			if(pI2CHandle->Sr == I2C_DISABLE_SR)
			{
				I2C_GenerateStopCondition(pI2CHandle->pI2Cx);
			}
		}

		//read DR
		*pI2CHandle->pRxBuffer = pI2CHandle->pI2Cx->DR;
		pI2CHandle->pRxBuffer++;
		pI2CHandle->RxLen--;
	}

	if(pI2CHandle->RxLen == 0)
	{
		//close the I2C data reception and notify the application

		//1. Close the I2C rx
		I2C_CloseReceiveData(pI2CHandle);

		//2. notify the application
		I2C_ApplicationEventCallback(pI2CHandle, I2C_EV_RX_CMPLT);
	}
}
