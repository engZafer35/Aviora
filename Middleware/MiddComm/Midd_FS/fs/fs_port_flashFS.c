/**
 * @file fs_port_flashFS.c
 * @brief File system abstraction layer (simple internal FlashFS)
 **/

#include <string.h>
#include "fs_port.h"
#include "fs_port_flashFS.h"
#include "str.h"
#include "date_time.h"

/**
 * This is a very simple append-only log-structured filesystem intended for use on MCU internal flash ranges. It is not designed for performance or long-term endurance, but rather to provide a simple way to store files in flash without needing an external SPI flash or more complex filesystem. The on-flash format is intentionally simple and hardware-agnostic, relying on user-provided callbacks for flash read/prog/erase operations. The filesystem maintains an in-memory index of files for fast lookup, which is rebuilt on mount by scanning the log. When the log is full, the user can call flashFsFormat() to erase and start fresh.
 * Design goals:
 * - Simple and easy to use
 * - Low power consumption
 * - Minimal memory footprint
 * Overall, this is a simple and lightweight filesystem for basic file storage needs on MCU internal flash, with the tradeoff of simplicity and ease of use over performance and endurance.
   * Note: This implementation does not include any wear leveling or garbage collection, so it is not suitable for high-write workloads or long-term use without periodic formatting. It is intended for simple use cases where files are written infrequently and the total number of writes is limited (e.g., configuration storage, occasional logs). The append-only log format means that deleted files still consume flash space until the log is formatted, so it is important to monitor free space and format when necessary.
   * The on-flash format consists of a superblock followed by an append-only log of file records. Each file record contains metadata (name, size, data offset) and the file data is stored in the flash region immediately following the record. When a file is deleted, a delete record is appended to the log, but the file data is not erased until the log is formatted. When a file is renamed, a rename record is appended to the log. The in-memory index is rebuilt on mount by scanning the log and applying the file/create/delete/rename records in order.
   * The API provides basic file operations (open/read/write/close, delete, rename) and relies on user-provided callbacks for flash read/prog/erase operations. The user must call flashFsConfigure() to provide the callbacks and geometry before calling fsInit() to mount the filesystem. The user can call flashFsFormat() to erase the flash region and create a fresh superblock when needed (e.g., when the log is full or on first use).
   * This implementation is not designed for concurrent access or multi-threaded use, so the user should ensure that file operations are serialized appropriately. The API does not include any locking or synchronization mechanisms, so it is the caller's responsibility to ensure thread safety if needed.
   * Overall, this is a simple and lightweight filesystem for basic file storage needs on MCU internal flash, with the tradeoff of simplicity and ease of use over performance and endurance. It is suitable for simple use cases where files are written infrequently and the total number of writes is limited, but it is not suitable for high-write workloads or long-term use without periodic formatting.
   * Example usage:
   * FlashFsOps ops = {
   *  .read = myFlashRead,
   *  .prog = myFlashProg,
   * .erase = myFlashErase,
   * .sync = myFlashSync
   * };
   * FlashFsGeom geom = {
   * .baseAddr = 0x080A0000,
   * .totalSize = 128 * 1024,
   * .eraseBlockSize = 4 * 1024,
   * .progMinSize = 256
   * };
   * flashFsConfigure(&ops, &geom);
   * flashFsFormat(); // format on first use
   * fsInit(); // mount filesystem
   * // Now can use fsFileExists, fsGetFileSize, fsOpenFile, fsReadFile, fsWriteFile, fsCloseFile, fsDeleteFile, fsRenameFile APIs
   * Note: The user should monitor free space and call flashFsFormat() when the log is full, as there is no garbage collection or wear leveling in this implementation. The append-only log format means that deleted files still consume flash space until the log is formatted, so it is important to manage free space appropriately.
 */
//----------------------------------------------------------------------------//
// On-flash layout (append-only log)
//----------------------------------------------------------------------------//

#define FLASHFS_MAGIC        0x46534653u /* 'FSFS' */
#define FLASHFS_VERSION      0x00010000u

#define FLASHFS_MAX_FILES    32u
#define FLASHFS_NAME_MAX     FS_MAX_NAME_LEN

typedef enum
{
   FLASHFS_REC_UNUSED = 0xFFFFFFFFu, // erased flash
   FLASHFS_REC_FILE   = 0xA11E0001u, // file record
   FLASHFS_REC_DELETE = 0xA11E0002u, // delete record
   FLASHFS_REC_RENAME = 0xA11E0003u  // rename record
} FlashFsRecType;

