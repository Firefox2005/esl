/********************************************************************************************************
 * @file     flash.c
 *
 * @brief    This file provides set of driver functions to read and write the internal flash
 *
 * @author   jian.zhang@telink-semi.com
 * @date     Oct. 8, 2016
 *
 * @par      Copyright (c) 2016, Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *           The information contained herein is confidential property of Telink 
 *           Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *           of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *           Co., Ltd. and the licensee or the terms described here-in. This heading 
 *           MUST NOT be removed from this file.
 *
 *           Licensees are granted free, non-transferable use of the information in this 
 *           file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
#include "flash.h"
#include "mspi.h"

#define FLASH_IRQ_EN    REG_ADDR8(0x643)

__attribute__((section(".ram_code"))) static inline int FLASH_IsBusy()
{
    return MSPI_Read() & 0x01;              //  the busy bit, pls check flash spec
}

__attribute__((section(".ram_code"))) static void FLASH_CmdSend(unsigned char cmd)
{
    MSPI_High();
    WaitUs(1);
    MSPI_Low();
    MSPI_Write(cmd);
    MSPI_Wait();
}

__attribute__((section(".ram_code"))) static void FLASH_AddrSend(unsigned int addr)
{
    MSPI_Write((unsigned char)(addr>>16));
    MSPI_Wait();
    MSPI_Write((unsigned char)(addr>>8));
    MSPI_Wait();
    MSPI_Write((unsigned char)(addr));
    MSPI_Wait();
}

//  make this a asynchorous version
__attribute__((section(".ram_code"))) static void FLASH_WaitDone()
{
    WaitUs(100);
    FLASH_CmdSend(FLASH_READ_STATUS_CMD);

    int i;
    for (i = 0; i < 10000000; ++i) {
        if (!FLASH_IsBusy()) {
            break;
        }
    }
    MSPI_High();
}

/**
 * @brief This function serves to erase a sector.
 * @param[in]   addr the start address of the sector needs to erase.
 * @return none
 */
__attribute__((section(".ram_code"))) void FLASH_SectorErase(unsigned int addr)
{
    unsigned char r = FLASH_IRQ_EN; //disable the irq
    FLASH_IRQ_EN = 0;

    FLASH_CmdSend(FLASH_WRITE_ENABLE_CMD);
    FLASH_CmdSend(FLASH_SECT_ERASE_CMD);
    FLASH_AddrSend(addr);
    MSPI_High();
    FLASH_WaitDone();
    
    FLASH_IRQ_EN = r; //restore the irq
}

/**
 * @brief This function writes the buffer's content to a page.
 * @param[in]   addr the start address of the page
 * @param[in]   len the length(in byte) of content needs to write into the page
 * @param[in]   buf the start address of the content needs to write into
 * @return none
 */
__attribute__((section(".ram_code"))) void FLASH_PageWrite(unsigned int addr, unsigned int len, unsigned char *buf)
{
    unsigned char r = FLASH_IRQ_EN; //disable the irq
    FLASH_IRQ_EN = 0;

    // important:  buf must not reside at flash, such as constant string.  If that case, pls copy to memory first before write
    FLASH_CmdSend(FLASH_WRITE_ENABLE_CMD);
    FLASH_CmdSend(FLASH_WRITE_CMD);
    FLASH_AddrSend(addr);

    unsigned int i;
    for(i = 0; i < len; ++i){
        MSPI_Write(buf[i]);     /* write data */
        MSPI_Wait();
    }
    MSPI_High();
    FLASH_WaitDone();
    
    FLASH_IRQ_EN = r; //restore the irq
}

/**
 * @brief This function reads the content from a page to the buf.
 * @param[in]   addr the start address of the page
 * @param[in]   len the length(in byte) of content needs to read out from the page
 * @param[out]  buf the start address of the buffer
 * @return none
 */
__attribute__((section(".ram_code"))) void FLASH_PageRead(unsigned int addr, unsigned int len, unsigned char *buf)
{
    unsigned char r = FLASH_IRQ_EN; //disable the irq
    FLASH_IRQ_EN = 0;

    FLASH_CmdSend(FLASH_READ_CMD);
    FLASH_AddrSend(addr);

    MSPI_Write(0x00);       /* dummy,  to issue clock */
    MSPI_Wait();
    MSPI_CtrlWrite(0x0a);  /* auto mode */
    MSPI_Wait();

    /* get data */
    int i = 0;
    for (i = 0; i < len; ++i) {
        *buf++ = MSPI_Get();
        MSPI_Wait();
    }
    MSPI_High();

    FLASH_IRQ_EN = r; //restore the irq
}

/***********************************
 * @brief  read flash ID
 * @param[in] the buffer address to store the flash ID.
 * @return none.
 */
__attribute__((section(".ram_code"))) void FLASH_GetID(unsigned int* FLASH_ID)
{
    unsigned char r = FLASH_IRQ_EN; //disable the irq

    FLASH_CmdSend(FLASH_GET_JEDEC_ID);  //FLASH_GET_JEDEC_ID
    unsigned char manufacturer = MSPI_Read();
    unsigned char mem_type = MSPI_Read();
    unsigned char cap_id = MSPI_Read();

    *FLASH_ID = (unsigned int)(manufacturer<<24|mem_type<<16|cap_id<<8);

    MSPI_High();
    FLASH_IRQ_EN = r; //restore the irq
}
