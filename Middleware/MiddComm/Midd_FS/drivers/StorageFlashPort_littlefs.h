/**
 * @file StorageFlashPort_littlefs.h
 * @brief littleFS block device hooks (platform-specific)
 *
 * These functions must be implemented to access the real external memory.
 * For now, they are provided as stubs.
 **/

#ifndef STORAGE_FLASH_PORT_LITTLEFS_H
#define STORAGE_FLASH_PORT_LITTLEFS_H

#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

int Storage_FlashRead(const struct lfs_config *c, lfs_block_t block,
   lfs_off_t off, void *buffer, lfs_size_t size);

int Storage_FlashProg(const struct lfs_config *c, lfs_block_t block,
   lfs_off_t off, const void *buffer, lfs_size_t size);

int Storage_FlashErase(const struct lfs_config *c, lfs_block_t block);

int Storage_FlashSync(const struct lfs_config *c);

#ifdef __cplusplus
}
#endif

#endif

