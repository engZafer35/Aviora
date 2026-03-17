/**
 * @file StorageFlashPort_flashLink_stm32f407.c
 * @brief STM32F407 internal flash callbacks for fs_port_flashLink
 *
 * Notes:
 * - STM32F4 flash programming supports 1->0 only. Any 0->1 requires sector erase.
 * - `fs_port_flashLink` was adapted to avoid in-place "increment" updates by using
 *   inverted encoding for nextIndex and dataLen.
 */

#include "StorageFlashPort_flashLink_stm32f407.h"

#include "stm32f4xx_hal.h"
#include <string.h>

static error_t prvHalToError(HAL_StatusTypeDef st)
{
   return (st == HAL_OK) ? NO_ERROR : ERROR_FAILURE;
}

static uint32_t prvGetSectorForAddress(uint32_t address)
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

error_t Storage_FlashLinkRead(uint32_t addr, void *data, size_t len)
{
   if(data == NULL || len == 0)
      return ERROR_INVALID_PARAMETER;

   // Internal flash is memory mapped
   memcpy(data, (const void *)addr, len);
   return NO_ERROR;
}

error_t Storage_FlashLinkProg(uint32_t addr, const void *data, size_t len)
{
   const uint8_t *p = (const uint8_t *)data;
   HAL_StatusTypeDef st;
   size_t i;

   if(data == NULL || len == 0)
      return ERROR_INVALID_PARAMETER;

   // We program 32-bit words on STM32F4
   if((addr % 4u) != 0u || (len % 4u) != 0u)
      return ERROR_INVALID_PARAMETER;

   st = HAL_FLASH_Unlock();
   if(st != HAL_OK)
      return prvHalToError(st);

   for(i = 0; i < len; i += 4u)
   {
      uint32_t word;
      memcpy(&word, p + i, 4u);

      st = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + (uint32_t)i, (uint64_t)word);
      if(st != HAL_OK)
      {
         (void)HAL_FLASH_Lock();
         return prvHalToError(st);
      }
   }

   st = HAL_FLASH_Lock();
   return prvHalToError(st);
}

error_t Storage_FlashLinkErase(uint32_t addr)
{
   FLASH_EraseInitTypeDef erase;
   uint32_t sectorError = 0;
   HAL_StatusTypeDef st;

   uint32_t sector = prvGetSectorForAddress(addr);

   st = HAL_FLASH_Unlock();
   if(st != HAL_OK)
      return prvHalToError(st);

   memset(&erase, 0, sizeof(erase));
   erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
   erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
   erase.Sector       = sector;
   erase.NbSectors    = 1;

   st = HAL_FLASHEx_Erase(&erase, &sectorError);

   (void)HAL_FLASH_Lock();

   return (st == HAL_OK) ? NO_ERROR : ERROR_FAILURE;
}

error_t Storage_FlashLinkSync(void)
{
   return NO_ERROR;
}

