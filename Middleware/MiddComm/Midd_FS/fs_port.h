/**
 * @file fs_port.h
 * @brief File system abstraction layer
 **/

#ifndef __FS_PORT_H__
#define __FS_PORT_H__

#include "os_port.h"
#include "date_time.h"
#include "error.h"

//Maximum filename length
#ifndef FS_MAX_NAME_LEN
   #define FS_MAX_NAME_LEN 64
#elif (FS_MAX_NAME_LEN < 11)
   #error FS_MAX_NAME_LEN parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief File attributes
 **/
typedef enum
{
   FS_FILE_ATTR_READ_ONLY   = 0x01,
   FS_FILE_ATTR_HIDDEN      = 0x02,
   FS_FILE_ATTR_SYSTEM      = 0x04,
   FS_FILE_ATTR_VOLUME_NAME = 0x08,
   FS_FILE_ATTR_DIRECTORY   = 0x10,
   FS_FILE_ATTR_ARCHIVE     = 0x20
} FsFileAttributes;

/**
 * @brief File access mode
 **/
typedef enum
{
   FS_FILE_MODE_READ   = 1,
   FS_FILE_MODE_WRITE  = 2,
   FS_FILE_MODE_CREATE = 4,
   FS_FILE_MODE_TRUNC  = 8
} FsFileMode;

/**
 * @brief File seek origin
 **/
typedef enum
{
   FS_SEEK_SET = 0,
   FS_SEEK_CUR = 1,
   FS_SEEK_END = 2
} FsSeekOrigin;

/**
 * @brief File status
 **/
typedef struct
{
   uint32_t attributes;
   uint32_t size;
   DateTime modified;
} FsFileStat;

/**
 * @brief Directory entry
 **/
typedef struct
{
   uint32_t attributes;
   uint32_t size;
   DateTime modified;
   char_t name[FS_MAX_NAME_LEN + 1];
} FsDirEntry;

#include "fs_port_config.h"

//C++ guard
#ifdef __cplusplus
}
#endif

#endif /* __FS_PORT_H__ */
