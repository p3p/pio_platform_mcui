#ifdef __LPC177X_8X__

/**********************************************************************
* $Id$		lpc177x_8x_eeprom.c			2011-06-02
*//**
* @file		lpc177x_8x_eeprom.c
* @brief	Contains all functions support for EEPROM firmware library on
*			LPC177x_8x
* @version	1.0
* @date		02. June. 2011
* @author	NXP MCU SW Application Team
* 
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/

/* Peripheral group ----------------------------------------------------------- */
/** @addtogroup EEPROM
 * @{
 */
#ifdef __BUILD_WITH_EXAMPLE__
#include "lpc177x_8x_libcfg.h"
#else
#include "lpc177x_8x_libcfg_default.h"
#endif /* __BUILD_WITH_EXAMPLE__ */
#ifdef _EEPROM
 
/* Includes ------------------------------------------------------------------- */
#include "lpc177x_8x_eeprom.h"
#include "lpc177x_8x_clkpwr.h"

/* Public Functions ----------------------------------------------------------- */

/*********************************************************************//**
 * @brief 		Initial EEPROM
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void EEPROM_Init(void)
{
	uint32_t val, cclk;
	LPC_EEPROM->PWRDWN = 0x0;
	/* EEPROM is automate turn on after reset */
	/* Setting clock:
	 * EEPROM required a 375kHz. This clock is generated by dividing the
	 * system bus clock.
	 */
   	cclk = CLKPWR_GetCLK(CLKPWR_CLKTYPE_CPU);
	val = (cclk/375000)-1;
	LPC_EEPROM->CLKDIV = val;

	/* Setting wait state */
	val  = ((((cclk / 1000000) * 15) / 1000) + 1);
	val |= (((((cclk / 1000000) * 55) / 1000) + 1) << 8);
	val |= (((((cclk / 1000000) * 35) / 1000) + 1) << 16);
	LPC_EEPROM->WSTATE = val;
}

/*********************************************************************//**
 * @brief 		Write data to EEPROM at specific address
 * @param[in]	page_offset offset of data in page register(0 - 63)
 *              page_address page address (0-62)                    
 * 				mode	Write mode, should be:
 * 					- MODE_8_BIT	: write 8 bit mode
 * 					- MODE_16_BIT	: write 16 bit mode
 * 					- MODE_32_BIT	: write 32 bit mode
 * 				data	buffer that contain data that will be written to buffer
 * 				count	number written data
 * @return 		None
 * @note		This function actually write data into EEPROM memory and automatically
 * 				write into next page if current page is overflowed
 **********************************************************************/
void EEPROM_Write(uint16_t page_offset, uint16_t page_address, void* data, EEPROM_Mode_Type mode, uint32_t count)
{
    uint32_t i;
    uint8_t *tmp8 = (uint8_t *)data;
    uint16_t *tmp16 = (uint16_t *)data;
    uint32_t *tmp32 = (uint32_t *)data;

	LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW)|(1 << EEPROM_ENDOF_PROG));
	//check page_offset
	if(mode == MODE_16_BIT){
		if((page_offset & 0x01)!=0) while(1);
	}
	else if(mode == MODE_32_BIT){
	 	if((page_offset & 0x03)!=0) while(1);
	}
	LPC_EEPROM->ADDR = EEPROM_PAGE_OFFSET(page_offset);
	for(i=0;i<count;i++)
	{
		//update data to page register
		if(mode == MODE_8_BIT){
			LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_WRITE;
			LPC_EEPROM -> WDATA = *tmp8;
			tmp8++;
			page_offset +=1;
		}
		else if(mode == MODE_16_BIT){
			LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_WRITE;
			LPC_EEPROM -> WDATA = *tmp16;
			tmp16++;
			page_offset +=2;
		}
		else{
			LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_WRITE;
			LPC_EEPROM -> WDATA = *tmp32;
			tmp32++;
			page_offset +=4;
		}
		while(!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW)&0x01));
		LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
		if((page_offset >= EEPROM_PAGE_SIZE)|(i==count-1)){
			//update to EEPROM memory
			LPC_EEPROM->INT_CLR_STATUS = (0x1 << EEPROM_ENDOF_PROG);
			LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_address);
			LPC_EEPROM->CMD = EEPROM_CMD_ERASE_PRG_PAGE;
			while(!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_PROG)&0x01));
			LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_PROG);
		}
		if(page_offset >= EEPROM_PAGE_SIZE)
		{
			page_offset = 0;
			page_address +=1;
			LPC_EEPROM->ADDR =0;
			if(page_address > EEPROM_PAGE_NUM - 1) page_address = 0;
		}
	}
}

