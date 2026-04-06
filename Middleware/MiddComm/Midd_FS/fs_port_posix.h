

#ifndef _FS_PORT_POSIX_H
#define _FS_PORT_POSIX_H

//Dependencies
#include "Midd_OSPort.h"

//Maximum path length
#ifndef FS_MAX_PATH_LEN
   #define FS_MAX_PATH_LEN 260
#elif (FS_MAX_PATH_LEN < 1)
   #error FS_MAX_PATH_LEN parameter is not valid
#endif

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

typedef struct
{
   void *handle;
   char path[FS_MAX_PATH_LEN + 1];
} FsDir;


//File system abstraction layer
error_t fsInit(void);

BOOL fsFileExists(const char *path);
error_t fsGetFileSize(const char *path, U32 *size);
error_t fsGetFileStat(const char *path, FsFileStat *fileStat);
error_t fsRenameFile(const char *oldPath, const char *newPath);
error_t fsDeleteFile(const char *path);

FsFile *fsOpenFile(const char *path, U32 mode);
error_t fsSeekFile(FsFile *file, S32 offset, U32 origin);
error_t fsWriteFile(FsFile *file, void *data, size_t length);
error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length);
void fsCloseFile(FsFile *file);

BOOL fsDirExists(const char *path);
error_t fsCreateDir(const char *path);
error_t fsRemoveDir(const char *path);

FsDir *fsOpenDir(const char *path);
error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry);
void fsCloseDir(FsDir *dir);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
