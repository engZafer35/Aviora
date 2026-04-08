/**
 * @file fs_port_littlefs.c
 * @brief File system abstraction layer (littleFS)
 **/

//Dependencies
#include <string.h>
#include "fs_port.h"
#include "fs_port_littlefs.h"
#include "str.h"
#include "error.h"
#include "Linux_DateTime.h"

//littleFS
#include "lfs.h"

// Internal state
static lfs_t g_lfs;
static struct lfs_config g_lfsCfg;
static bool_t g_lfsReady = FALSE;

// A very small helper to map lfs error to CycloneTCP error_t
static error_t fsMapLfsError(int err)
{
   if(err >= 0)
      return NO_ERROR;

   switch(err)
   {
   case LFS_ERR_NOENT:
      return ERROR_FILE_NOT_FOUND;
   case LFS_ERR_EXIST:
      return ERROR_ALREADY_EXISTS;
   case LFS_ERR_NOSPC:
      return ERROR_OUT_OF_RESOURCES;
   case LFS_ERR_INVAL:
      return ERROR_INVALID_PARAMETER;
   default:
      return ERROR_FAILURE;
   }
}

static void fsSetUnknownModified(DateTime *dt)
{
   if(dt == NULL)
      return;

   dt->year = 1970;
   dt->month = 1;
   dt->day = 1;
   dt->dayOfWeek = 0;
   dt->hours = 0;
   dt->minutes = 0;
   dt->seconds = 0;
   dt->milliseconds = 0;
}

error_t fsInit(void)
{
   int err;
   

   // Configure block-device callbacks (all geometry fields must be provided
   // by the application through fs_port_config.h or by editing below)
   osMemset(&lfsCfg, 0, sizeof(lfsCfg));

   g_lfsCfg.read_size = LITTLEFS_GEOMETRY_READ_SIZE;
   g_lfsCfg.prog_size = LITTLEFS_GEOMETRY_PROG_SIZE;
   g_lfsCfg.block_size = LITTLEFS_GEOMETRY_BLOCK_SIZE;
   g_lfsCfg.block_count = LITTLEFS_GEOMETRY_BLOCK_COUNT;
   g_lfsCfg.cache_size = LITTLEFS_GEOMETRY_CACHE_SIZE;
   g_lfsCfg.lookahead_size = LITTLEFS_GEOMETRY_LOOKAHEAD_SIZE;
   g_lfsCfg.block_cycles = LITTLEFS_GEOMETRY_BLOCK_CYCLES;
   g_lfsCfg.name_max = LITTLEFS_GEOMETRY_NAME_MAX;
   g_lfsCfg.file_max = LITTLEFS_GEOMETRY_FILE_MAX;
   g_lfsCfg.attr_max = LITTLEFS_GEOMETRY_ATTR_MAX;
   g_lfsCfg.base_addr = LITTLEFS_GEOMETRY_BASE_ADDR;
   
   g_lfsCfg.mutex_lock = LITTLEFS_MUTEX_LOCK;
   g_lfsCfg.mutex_unlock = LITTLEFS_MUTEX_UNLOCK;

   g_lfsCfg.read = STORAGE_DRV_FUNC_READ;
   g_lfsCfg.prog = STORAGE_DRV_FUNC_PROG;
   g_lfsCfg.erase = STORAGE_DRV_FUNC_ERASE;
   g_lfsCfg.sync = STORAGE_DRV_FUNC_SYNC;

   // Mount the filesystem
   err = lfs_mount(&g_lfs, &g_lfsCfg);
   if(err < 0)
   {
      // If no filesystem is found, format then retry mount
      err = lfs_format(&g_lfs, &g_lfsCfg);
      if(err < 0)
         return fsMapLfsError(err);

      err = lfs_mount(&g_lfs, &g_lfsCfg);
      if(err < 0)
         return fsMapLfsError(err);
   }

   g_lfsReady = TRUE;
   return NO_ERROR;
}

bool_t fsFileExists(const char_t *path)
{
   int err;
   struct lfs_info info;

   if(!g_lfsReady || path == NULL)
      return FALSE;

   err = lfs_stat(&g_lfs, path, &info);
   if(err < 0)
      return FALSE;

   return (info.type == LFS_TYPE_REG);
}

error_t fsGetFileSize(const char_t *path, uint32_t *size)
{
   error_t error;
   FsFileStat st;

   if(size == NULL)
      return ERROR_INVALID_PARAMETER;

   error = fsGetFileStat(path, &st);
   if(!error)
      *size = st.size;

   return error;
}

error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat)
{
   int err;
   struct lfs_info info;

   if(!g_lfsReady || path == NULL || fileStat == NULL)
      return ERROR_INVALID_PARAMETER;

   err = lfs_stat(&g_lfs, path, &info);
   if(err < 0)
      return fsMapLfsError(err);

   osMemset(fileStat, 0, sizeof(FsFileStat));
   if(info.type == LFS_TYPE_DIR)
      fileStat->attributes |= FS_FILE_ATTR_DIRECTORY;

   fileStat->size = (uint32_t) info.size;
   fsSetUnknownModified(&fileStat->modified);

   return NO_ERROR;
}

error_t fsRenameFile(const char_t *oldPath, const char_t *newPath)
{
   int err;

   if(!g_lfsReady || oldPath == NULL || newPath == NULL)
      return ERROR_INVALID_PARAMETER;

   err = lfs_rename(&g_lfs, oldPath, newPath);
   return fsMapLfsError(err);
}

error_t fsDeleteFile(const char_t *path)
{
   int err;

   if(!g_lfsReady || path == NULL)
      return ERROR_INVALID_PARAMETER;

   err = lfs_remove(&g_lfs, path);
   return fsMapLfsError(err);
}