/*********************************************************************//**
 * @brief 		Read data to EEPROM at specific address
 * @param[in]
 * 				data	buffer that contain data that will be written to buffer
 * 				mode	Read mode, should be:
 * 					- MODE_8_BIT	: read 8 bit mode
 * 					- MODE_16_BIT	: read 16 bit mode
 * 					- MODE_32_BIT	: read 32 bit mode
 * 				count	number read data (bytes)
 * @return 		data	buffer that contain data that will be read to buffer
 **********************************************************************/
void EEPROM_Read(uint16_t page_offset, uint16_t page_address, void* data, EEPROM_Mode_Type mode, uint32_t count)
{
        uint32_t i;
	uint8_t *tmp8 = (uint8_t *)data;
	uint16_t *tmp16 = (uint16_t *)data;
	uint32_t *tmp32 = (uint32_t *)data;

	LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW)|(1 << EEPROM_ENDOF_PROG));
	LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_address)|EEPROM_PAGE_OFFSET(page_offset);
	if(mode == MODE_8_BIT)
		LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_READ|EEPROM_CMD_RDPREFETCH;
	else if(mode == MODE_16_BIT){
		LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_READ|EEPROM_CMD_RDPREFETCH;
		//check page_offset
		if((page_offset &0x01)!=0)
			return;
	}
	else{
		LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_READ|EEPROM_CMD_RDPREFETCH;
		//page_offset must be a multiple of 0x04
		if((page_offset & 0x03)!=0)
			return;
	}

	//read and store data in buffer
	for(i=0;i<count;i++){
		 
		 if(mode == MODE_8_BIT){
			 *tmp8 = (uint8_t)(LPC_EEPROM -> RDATA);
			 tmp8++;
			 page_offset +=1;
		 }
		 else if (mode == MODE_16_BIT)
		 {
			 *tmp16 =  (uint16_t)(LPC_EEPROM -> RDATA);
			 tmp16++;
			 page_offset +=2;
		 }
		 else{
			 *tmp32 = (uint32_t)(LPC_EEPROM ->RDATA);
			 tmp32++;
			 page_offset +=4;
		 }
		 while(!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW)&0x01));
                 LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
		 if((page_offset >= EEPROM_PAGE_SIZE) && (i < count - 1)) {
			 page_offset = 0;
			 page_address++;
			 LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_address)|EEPROM_PAGE_OFFSET(page_offset);
			 if(mode == MODE_8_BIT)
			 	LPC_EEPROM->CMD = EEPROM_CMD_8_BIT_READ|EEPROM_CMD_RDPREFETCH;
			 else if(mode == MODE_16_BIT)
				LPC_EEPROM->CMD = EEPROM_CMD_16_BIT_READ|EEPROM_CMD_RDPREFETCH;
			 else
			 	LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_READ|EEPROM_CMD_RDPREFETCH;
		 }
	}
}

/*********************************************************************//**
 * @brief 		Erase a page at the specific address
 * @param[in]	address EEPROM page address (0-62)
 * @return 		data	buffer that contain data that will be read to buffer
 **********************************************************************/
void EEPROM_Erase(uint16_t page_address)
{
	uint32_t i;
    uint32_t count = EEPROM_PAGE_SIZE/4;

    LPC_EEPROM->INT_CLR_STATUS = ((1 << EEPROM_ENDOF_RW)|(1 << EEPROM_ENDOF_PROG));  

	//clear page register
    LPC_EEPROM->ADDR = EEPROM_PAGE_OFFSET(0);
	LPC_EEPROM->CMD = EEPROM_CMD_32_BIT_WRITE;
	for(i=0;i<count;i++)
	{
		LPC_EEPROM->WDATA = 0;
		while(!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_RW)&0x01));
		LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_RW);
	}

    LPC_EEPROM->INT_CLR_STATUS = (0x1 << EEPROM_ENDOF_PROG);
	LPC_EEPROM->ADDR = EEPROM_PAGE_ADRESS(page_address);
	LPC_EEPROM->CMD = EEPROM_CMD_ERASE_PRG_PAGE;
	while(!((LPC_EEPROM->INT_STATUS >> EEPROM_ENDOF_PROG)&0x01));
	LPC_EEPROM->INT_CLR_STATUS = (1 << EEPROM_ENDOF_PROG);
}

/*********************************************************************//**
 * @brief 		Enable/Disable EEPROM power down mdoe
 * @param[in]	NewState	PowerDown mode state, should be:
 * 					- ENABLE: Enable power down mode
 * 					- DISABLE: Disable power down mode
 * @return 		None
 **********************************************************************/
void EEPROM_PowerDown(FunctionalState NewState)
{
	if(NewState == ENABLE)
		LPC_EEPROM->PWRDWN = 0x1;
	else
		LPC_EEPROM->PWRDWN = 0x0;
}

#endif /*_EEPROM*/

/**
 * @}
 */

/* --------------------------------- End Of File ------------------------------ */
#endif /* __LPC177X_8X__ */
