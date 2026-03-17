/**
 * @file fs_port_littlefs.h
 * @brief File system abstraction layer (littleFS)
 *
 * This port plugs into CycloneTCP's fs_port.h selection logic when USE_LITTLEFS
 * is defined at build time.
 *
 * Note: This module expects the littleFS headers to be available in the include
 * path (e.g., lfs.h).
 **/

#ifndef _FS_PORT_LITTLEFS_H
#define _FS_PORT_LITTLEFS_H

//Dependencies
#include "os_port.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief File descriptor
 **/
typedef void FsFile;

/**
 * @brief Directory descriptor
 **/
typedef void FsDir;

//File system abstraction layer
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

