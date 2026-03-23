/**
 * @file StorageFlashPort_littlefs.c
 * @brief littleFS block device hooks (stub)
 **/

#include "StorageFlashPort_littlefs.h"


int Storage_Init(void)
{
   return 0; //success
}

int Storage_FlashRead(const struct lfs_config *c, lfs_block_t block,
   lfs_off_t off, void *buffer, lfs_size_t size)
{
   (void) c;
   (void) block;
   (void) off;
   (void) buffer;
   (void) size;
   return LFS_ERR_IO;
}

int Storage_FlashProg(const struct lfs_config *c, lfs_block_t block,
   lfs_off_t off, const void *buffer, lfs_size_t size)
{
   (void) c;
   (void) block;
   (void) off;
   (void) buffer;
   (void) size;
   return LFS_ERR_IO;
}

int Storage_FlashErase(const struct lfs_config *c, lfs_block_t block)
{
   (void) c;
   (void) block;
   return LFS_ERR_IO;
}

int Storage_FlashSync(const struct lfs_config *c)
{
   (void) c;
   return 0;
}

