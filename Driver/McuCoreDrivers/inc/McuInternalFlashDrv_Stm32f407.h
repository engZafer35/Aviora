/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 21 Mar 2026 - 18:46:22
* #File Name    : McuInternalFlashDrv_Stm32f407.c
*******************************************************************************/
/******************************************************************************
* This file implements stm32f407 internal flash driver. 
* The stm32f407 internal flash driver is used to read/write/erase/sync the internal flash.
* The stm32f407 internal flash driver is used to read/write/erase/sync the internal flash.
* Example usage:
* Storage_FlashLinkRead(0x080A0000, data, 1024);
* Storage_FlashLinkProg(0x080A0000, data, 1024);
* Storage_FlashLinkErase(0x080A0000);
* Storage_FlashLinkSync();
*******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __MCU_INTERNAL_FLASH_DRV_STM32F407_H__
#define __MCU_INTERNAL_FLASH_DRV_STM32F407_H__

/*********************************INCLUDES*************************************/

#include "stdint.h"
#include "stddef.h"
#ifdef __cplusplus
extern "C" {
#endif
/******************************MACRO DEFINITIONS*******************************/
/** driver functions for flash. Each flash driver has its own implementation */
#define STORAGE_DRV_READ_FUNC    internalStm32f407FlashRead
#define STORAGE_DRV_PROG_FUNC    internalStm32f407FlashProg
#define STORAGE_DRV_ERASE_FUNC   internalStm32f407FlashErase
#define STORAGE_DRV_SYNC_FUNC    internalStm32f407FlashSync
/** end of driver functions for flash */
/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/** 
 * @brief Read data from internal flash.
 * @param addr Address to read from.
 * @param data Pointer to the data buffer.
 * @param len Length of the data to read.
 * @return 0 if successful, otherwise an error code.
 */
int internalStm32f407FlashRead(unsigned int addr, void *data, size_t len);
/**
 * @brief Program data to internal flash.
 * @param addr Address to program to.
 * @param data Pointer to the data buffer.
 * @param len Length of the data to program.
 * @return 0 if successful, otherwise an error code.
 */
int internalStm32f407FlashProg(unsigned int addr, const void *data, size_t len);

/**
 * @brief Erase data from internal flash.
 * @param addr Address to erase from.
 * @return 0 if successful, otherwise an error code.
 */
int internalStm32f407FlashErase(unsigned int addr);

/**
 * @brief Sync data to internal flash.
 * @return 0 if successful, otherwise an error code.
 */
int internalStm32f407FlashSync(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCU_INTERNAL_FLASH_DRV_STM32F407_H__ */

/********************************* End Of File ********************************/