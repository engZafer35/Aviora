/**
 * @file fs_port_flashFS.h
 * @brief File system abstraction layer (simple internal FlashFS)
 *
 * Very small filesystem intended for MCU internal flash ranges.
 *
 * Design goals:
 * - Keep it extremely simple (flat namespace, no subdirectories)
 * - Hardware agnostic: flash read/prog/erase/sync provided by callbacks
 * - Append-only metadata + data, no garbage collection (format when full)
 *
 * Enable with USE_FLASHFS.
 **/

#ifndef _FS_PORT_FLASHFS_H
#define _FS_PORT_FLASHFS_H

//Dependencies
#include "os_port.h"
#include "error.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------//
// Flash device callbacks
//----------------------------------------------------------------------------//

typedef struct
{
   /**
    * @brief Read bytes from flash
    * @param[in] addr Absolute flash address
    * @param[out] data Destination buffer
    * @param[in] len Number of bytes
    * @return Error code
    **/
   error_t (*read)(uint32_t addr, void *data, size_t len);

   /**
    * @brief Program bytes to flash (must respect target's programming rules)
    * @param[in] addr Absolute flash address
    * @param[in] data Source buffer
    * @param[in] len Number of bytes
    * @return Error code
    **/
   error_t (*prog)(uint32_t addr, const void *data, size_t len);

   /**
    * @brief Erase one erase-block containing addr (or erase at exact addr)
    * @param[in] addr Absolute flash address aligned to erase block
    * @return Error code
    **/
   error_t (*erase)(uint32_t addr);

   /**
    * @brief Optional sync / flush
    * @return Error code
    **/
   error_t (*sync)(void);
} FlashFsOps;

/**
 * @brief FlashFS geometry and address range
 **/
typedef struct
{
   uint32_t baseAddr;        // base address of flash region used for FlashFS
   uint32_t totalSize;       // total size of flash region used for FlashFS
   uint32_t eraseBlockSize;  // minimum erasable block size (e.g. sector size)
   uint32_t progMinSize; // minimum write size and alignment (e.g. page size)
} FlashFsGeom;

/**
 * @brief Provide hardware callbacks and geometry before fsInit()
 **/
error_t flashFsConfigure(const FlashFsOps *ops, const FlashFsGeom *geom);

/**
 * @brief Format the FlashFS region (erases whole region)
 **/
error_t flashFsFormat(void);

//----------------------------------------------------------------------------//
// CycloneTCP fs_port API (same signatures as POSIX port)
//----------------------------------------------------------------------------//

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

//C++ guard
#ifdef __cplusplus
}
#endif

#endif

