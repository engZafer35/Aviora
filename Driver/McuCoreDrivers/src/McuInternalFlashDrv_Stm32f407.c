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
/********************************* INCLUDES ***********************************/
#include "McuInternalFlashDrv_Stm32f407.h"

#include "stm32f4xx_hal.h"
#include <string.h>
#include "tim.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
static unsigned int prvGetSectorForAddress(unsigned int address)
{
   // STM32F407 (1MB) sector mapping
   if(address < 0x08004000u) return FLASH_SECTOR_0;
   if(address < 0x08008000u) return FLASH_SECTOR_1;
   if(address < 0x0800C000u) return FLASH_SECTOR_2;
   if(address < 0x08010000u) return FLASH_SECTOR_3;
   if(address < 0x08020000u) return FLASH_SECTOR_4;
   if(address < 0x08040000u) return FLASH_SECTOR_5;
   if(address < 0x08060000u) return FLASH_SECTOR_6;
   if(address < 0x08080000u) return FLASH_SECTOR_7;
   if(address < 0x080A0000u) return FLASH_SECTOR_8;
   if(address < 0x080C0000u) return FLASH_SECTOR_9;
   if(address < 0x080E0000u) return FLASH_SECTOR_10;
   return FLASH_SECTOR_11;
}

/***************************** PUBLIC FUNCTIONS  ******************************/
int internalStm32f407FlashRead(unsigned int addr, void *data, size_t len)
{
   if(data == NULL || len == 0)
      return HAL_ERROR;

   // Internal flash is memory mapped
   memcpy(data, (const void *)addr, len);
   return HAL_OK;
}

int internalStm32f407FlashProg(unsigned int addr, const void *data, size_t len)
{
   const uint8_t *p = (const uint8_t *)data;
   HAL_StatusTypeDef st;
   size_t i;

   if(data == NULL || len == 0)
      return HAL_ERROR;

   // We program 32-bit words on STM32F4
   if((addr % 4u) != 0u || (len % 4u) != 0u)
      return HAL_ERROR;

   HAL_TIM_Base_Stop_IT(&htim6);
   HAL_TIM_Base_Stop_IT(&htim7);

   __disable_irq();

   st = HAL_FLASH_Unlock();
   if(st != HAL_OK)
      return st;

   for(i = 0; i < len; i += 4u)
   {
      uint32_t word;
      memcpy(&word, p + i, 4u);

      st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + (uint32_t)i, (uint64_t)word);
      if(st != HAL_OK)
      {
          break;
      }
   }

   __enable_irq();

   HAL_FLASH_Lock();

   HAL_TIM_Base_Start_IT(&htim6);
   HAL_TIM_Base_Start_IT(&htim7);
   return st;
}


int internalStm32f407FlashErase(unsigned int addr)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sectorError = 0;
    HAL_StatusTypeDef st;

    uint32_t sector = prvGetSectorForAddress(addr);

    HAL_TIM_Base_Stop_IT(&htim6);
    HAL_TIM_Base_Stop_IT(&htim7);

    __disable_irq();


    HAL_FLASH_Unlock();

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP |
                           FLASH_FLAG_OPERR |
                           FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGSERR);

    memset(&erase, 0, sizeof(erase));
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = sector;
    erase.NbSectors = 1;


    st = HAL_FLASHEx_Erase(&erase, &sectorError);
    __enable_irq();

    HAL_FLASH_Lock();

    HAL_TIM_Base_Start_IT(&htim6);
    HAL_TIM_Base_Start_IT(&htim7);

    return st;
}

int internalStm32f407FlashSync(void)
{
   return HAL_OK;
}

/******************************** End Of File *********************************/