FsFile *fsOpenFile(const char_t *path, uint_t mode)
{
   int flags;
   int err;
   lfs_file_t *f;

   if(!g_lfsReady || path == NULL)
      return NULL;

   flags = 0;

   if(mode & FS_FILE_MODE_READ)
      flags |= LFS_O_RDONLY;
   if(mode & FS_FILE_MODE_WRITE)
      flags |= LFS_O_WRONLY;
   if((mode & FS_FILE_MODE_READ) && (mode & FS_FILE_MODE_WRITE))
      flags = LFS_O_RDWR;
   if(mode & FS_FILE_MODE_CREATE)
      flags |= LFS_O_CREAT;
   if(mode & FS_FILE_MODE_TRUNC)
      flags |= LFS_O_TRUNC;

   f = osAllocMem(sizeof(lfs_file_t));
   if(f == NULL)
      return NULL;

   osMemset(f, 0, sizeof(lfs_file_t));
   err = lfs_file_open(&g_lfs, f, path, flags);
   if(err < 0)
   {
      osFreeMem(f);
      return NULL;
   }

   return (FsFile *) f;
}

error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin)
{
   int whence;
   lfs_soff_t res;
   lfs_file_t *f;

   if(!g_lfsReady || file == NULL)
      return ERROR_INVALID_PARAMETER;

   f = (lfs_file_t *) file;

   if(origin == FS_SEEK_CUR)
      whence = LFS_SEEK_CUR;
   else if(origin == FS_SEEK_END)
      whence = LFS_SEEK_END;
   else
      whence = LFS_SEEK_SET;

   res = lfs_file_seek(&g_lfs, f, offset, whence);
   if(res < 0)
      return fsMapLfsError((int) res);

   return NO_ERROR;
}

error_t fsWriteFile(FsFile *file, void *data, size_t length)
{
   lfs_ssize_t n;
   lfs_file_t *f;

   if(!g_lfsReady || file == NULL)
      return ERROR_INVALID_PARAMETER;

   f = (lfs_file_t *) file;
   n = lfs_file_write(&g_lfs, f, data, length);
   if(n < 0 || (size_t) n != length)
      return fsMapLfsError((int) n);

   return NO_ERROR;
}

error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length)
{
   lfs_ssize_t n;
   lfs_file_t *f;

   if(!g_lfsReady || file == NULL || length == NULL)
      return ERROR_INVALID_PARAMETER;

   *length = 0;
   f = (lfs_file_t *) file;
   n = lfs_file_read(&g_lfs, f, data, size);
   if(n < 0)
      return fsMapLfsError((int) n);

   *length = (size_t) n;
   if(n == 0)
      return ERROR_END_OF_FILE;

   return NO_ERROR;
}

void fsCloseFile(FsFile *file)
{
   lfs_file_t *f;

   if(!g_lfsReady || file == NULL)
      return;

   f = (lfs_file_t *) file;
   (void) lfs_file_close(&g_lfs, f);
   osFreeMem(f);
}

bool_t fsDirExists(const char_t *path)
{
   int err;
   struct lfs_info info;

   if(!g_lfsReady || path == NULL)
      return FALSE;

   err = lfs_stat(&g_lfs, path, &info);
   if(err < 0)
      return FALSE;

   return (info.type == LFS_TYPE_DIR);
}

error_t fsCreateDir(const char_t *path)
{
   int err;

   if(!g_lfsReady || path == NULL)
      return ERROR_INVALID_PARAMETER;

   err = lfs_mkdir(&g_lfs, path);
   return fsMapLfsError(err);
}

error_t fsRemoveDir(const char_t *path)
{
   // littleFS removes directories via lfs_remove if empty
   return fsDeleteFile(path);
}

FsDir *fsOpenDir(const char_t *path)
{
   int err;
   lfs_dir_t *d;

   if(!g_lfsReady || path == NULL)
      return NULL;

   d = osAllocMem(sizeof(lfs_dir_t));
   if(d == NULL)
      return NULL;

   osMemset(d, 0, sizeof(lfs_dir_t));
   err = lfs_dir_open(&g_lfs, d, path);
   if(err < 0)
   {
      osFreeMem(d);
      return NULL;
   }

   // We only need the handle; FsDir wrapper is unnecessary here, keep API
   return (FsDir *) d;
}

error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry)
{
   int err;
   struct lfs_info info;
   lfs_dir_t *d;

   if(!g_lfsReady || dir == NULL || dirEntry == NULL)
      return ERROR_INVALID_PARAMETER;

   d = (lfs_dir_t *) dir;
   osMemset(dirEntry, 0, sizeof(FsDirEntry));

   err = lfs_dir_read(&g_lfs, d, &info);
   if(err < 0)
      return fsMapLfsError(err);

   // lfs_dir_read returns 0 on end of dir
   if(err == 0)
      return ERROR_END_OF_STREAM;

   if(info.type == LFS_TYPE_DIR)
      dirEntry->attributes |= FS_FILE_ATTR_DIRECTORY;

   dirEntry->size = (uint32_t) info.size;
   fsSetUnknownModified(&dirEntry->modified);

   if(info.name != NULL)
      strSafeCopy(dirEntry->name, info.name, FS_MAX_NAME_LEN);

   return NO_ERROR;
}

void fsCloseDir(FsDir *dir)
{
   lfs_dir_t *d;

   if(!g_lfsReady || dir == NULL)
      return;

   d = (lfs_dir_t *) dir;
   (void) lfs_dir_close(&g_lfs, d);
   osFreeMem(d);
}

