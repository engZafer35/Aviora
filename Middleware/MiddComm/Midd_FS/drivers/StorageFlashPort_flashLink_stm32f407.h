/**
 * @file StorageFlashPort_flashLink_stm32f407.h
 * @brief STM32F407 internal flash callbacks for fs_port_flashLink
 */

#ifndef STORAGE_FLASH_PORT_FLASHLINK_STM32F407_H
#define STORAGE_FLASH_PORT_FLASHLINK_STM32F407_H

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

error_t Storage_FlashLinkRead(uint32_t addr, void *data, size_t len);
error_t Storage_FlashLinkProg(uint32_t addr, const void *data, size_t len);
error_t Storage_FlashLinkErase(uint32_t addr);
error_t Storage_FlashLinkSync(void);

#ifdef __cplusplus
}
#endif

#endif