typedef struct
{
   uint32_t magic;
   uint32_t version;
   uint32_t headerSize;
   uint32_t regionSize;
   uint32_t eraseBlockSize;
   uint32_t progMinSize;
   uint32_t reserved[10];
   uint32_t crc32; // not used (kept for future)
} FlashFsSuper;

typedef struct
{
   uint32_t type;
   uint32_t length; // total record bytes including header
   uint32_t seq;
   uint32_t reserved;
} FlashFsRecHdr;

typedef struct
{
   FlashFsRecHdr hdr;
   uint32_t nameLen;
   uint32_t dataOffset; // from baseAddr
   uint32_t dataSize;
   // followed by name bytes (not null-terminated), then padding
} FlashFsFileRec;

typedef struct
{
   FlashFsRecHdr hdr;
   uint32_t nameLen;
   // followed by name
} FlashFsDeleteRec;

typedef struct
{
   FlashFsRecHdr hdr;
   uint32_t oldNameLen;
   uint32_t newNameLen;
   // followed by oldName then newName
} FlashFsRenameRec;

typedef struct
{
   bool_t inUse;
   char_t name[FLASHFS_NAME_MAX + 1];
   uint32_t dataOffset;
   uint32_t dataSize;
} FlashFsIndexEntry;

typedef struct
{
   FlashFsOps ops;
   FlashFsGeom geom;
   bool_t configured;
   bool_t mounted;

   FlashFsSuper super;
   uint32_t logWriteOff; // next write position (offset from baseAddr)
   uint32_t nextSeq;

   FlashFsIndexEntry index[FLASHFS_MAX_FILES];
} FlashFsContext;

static FlashFsContext g_fs;

//----------------------------------------------------------------------------//
// Helpers
//----------------------------------------------------------------------------//

static uint32_t alignUp(uint32_t x, uint32_t a)
{
   if(a == 0)
      return x;
   return (x + (a - 1)) & ~(a - 1);
}

static error_t flashRead(uint32_t off, void *data, size_t len)
{
   if(!g_fs.configured || g_fs.ops.read == NULL)
      return ERROR_NOT_READY;
   if(off + (uint32_t)len > g_fs.geom.totalSize)
      return ERROR_INVALID_PARAMETER;
   return g_fs.ops.read(g_fs.geom.baseAddr + off, data, len);
}

static error_t flashProg(uint32_t off, const void *data, size_t len)
{
   if(!g_fs.configured || g_fs.ops.prog == NULL)
      return ERROR_NOT_READY;
   if(off + (uint32_t)len > g_fs.geom.totalSize)
      return ERROR_INVALID_PARAMETER;
   return g_fs.ops.prog(g_fs.geom.baseAddr + off, data, len);
}

static error_t flashEraseBlock(uint32_t off)
{
   if(!g_fs.configured || g_fs.ops.erase == NULL)
      return ERROR_NOT_READY;
   if(off >= g_fs.geom.totalSize)
      return ERROR_INVALID_PARAMETER;
   return g_fs.ops.erase(g_fs.geom.baseAddr + off);
}

static error_t flashSync(void)
{
   if(!g_fs.configured || g_fs.ops.sync == NULL)
      return NO_ERROR;
   return g_fs.ops.sync();
}

static int fsFindIndexByName(const char_t *name)
{
   uint_t i;
   for(i = 0; i < FLASHFS_MAX_FILES; i++)
   {
      if(g_fs.index[i].inUse && osStrcmp(g_fs.index[i].name, name) == 0)
         return (int)i;
   }
   return -1;
}

