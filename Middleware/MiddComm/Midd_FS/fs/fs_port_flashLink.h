/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 20 Mar 2026 - 14:21:51
* #File Name    : AppSWUpdate.c
*******************************************************************************/
/******************************************************************************
 *  - [ 64] name (ASCII, zero-terminated, left as 0xFF when unused)
 *  - [  2] next cell index (stored as ~nextIndex; erased 0xFFFF means nextIndex=0)
 *  - [  2] data length in this cell (stored as ~dataLen; erased 0xFFFF means dataLen=0)
* This file implements the flashLink storage in MCU internal flash. 
* The flashLink storage is a simple linked-cell storage in MCU internal flash.
* Example usage:
* FlashLinkOps ops = {
*  .read = myFlashRead,
*  .prog = myFlashProg,
*  .erase = myFlashErase,
*  .sync = myFlashSync
* };
* FlashLinkConfig cfg = {
*  .baseAddr = 0x080A0000,
*  .regionSize = 128 * 1024,
*  .cellCount = 1024,
*  .eraseBlockSize = 4 * 1024
* };
* flashLinkInit(&ops, &cfg);
* flashLinkFormat();
* fsInit();
* fsFileExists("test.txt");
* fsOpenFile("test.txt", FS_FILE_MODE_WRITE);
* fsWriteFile("test.txt", "Hello, world!", 13);
* fsCloseFile("test.txt");
*******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef _FS_PORT_FLASHLINK_H
#define _FS_PORT_FLASHLINK_H

/*********************************INCLUDES*************************************/
#include "Midd_OSPort.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************MACRO DEFINITIONS*******************************/
#define FLASHLINK_MAX_OPEN_FILES  (10)
#define FLASHLINK_MAX_OPEN_DIRS   (5)

/*******************************TYPE DEFINITIONS ******************************/
typedef struct
{
   error_t (*read)(uint32_t addr, void *data, size_t len);
   error_t (*prog)(uint32_t addr, const void *data, size_t len);
   error_t (*erase)(uint32_t addr);
   error_t (*sync)(void);
} FlashLinkOps;

typedef struct
{
   uint32_t baseAddr;   ///< Start of region in flash
   uint32_t regionSize; ///< Total bytes reserved for cells (must be multiple of 196)
   uint16_t cellCount;  ///< Optional; if 0, computed as regionSize / 196
   uint32_t eraseBlockSize; ///< Optional; if non-zero, can be used to format/erase region safely
} FlashLinkConfig;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
/**
 * @brief Initialize FlashLink storage.
 */
error_t flashLinkInit(const FlashLinkOps *ops, const FlashLinkConfig *cfg);

/**
 * @brief Erase the whole FlashLink region (format).
 * @note Requires ops->erase and cfg->eraseBlockSize.
 */
error_t flashLinkFormat(void);

/**
Mod	                 Dosya	     Sonuç
READ	                  Var	      OK
READ	                  Yok	      NULL
WRITE	                  Var	      OK (append)
WRITE	                  Yok	      NULL
WRITE | CREATE	         Yok	      OK (created)
WRITE | CREATE | TRUNC	Var	      OK (önce silinir, sonra yazılır)
CREATE tek başına	       -	         NULL (READ/WRITE yok)
*/

/**
Mod	                       Dosya var	              Dosya yok
WRITE | CREATE | TRUNC	 Sil → sıfırdan yaz	    Handle ver → sıfırdan yaz
WRITE | TRUNC	          Sil → sıfırdan yaz	    NULL
WRITE | CREATE	          Append	                Yeni dosya oluştur
WRITE	                   Append	                NULL
*/
typedef void FsFile;
typedef void FsDir;

error_t fsInit(void);

bool_t fsFileExists(const char_t *path);
error_t fsGetFileSize(const char_t *path, uint32_t *size);
error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat);
error_t fsRenameFile(const char_t *oldPath, const char_t *newPath);
error_t fsDeleteFile(const char_t *path);

FsFile *fsOpenFile(const char_t *path, uint_t mode);
error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin);
error_t fsWriteFile(FsFile *file, void *data, size_t length);
error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length);
void fsCloseFile(FsFile *file);

bool_t fsDirExists(const char_t *path);
error_t fsCreateDir(const char_t *path);
error_t fsRemoveDir(const char_t *path);

FsDir *fsOpenDir(const char_t *path);
error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry);
void fsCloseDir(FsDir *dir);

/* example usage /
FlashLinkOps ops = { ... };
FlashLinkConfig cfg = { ... };
flashLinkInit(&ops, &cfg);
fsInit();

FsFile *f = fsOpenFile("config.bin", FS_FILE_MODE_READ);
// veya
FsFile *f = fsOpenFile("/config.bin", FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);
*/


#ifdef __cplusplus
}
#endif

#endif
/********************************* End Of File ********************************/