static int fsFindFreeIndexSlot(void)
{
   uint_t i;
   for(i = 0; i < FLASHFS_MAX_FILES; i++)
   {
      if(!g_fs.index[i].inUse)
         return (int)i;
   }
   return -1;
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

static error_t fsWriteRecord(const void *rec, uint32_t recLen)
{
   error_t error;
   uint32_t alignedLen;

   if(!g_fs.mounted)
      return ERROR_NOT_READY;

   alignedLen = alignUp(recLen, g_fs.geom.progMinSize);
   if(g_fs.logWriteOff + alignedLen > g_fs.geom.totalSize)
      return ERROR_OUT_OF_RESOURCES;

   error = flashProg(g_fs.logWriteOff, rec, recLen);
   if(error)
      return error;

   // pad (optional, but keep deterministic)
   if(alignedLen > recLen)
   {
      uint8_t pad[32];
      uint32_t remain = alignedLen - recLen;
      osMemset(pad, 0xFF, sizeof(pad));
      while(remain > 0)
      {
         uint32_t n = (remain > sizeof(pad)) ? sizeof(pad) : remain;
         error = flashProg(g_fs.logWriteOff + (alignedLen - remain), pad, n);
         if(error)
            return error;
         remain -= n;
      }
   }

   g_fs.logWriteOff += alignedLen;
   return flashSync();
}

static error_t fsLoadOrCreateSuper(void)
{
   error_t error;
   FlashFsSuper s;

   error = flashRead(0, &s, sizeof(s));
   if(error)
      return error;

   if(s.magic == FLASHFS_MAGIC && s.version == FLASHFS_VERSION &&
      s.headerSize >= sizeof(FlashFsSuper) &&
      s.regionSize == g_fs.geom.totalSize &&
      s.eraseBlockSize == g_fs.geom.eraseBlockSize &&
      s.progMinSize == g_fs.geom.progMinSize)
   {
      g_fs.super = s;
      return NO_ERROR;
   }

   // No valid superblock found; create one (requires format to have erased area)
   osMemset(&g_fs.super, 0xFF, sizeof(g_fs.super));
   g_fs.super.magic = FLASHFS_MAGIC;
   g_fs.super.version = FLASHFS_VERSION;
   g_fs.super.headerSize = sizeof(FlashFsSuper);
   g_fs.super.regionSize = g_fs.geom.totalSize;
   g_fs.super.eraseBlockSize = g_fs.geom.eraseBlockSize;
   g_fs.super.progMinSize = g_fs.geom.progMinSize;
   g_fs.super.crc32 = 0;

   return flashProg(0, &g_fs.super, sizeof(g_fs.super));
}

static error_t fsRebuildIndexAndLogPtr(void)
{
   error_t error;
   uint32_t off;
   uint32_t seqMax;

   osMemset(g_fs.index, 0, sizeof(g_fs.index));
   off = alignUp(sizeof(FlashFsSuper), g_fs.geom.progMinSize);
   seqMax = 0;

   while(off + sizeof(FlashFsRecHdr) <= g_fs.geom.totalSize)
   {
      FlashFsRecHdr hdr;

      error = flashRead(off, &hdr, sizeof(hdr));
      if(error)
         return error;

      // erased means end of log
      if(hdr.type == FLASHFS_REC_UNUSED || hdr.length == 0xFFFFFFFFu)
         break;

      // basic sanity
      if(hdr.length < sizeof(FlashFsRecHdr) || hdr.length > (g_fs.geom.totalSize - off))
         break;

      if(hdr.seq != 0xFFFFFFFFu && hdr.seq > seqMax)
         seqMax = hdr.seq;

      if(hdr.type == FLASHFS_REC_FILE)
      {
         FlashFsFileRec rec;
         char_t name[FLASHFS_NAME_MAX + 1];
         uint32_t nameLen;

         if(hdr.length < sizeof(FlashFsFileRec))
            break;

         error = flashRead(off, &rec, sizeof(rec));
         if(error)
            return error;

         nameLen = rec.nameLen;
         if(nameLen == 0 || nameLen > FLASHFS_NAME_MAX)
         {
            // skip invalid
         }
         else
         {
            error = flashRead(off + sizeof(FlashFsFileRec), name, nameLen);
            if(error)
               return error;
            name[nameLen] = '\0';

            // insert/update index
            {
               int idx = fsFindIndexByName(name);
               if(idx < 0)
                  idx = fsFindFreeIndexSlot();
               if(idx >= 0)
               {
                  g_fs.index[idx].inUse = TRUE;
                  strSafeCopy(g_fs.index[idx].name, name, FLASHFS_NAME_MAX);
                  g_fs.index[idx].dataOffset = rec.dataOffset;
                  g_fs.index[idx].dataSize = rec.dataSize;
               }
            }
         }
      }
      else if(hdr.type == FLASHFS_REC_DELETE)
      {
         FlashFsDeleteRec rec;
         char_t name[FLASHFS_NAME_MAX + 1];
         uint32_t nameLen;

         if(hdr.length < sizeof(FlashFsDeleteRec))
            break;

         error = flashRead(off, &rec, sizeof(rec));
         if(error)
            return error;

         nameLen = rec.nameLen;
         if(nameLen > 0 && nameLen <= FLASHFS_NAME_MAX)
         {
            error = flashRead(off + sizeof(FlashFsDeleteRec), name, nameLen);
            if(error)
               return error;
            name[nameLen] = '\0';

            {
               int idx = fsFindIndexByName(name);
               if(idx >= 0)
                  osMemset(&g_fs.index[idx], 0, sizeof(g_fs.index[idx]));
            }
         }
      }
      else if(hdr.type == FLASHFS_REC_RENAME)
      {
         FlashFsRenameRec rec;
         char_t oldName[FLASHFS_NAME_MAX + 1];
         char_t newName[FLASHFS_NAME_MAX + 1];

         if(hdr.length < sizeof(FlashFsRenameRec))
            break;

         error = flashRead(off, &rec, sizeof(rec));
         if(error)
            return error;

         if(rec.oldNameLen > 0 && rec.oldNameLen <= FLASHFS_NAME_MAX &&
            rec.newNameLen > 0 && rec.newNameLen <= FLASHFS_NAME_MAX &&
            (sizeof(FlashFsRenameRec) + rec.oldNameLen + rec.newNameLen) <= hdr.length)
         {
            error = flashRead(off + sizeof(FlashFsRenameRec), oldName, rec.oldNameLen);
            if(error)
               return error;
            oldName[rec.oldNameLen] = '\0';

            error = flashRead(off + sizeof(FlashFsRenameRec) + rec.oldNameLen, newName, rec.newNameLen);
            if(error)
               return error;
            newName[rec.newNameLen] = '\0';

            {
               int idx = fsFindIndexByName(oldName);
               if(idx >= 0)
               {
                  strSafeCopy(g_fs.index[idx].name, newName, FLASHFS_NAME_MAX);
               }
            }
         }
      }

      off += alignUp(hdr.length, g_fs.geom.progMinSize);
   }

   g_fs.logWriteOff = off;
   g_fs.nextSeq = seqMax + 1;
   return NO_ERROR;
}

//----------------------------------------------------------------------------//
// Public configuration
//----------------------------------------------------------------------------//

error_t flashFsConfigure(const FlashFsOps *ops, const FlashFsGeom *geom)
{
   if(ops == NULL || geom == NULL)
      return ERROR_INVALID_PARAMETER;
   if(ops->read == NULL || ops->prog == NULL || ops->erase == NULL)
      return ERROR_INVALID_PARAMETER;
   if(geom->totalSize == 0 || geom->eraseBlockSize == 0 || geom->progMinSize == 0)
      return ERROR_INVALID_PARAMETER;
   if((geom->eraseBlockSize & (geom->eraseBlockSize - 1)) != 0)
      return ERROR_INVALID_PARAMETER;
   if((geom->progMinSize & (geom->progMinSize - 1)) != 0)
      return ERROR_INVALID_PARAMETER;
   if(geom->totalSize < (sizeof(FlashFsSuper) + geom->eraseBlockSize))
      return ERROR_INVALID_PARAMETER;

   osMemset(&g_fs, 0, sizeof(g_fs));
   g_fs.ops = *ops;
   g_fs.geom = *geom;
   g_fs.configured = TRUE;
   g_fs.mounted = FALSE;
   return NO_ERROR;
}

error_t flashFsFormat(void)
{
   error_t error;
   uint32_t off;

   if(!g_fs.configured)
      return ERROR_NOT_READY;

   // Erase entire region block-by-block
   for(off = 0; off < g_fs.geom.totalSize; off += g_fs.geom.eraseBlockSize)
   {
      error = flashEraseBlock(off);
      if(error)
         return error;
   }

   // Write a fresh superblock
   error = fsLoadOrCreateSuper();
   if(error)
      return error;

   return flashSync();
}

//----------------------------------------------------------------------------//
// fs_port API
//----------------------------------------------------------------------------//

error_t fsInit(void)
{
   error_t error;

   if(!g_fs.configured)
      return ERROR_NOT_READY;

   // Try to load superblock; if absent, user must call flashFsFormat() first
   error = fsLoadOrCreateSuper();
   if(error)
      return error;

   error = fsRebuildIndexAndLogPtr();
   if(error)
      return error;

   g_fs.mounted = TRUE;
   return NO_ERROR;
}

bool_t fsFileExists(const char_t *path)
{
   if(path == NULL || !g_fs.mounted)
      return FALSE;
   return (fsFindIndexByName(path) >= 0) ? TRUE : FALSE;
}

error_t fsGetFileSize(const char_t *path, uint32_t *size)
{
   int idx;
   if(path == NULL || size == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;
   idx = fsFindIndexByName(path);
   if(idx < 0)
      return ERROR_FILE_NOT_FOUND;
   *size = g_fs.index[idx].dataSize;
   return NO_ERROR;
}

error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat)
{
   int idx;
   if(path == NULL || fileStat == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   osMemset(fileStat, 0, sizeof(FsFileStat));

   // Root dir
   if(osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
   {
      fileStat->attributes = FS_FILE_ATTR_DIRECTORY;
      fileStat->size = 0;
      fsSetUnknownModified(&fileStat->modified);
      return NO_ERROR;
   }

   idx = fsFindIndexByName(path);
   if(idx < 0)
      return ERROR_FILE_NOT_FOUND;

   fileStat->attributes = 0;
   fileStat->size = g_fs.index[idx].dataSize;
   fsSetUnknownModified(&fileStat->modified);
   return NO_ERROR;
}

error_t fsRenameFile(const char_t *oldPath, const char_t *newPath)
{
   FlashFsRenameRec rec;
   uint32_t oldLen;
   uint32_t newLen;
   uint32_t total;
   uint8_t buf[sizeof(FlashFsRenameRec) + (2 * (FLASHFS_NAME_MAX + 1))];

   if(oldPath == NULL || newPath == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   if(fsFindIndexByName(oldPath) < 0)
      return ERROR_FILE_NOT_FOUND;
   if(fsFindIndexByName(newPath) >= 0)
      return ERROR_ALREADY_EXISTS;

   oldLen = (uint32_t) osStrlen(oldPath);
   newLen = (uint32_t) osStrlen(newPath);
   if(oldLen == 0 || newLen == 0 || oldLen > FLASHFS_NAME_MAX || newLen > FLASHFS_NAME_MAX)
      return ERROR_INVALID_NAME;

   osMemset(&rec, 0xFF, sizeof(rec));
   rec.hdr.type = FLASHFS_REC_RENAME;
   rec.hdr.seq = g_fs.nextSeq++;
   rec.oldNameLen = oldLen;
   rec.newNameLen = newLen;

   total = sizeof(FlashFsRenameRec) + oldLen + newLen;
   rec.hdr.length = total;

   osMemcpy(buf, &rec, sizeof(rec));
   osMemcpy(buf + sizeof(FlashFsRenameRec), oldPath, oldLen);
   osMemcpy(buf + sizeof(FlashFsRenameRec) + oldLen, newPath, newLen);

   // apply to index first only after record written successfully
   {
      error_t error = fsWriteRecord(buf, total);
      if(error)
         return error;
   }

   {
      int idx = fsFindIndexByName(oldPath);
      if(idx >= 0)
         strSafeCopy(g_fs.index[idx].name, newPath, FLASHFS_NAME_MAX);
   }

   return NO_ERROR;
}

error_t fsDeleteFile(const char_t *path)
{
   FlashFsDeleteRec rec;
   uint32_t nameLen;
   uint32_t total;
   uint8_t buf[sizeof(FlashFsDeleteRec) + (FLASHFS_NAME_MAX + 1)];
   int idx;

   if(path == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   idx = fsFindIndexByName(path);
   if(idx < 0)
      return ERROR_FILE_NOT_FOUND;

   nameLen = (uint32_t) osStrlen(path);
   if(nameLen == 0 || nameLen > FLASHFS_NAME_MAX)
      return ERROR_INVALID_NAME;

   osMemset(&rec, 0xFF, sizeof(rec));
   rec.hdr.type = FLASHFS_REC_DELETE;
   rec.hdr.seq = g_fs.nextSeq++;
   rec.nameLen = nameLen;
   total = sizeof(FlashFsDeleteRec) + nameLen;
   rec.hdr.length = total;

   osMemcpy(buf, &rec, sizeof(rec));
   osMemcpy(buf + sizeof(FlashFsDeleteRec), path, nameLen);

   {
      error_t error = fsWriteRecord(buf, total);
      if(error)
         return error;
   }

   osMemset(&g_fs.index[idx], 0, sizeof(g_fs.index[idx]));
   return NO_ERROR;
}

typedef struct
{
   bool_t writeMode;
   bool_t readMode;

   char_t name[FLASHFS_NAME_MAX + 1];
   uint32_t size;

   // read cursor (for existing files)
   uint32_t readOff;

   // write staging
   uint32_t writeDataOff; // where file data starts (offset from base)
   uint32_t writePos;

   // opened existing file?
   bool_t existing;
   uint32_t existingDataOff;
   uint32_t existingSize;
} FlashFsFileHandle;

FsFile *fsOpenFile(const char_t *path, uint_t mode)
{
   FlashFsFileHandle *h;
   uint32_t nameLen;
   int idx;
   bool_t truncMode;

   if(path == NULL || !g_fs.mounted)
      return NULL;

   nameLen = (uint32_t) osStrlen(path);
   if(nameLen == 0 || nameLen > FLASHFS_NAME_MAX)
      return NULL;

   idx = fsFindIndexByName(path);

   if((mode & FS_FILE_MODE_CREATE) == 0)
   {
      // must exist when not creating
      if(idx < 0 && (mode & FS_FILE_MODE_WRITE) == 0)
         return NULL;
   }

   h = osAllocMem(sizeof(FlashFsFileHandle));
   if(h == NULL)
      return NULL;
   osMemset(h, 0, sizeof(*h));

   strSafeCopy(h->name, path, FLASHFS_NAME_MAX);
   h->readMode = (mode & FS_FILE_MODE_READ) ? TRUE : FALSE;
   h->writeMode = (mode & FS_FILE_MODE_WRITE) ? TRUE : FALSE;

   if(idx >= 0)
   {
      h->existing = TRUE;
      h->existingDataOff = g_fs.index[idx].dataOffset;
      h->existingSize = g_fs.index[idx].dataSize;
   }

   truncMode = (mode & FS_FILE_MODE_TRUNC) ? TRUE : FALSE;

   // For write mode, we always create a new version (append-only log).
   // If TRUNC yoksa ve dosya zaten varsa, önce eski içeriği yeni bölgeye kopyalayıp
   // ardından append yapılmasına izin veriyoruz (copy-on-append).
   if(h->writeMode)
   {
      error_t error;
      uint32_t copied = 0;

      // Reserve data area at end of log: data first, then metadata record written on close.
      // We'll place data starting at current logWriteOff, and metadata record after data.
      h->writeDataOff = g_fs.logWriteOff;
      h->writePos = 0;

      // Copy existing content when appending (no TRUNC and file exists)
      if(h->existing && !truncMode && h->existingSize > 0)
      {
         uint8_t buf[512];
         uint32_t remaining = h->existingSize;

         // Ensure we have space for old data
         if(h->writeDataOff + remaining > g_fs.geom.totalSize)
         {
            osFreeMem(h);
            return NULL;
         }

         while(remaining > 0)
         {
            uint32_t chunk = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;

            // enforce prog granularity on chunks
            if(chunk % g_fs.geom.progMinSize != 0)
            {
               chunk -= (chunk % g_fs.geom.progMinSize);
               if(chunk == 0)
                  break;
            }

            error = flashRead(h->existingDataOff + copied, buf, chunk);
            if(error)
            {
               osFreeMem(h);
               return NULL;
            }

            error = flashProg(h->writeDataOff + h->writePos, buf, chunk);
            if(error)
            {
               osFreeMem(h);
               return NULL;
            }

            copied += chunk;
            h->writePos += chunk;
            remaining -= chunk;
         }

         h->size = h->writePos;
      }
   }
   else
   {
      h->readOff = 0;
   }

   return (FsFile *) h;
}

error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin)
{
   FlashFsFileHandle *h;
   int32_t base;
   int32_t newPos;

   if(file == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   h = (FlashFsFileHandle *) file;

   if(!h->readMode)
      return ERROR_NOT_IMPLEMENTED;

   if(origin == FS_SEEK_CUR)
      base = (int32_t) h->readOff;
   else if(origin == FS_SEEK_END)
      base = (int32_t) h->existingSize;
   else
      base = 0;

   newPos = base + offset;
   if(newPos < 0)
      return ERROR_INVALID_PARAMETER;
   if((uint32_t)newPos > h->existingSize)
      return ERROR_INVALID_PARAMETER;

   h->readOff = (uint32_t) newPos;
   return NO_ERROR;
}

error_t fsWriteFile(FsFile *file, void *data, size_t length)
{
   FlashFsFileHandle *h;
   error_t error;

   if(file == NULL || data == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   h = (FlashFsFileHandle *) file;
   if(!h->writeMode)
      return ERROR_ACCESS_DENIED;

   // Ensure we don't overlap with metadata record area by writing directly into log region.
   // Data is written starting at writeDataOff; metadata will be appended on close.
   if(h->writeDataOff + h->writePos + (uint32_t)length > g_fs.geom.totalSize)
      return ERROR_OUT_OF_RESOURCES;

   // enforce prog granularity alignment on writes (simple policy)
   if((h->writePos % g_fs.geom.progMinSize) != 0 || (length % g_fs.geom.progMinSize) != 0)
      return ERROR_INVALID_PARAMETER;

   error = flashProg(h->writeDataOff + h->writePos, data, length);
   if(error)
      return error;

   h->writePos += (uint32_t)length;
   if(h->writePos > h->size)
      h->size = h->writePos;

   return flashSync();
}

error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length)
{
   FlashFsFileHandle *h;
   error_t error;
   uint32_t remain;
   uint32_t n;

   if(file == NULL || data == NULL || length == NULL || !g_fs.mounted)
      return ERROR_INVALID_PARAMETER;

   h = (FlashFsFileHandle *) file;
   if(!h->readMode)
      return ERROR_ACCESS_DENIED;

   *length = 0;

   if(h->existing == FALSE)
      return ERROR_FILE_NOT_FOUND;

   if(h->readOff >= h->existingSize)
      return ERROR_END_OF_FILE;

   remain = h->existingSize - h->readOff;
   n = (remain < (uint32_t)size) ? remain : (uint32_t)size;

   error = flashRead(h->existingDataOff + h->readOff, data, n);
   if(error)
      return error;

   h->readOff += n;
   *length = n;
   return NO_ERROR;
}

void fsCloseFile(FsFile *file)
{
   FlashFsFileHandle *h;

   if(file == NULL || !g_fs.mounted)
      return;

   h = (FlashFsFileHandle *) file;

   if(h->writeMode)
   {
      // Commit file record (metadata) after written data
      FlashFsFileRec rec;
      uint32_t nameLen = (uint32_t) osStrlen(h->name);
      uint32_t total = sizeof(FlashFsFileRec) + nameLen;
      uint8_t buf[sizeof(FlashFsFileRec) + (FLASHFS_NAME_MAX + 1)];

      // Move logWriteOff past data area first (data written directly at logWriteOff)
      // Ensure logWriteOff tracks the end of written data.
      {
         uint32_t dataEnd = h->writeDataOff + alignUp(h->size, g_fs.geom.progMinSize);
         if(dataEnd > g_fs.logWriteOff)
            g_fs.logWriteOff = dataEnd;
      }

      osMemset(&rec, 0xFF, sizeof(rec));
      rec.hdr.type = FLASHFS_REC_FILE;
      rec.hdr.seq = g_fs.nextSeq++;
      rec.nameLen = nameLen;
      rec.dataOffset = h->writeDataOff;
      rec.dataSize = h->size;
      rec.hdr.length = total;

      osMemcpy(buf, &rec, sizeof(rec));
      osMemcpy(buf + sizeof(FlashFsFileRec), h->name, nameLen);

      if(fsWriteRecord(buf, total) == NO_ERROR)
      {
         // Update index
         int idx = fsFindIndexByName(h->name);
         if(idx < 0)
            idx = fsFindFreeIndexSlot();
         if(idx >= 0)
         {
            g_fs.index[idx].inUse = TRUE;
            strSafeCopy(g_fs.index[idx].name, h->name, FLASHFS_NAME_MAX);
            g_fs.index[idx].dataOffset = h->writeDataOff;
            g_fs.index[idx].dataSize = h->size;
         }
      }
   }

   osFreeMem(h);
}

bool_t fsDirExists(const char_t *path)
{
   uint_t i;
   const char_t *p;
   char_t prefix[FLASHFS_NAME_MAX + 1];
   size_t len;

   if(!g_fs.mounted || path == NULL)
      return FALSE;

   // Root directory
   if(osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
      return TRUE;

   // Normalize path to "prefix/" form (without leading '/')
   p = path;
   if(*p == '/')
      p++;
   strSafeCopy(prefix, p, FLASHFS_NAME_MAX);
   len = osStrlen(prefix);
   if(len == 0 || len >= FLASHFS_NAME_MAX)
      return FALSE;
   if(prefix[len - 1] != '/' && len < FLASHFS_NAME_MAX)
   {
      prefix[len] = '/';
      prefix[len + 1] = '\0';
   }

   // Directory exists if at least one file name starts with this prefix
   for(i = 0; i < FLASHFS_MAX_FILES; i++)
   {
      if(g_fs.index[i].inUse)
      {
         if(osStrncmp(g_fs.index[i].name, prefix, osStrlen(prefix)) == 0)
            return TRUE;
      }
   }

   return FALSE;
}

error_t fsCreateDir(const char_t *path)
{
   // No subdirectories
   if(path == NULL)
      return ERROR_INVALID_PARAMETER;
   if(osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
      return NO_ERROR;
   return ERROR_NOT_IMPLEMENTED;
}

error_t fsRemoveDir(const char_t *path)
{
   if(path == NULL)
      return ERROR_INVALID_PARAMETER;
   if(osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
      return ERROR_ACCESS_DENIED;
   return ERROR_NOT_IMPLEMENTED;
}

typedef struct
{
   uint_t cursor;
   bool_t root;
   char_t prefix[FLASHFS_NAME_MAX + 1];
} FlashFsDirHandle;

FsDir *fsOpenDir(const char_t *path)
{
   FlashFsDirHandle *d;
   const char_t *p;
   size_t len;

   if(!g_fs.mounted || path == NULL)
      return NULL;

   d = osAllocMem(sizeof(FlashFsDirHandle));
   if(d == NULL)
      return NULL;

   osMemset(d, 0, sizeof(FlashFsDirHandle));
   d->cursor = 0;

   // Normalize path: treat "" and "/" as root
   if(osStrcmp(path, "") == 0 || osStrcmp(path, "/") == 0)
   {
      d->root = TRUE;
      d->prefix[0] = '\0';
   }
   else
   {
      d->root = FALSE;

      // Skip leading '/'
      p = path;
      if(*p == '/')
         p++;

      // Copy as prefix and ensure it ends with '/'
      strSafeCopy(d->prefix, p, FLASHFS_NAME_MAX);
      len = osStrlen(d->prefix);
      if(len == 0 || len >= FLASHFS_NAME_MAX)
      {
         osFreeMem(d);
         return NULL;
      }

      if(d->prefix[len - 1] != '/' && len < FLASHFS_NAME_MAX)
      {
         d->prefix[len] = '/';
         d->prefix[len + 1] = '\0';
      }
   }

   return (FsDir *) d;
}

error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry)
{
   FlashFsDirHandle *d;
   uint_t i;

   if(!g_fs.mounted || dir == NULL || dirEntry == NULL)
      return ERROR_INVALID_PARAMETER;

   d = (FlashFsDirHandle *) dir;
   osMemset(dirEntry, 0, sizeof(FsDirEntry));

   for(i = d->cursor; i < FLASHFS_MAX_FILES; i++)
   {
      if(g_fs.index[i].inUse)
      {
         const char_t *name = g_fs.index[i].name;

         // Root: list everything
         if(d->root)
         {
            dirEntry->attributes = 0;
            dirEntry->size = g_fs.index[i].dataSize;
            fsSetUnknownModified(&dirEntry->modified);
            strSafeCopy(dirEntry->name, name, FS_MAX_NAME_LEN);
            d->cursor = i + 1;
            return NO_ERROR;
         }
         else
         {
            // Non-root: only list entries whose names start with prefix
            size_t plen = osStrlen(d->prefix);
            if(osStrncmp(name, d->prefix, plen) == 0)
            {
               dirEntry->attributes = 0;
               dirEntry->size = g_fs.index[i].dataSize;
               fsSetUnknownModified(&dirEntry->modified);
               // Return full path (including prefix) to keep it unique
               strSafeCopy(dirEntry->name, name, FS_MAX_NAME_LEN);
               d->cursor = i + 1;
               return NO_ERROR;
            }
         }
      }
   }

   return ERROR_END_OF_STREAM;
}

void fsCloseDir(FsDir *dir)
{
   if(dir != NULL)
      osFreeMem(dir);
}

