/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : 20 Mar 2026 - 14:21:51
* #File Name    : AppSWUpdate.c
*******************************************************************************/
/******************************************************************************
* This file implements the flashLink storage in MCU internal flash. 
* The flashLink storage is a simple linked-cell storage in MCU internal flash.
* The flashLink storage is used to store the files in the MCU internal flash.
* The flashLink storage is used to store the files in the MCU internal flash.
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
/********************************* INCLUDES ***********************************/
#include <string.h>
#include "fs_port.h"
#include "fs_port_flashLink.h"
#include "date_time.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

typedef struct
{
   FlashLinkOps ops;
   uint32_t baseAddr;
   uint32_t regionSize;
   uint16_t cellCount;
   uint32_t eraseBlockSize;
   bool_t   ready;
} FlashLinkContext;

typedef struct
{
   bool_t writeMode;
   char_t name[FLASHLINK_NAME_SIZE + 1];
   uint32_t readOff;
   uint32_t totalSize;
   int     headIdx;   /* Head cell index; -1 = yeni dosya */
   uint16_t lastIdx;  /* Son hücre; 0 = henüz yüklenmedi (sadece write modunda) */
} FlashLinkFileHandle;

typedef struct
{
   bool_t inUse;
   FlashLinkFileHandle h;
} FlashLinkFileSlot;

typedef struct
{
   uint16_t cursor;
} FlashLinkDirHandle;

typedef struct
{
   bool_t inUse;
   FlashLinkDirHandle h;
} FlashLinkDirSlot;

/********************************** VARIABLES *********************************/
static FlashLinkFileSlot g_filePool[FLASHLINK_MAX_OPEN_FILES];
static FlashLinkDirSlot  g_dirPool[FLASHLINK_MAX_OPEN_DIRS];

static FlashLinkContext g_flashLink;

/***************************** STATIC FUNCTIONS  ******************************/
static uint32_t flCellAddr(uint16_t index)
{
   // index is 1-based; 0 is "no cell"
   return g_flashLink.baseAddr + (uint32_t)(index - 1) * FLASHLINK_CELL_SIZE;
}

static error_t flRead(uint32_t addr, void *data, size_t len)
{
   return g_flashLink.ops.read(addr, data, len);
}

static error_t flProg(uint32_t addr, const void *data, size_t len)
{
   return g_flashLink.ops.prog(addr, data, len);
}

static error_t flSync(void)
{
   return g_flashLink.ops.sync();
}

static uint16_t flDecodeU16Inv(uint16_t raw)
{
   return (uint16_t)(~raw);
}

static uint16_t flEncodeU16Inv(uint16_t v)
{
   return (uint16_t)(~v);
}

static error_t flReadHeader(uint16_t index, char_t *name,
                            uint16_t *nextIndex, uint16_t *dataLen)
{
   uint8_t hdr[FLASHLINK_NAME_SIZE + 4];
   uint32_t addr;
   error_t error;
   uint16_t rawNext;
   uint16_t rawLen;

   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;

   addr = flCellAddr(index);
   error = flRead(addr, hdr, sizeof(hdr));
   if(error)
      return error;

   if(name)
   {
      osMemcpy(name, hdr, FLASHLINK_NAME_SIZE);
   }

   if(nextIndex)
   {
      rawNext = (uint16_t)((uint16_t)hdr[FLASHLINK_NAME_SIZE] |
                ((uint16_t)hdr[FLASHLINK_NAME_SIZE + 1] << 8));
      *nextIndex = flDecodeU16Inv(rawNext);
   }

   if(dataLen)
   {
      rawLen = (uint16_t)((uint16_t)hdr[FLASHLINK_NAME_SIZE + 2] |
               ((uint16_t)hdr[FLASHLINK_NAME_SIZE + 3] << 8));
      *dataLen = flDecodeU16Inv(rawLen);
   }

   return NO_ERROR;
}

static error_t flProgName(uint16_t index, const char_t *name)
{
   uint8_t nameBuf[FLASHLINK_NAME_SIZE];
   uint32_t addr;

   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;

   // Program name bytes; unused bytes stay 0xFF (requires erased flash)
   osMemset(nameBuf, 0xFF, sizeof(nameBuf));

   if(name)
   {
      // copy and ensure zero-terminated
      osStrncpy((char_t *)nameBuf, name, FLASHLINK_NAME_SIZE - 1);
      nameBuf[sizeof(nameBuf) - 1] = '\0';
   }

   addr = flCellAddr(index);
   return flProg(addr, nameBuf, sizeof(nameBuf));
}

static error_t flProgNext(uint16_t index, uint16_t nextIndex)
{
   uint8_t raw[2];
   uint16_t enc = flEncodeU16Inv(nextIndex);
   uint32_t addr;

   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;

   raw[0] = (uint8_t)(enc & 0xFF);
   raw[1] = (uint8_t)((enc >> 8) & 0xFF);

   addr = flCellAddr(index) + FLASHLINK_NAME_SIZE;
   return flProg(addr, raw, sizeof(raw));
}

static error_t flProgLen(uint16_t index, uint16_t dataLen)
{
   uint8_t raw[2];
   uint16_t enc = flEncodeU16Inv(dataLen);
   uint32_t addr;

   if((index == 0) || (index > g_flashLink.cellCount) || (dataLen > FLASHLINK_CELL_DATA_SIZE))
   {
      return ERROR_INVALID_PARAMETER;
   }
   raw[0] = (uint8_t)(enc & 0xFF);
   raw[1] = (uint8_t)((enc >> 8) & 0xFF);

   addr = flCellAddr(index) + FLASHLINK_NAME_SIZE + 2;
   return flProg(addr, raw, sizeof(raw));
}

static error_t flWriteData(uint16_t index, uint16_t offset, const void *data, uint16_t len)
{
   uint32_t addr;

   if((0 == index) || (index > g_flashLink.cellCount) || (offset + len > FLASHLINK_CELL_DATA_SIZE))
   {
      return ERROR_INVALID_PARAMETER;
   }

   addr = flCellAddr(index) + FLASHLINK_NAME_SIZE + 4 + offset;
   return flProg(addr, data, len);
}

static bool_t flCellIsEmpty(uint16_t index)
{
   uint8_t hdr[FLASHLINK_NAME_SIZE + 4];
   uint32_t addr;
   error_t error;
   size_t i;

   if((0 == index) || (index > g_flashLink.cellCount))
   {
      return FALSE;
   }

   addr = flCellAddr(index);
   error = flRead(addr, hdr, sizeof(hdr));
   if(error)
   {
      return FALSE;
   }
   // Empty = fully erased header (0xFF)
   bool_t allFF = TRUE;
   for(i = 0; i < sizeof(hdr); i++)
   {
      if(hdr[i] != 0xFF)
      {
         allFF = FALSE;
         break;
      }
   }

   if(allFF)
   {
      return TRUE;
   }

   //full cell
   return FALSE;
}

static int flFindFreeCell(void)
{
   uint16_t i;
   for(i = 1; i <= g_flashLink.cellCount; i++)
   {
      if(flCellIsEmpty(i))
         return (int)i;
   }
   return -1;
}

static int flFindHeadByName(const char_t *name)
{
   uint16_t i;
   char_t cellName[FLASHLINK_NAME_SIZE];

   for(i = 1; i <= g_flashLink.cellCount; i++)
   {
      uint16_t nextIdx, dataLen;
      if(flCellIsEmpty(i))
         continue;

      if(flReadHeader(i, cellName, &nextIdx, &dataLen))
         continue;

      // name is zero-terminated in header; compare
      if((cellName[0] != '\0') && (cellName[0] != (char_t)0xFF) && (0 == sStrcmp(cellName, name)))
      {
         return (int)i;
      }
   }

   return -1;
}

static uint16_t flFindLastInChain(uint16_t headIndex)
{
   uint16_t cur = headIndex;
   uint16_t next;
   char_t dummyName[FLASHLINK_NAME_SIZE];
   uint16_t dataLen;

   while(cur != 0)
   {
      if(flReadHeader(cur, dummyName, &next, &dataLen))
      {
         break;
      }
      if(0 == next)
      {
         return cur;
      }
      cur = next;
   }

   return headIndex;
}

static error_t flClearCellHeader(uint16_t index)
{
   uint8_t b = 0x00;
   uint32_t addr;

   if((0 == index) || (index > g_flashLink.cellCount))
   {
      return ERROR_INVALID_PARAMETER;
   }

   // Tombstone delete for flash (only 1->0 allowed): program first name byte to 0x00. 
   addr = flCellAddr(index);
   return flProg(addr, &b, 1); //it is enough to clear first byte of string
}

static error_t flWriteChain(const char_t *name, const void *data, size_t len,
                            int *headIdxInOut, uint16_t *lastIdxInOut)
{
   const uint8_t *p = (const uint8_t *) data;
   size_t remain = len;
   int headIdx;
   uint16_t lastIdx;
   uint16_t curDataLen;
   uint16_t nextIdx;
   char_t tmpName[FLASHLINK_NAME_SIZE];
   error_t error;

   if(!g_flashLink.ready || (NULL == name) || ('\0' == name[0]) || (NULL == data))
      return ERROR_INVALID_PARAMETER;

   if(*headIdxInOut >= 0)
   {
      //existing file
      headIdx = *headIdxInOut;
      lastIdx = *lastIdxInOut;
   }
   else if(*headIdxInOut < 0)
   {
      /* new file, last = head (single cell) */
      headIdx = flFindFreeCell();
      if(headIdx < 0)
         return ERROR_OUT_OF_RESOURCES;

      error = flProgName((uint16_t)headIdx, name);
      if(error)
         return error;

      // last = head (single cell)
      *headIdxInOut = headIdx;      
      *lastIdxInOut = (uint16_t)headIdx;
   }

   if(flReadHeader(lastIdx, tmpName, &nextIdx, &curDataLen))
      return ERROR_FAILURE;

   if((0 == curDataLen) && (remain > 0))
   {
      uint16_t chunk = (remain > FLASHLINK_CELL_DATA_SIZE) ?
              FLASHLINK_CELL_DATA_SIZE : (uint16_t)remain;
      error = flWriteData(lastIdx, 0, p, chunk);
      if(error)
         return error;

      error = flProgLen(lastIdx, chunk);
      if(error)
         return error;

      p += chunk;
      remain -= chunk;
   }

   while(remain > 0)
   {
      int freeIdx = flFindFreeCell();
      uint16_t chunk;

      if(freeIdx < 0)
         return ERROR_OUT_OF_RESOURCES;

      error = flReadHeader(lastIdx, tmpName, &nextIdx, &curDataLen);
      if(error)
         return error;

      nextIdx = (uint16_t) freeIdx;
      error = flProgNext(lastIdx, nextIdx);
      if(error)
         return error;

      chunk = (remain > FLASHLINK_CELL_DATA_SIZE) ?
              FLASHLINK_CELL_DATA_SIZE : (uint16_t)remain;
      error = flProgLen((uint16_t)freeIdx, chunk);
      if(error)
         return error;

      error = flWriteData((uint16_t)freeIdx, 0, p, chunk);
      if(error)
         return error;

      lastIdx = (uint16_t)freeIdx;
      if(lastIdxInOut != NULL)
         *lastIdxInOut = lastIdx;
      p += chunk;
      remain -= chunk;
   }

   return NO_ERROR;
}

/***************************** PUBLIC FUNCTIONS  ******************************/

error_t flashLinkInit(const FlashLinkOps *ops, const FlashLinkConfig *cfg)
{
   if((NULL == ops) || (NULL == cfg))
      return ERROR_INVALID_PARAMETER;
   if((NULL == ops->read) || (NULL == ops->prog) || (NULL == ops->erase) || (NULL == ops->sync))
      return ERROR_INVALID_PARAMETER;
   if((0 == cfg->regionSize) || (0 == cfg->baseAddr))
      return ERROR_INVALID_PARAMETER;

   osMemset(&g_flashLink, 0, sizeof(g_flashLink));

   g_flashLink.ops            = *ops;
   g_flashLink.baseAddr       = cfg->baseAddr;
   g_flashLink.regionSize     = cfg->regionSize;
   g_flashLink.eraseBlockSize = cfg->eraseBlockSize;

   if(0 != cfg->cellCount)
   {
      g_flashLink.cellCount = cfg->cellCount;
   }
   else
   {
      g_flashLink.cellCount = (uint16_t)(cfg->regionSize / FLASHLINK_CELL_SIZE);
   }

   if(g_flashLink.cellCount == 0)
   {
      return ERROR_INVALID_PARAMETER;
   }

   g_flashLink.ready = TRUE;
   return NO_ERROR;
}

error_t flashLinkFormat(void)
{
   uint32_t addr;
   uint32_t end;
   error_t error = ERROR_WRITE_FAILED;

   if(!g_flashLink.ready)
   {
      return ERROR_NOT_READY;
   }

   end = g_flashLink.baseAddr + g_flashLink.regionSize;
   for(addr = g_flashLink.baseAddr; addr < end; addr += g_flashLink.eraseBlockSize)
   {
      error = g_flashLink.ops.erase(addr);
      if(error)
      {
         break;
      }         
   }

   return error;
}

error_t flashLinkRead(const char_t *name, void *buf, size_t bufLen, size_t *outLen)
{
   uint8_t *dst = (uint8_t *) buf;
   size_t remaining = bufLen;
   size_t written = 0;
   int headIdx;
   uint16_t curIdx;
   error_t error;

   if(!g_flashLink.ready || (NULL == name) || ('\0' == name[0]) || (NULL == buf) || (NULL == outLen))
   {
      return ERROR_INVALID_PARAMETER;
   }

   *outLen = 0;

   headIdx = flFindHeadByName(name);
   if(headIdx < 0)
      return ERROR_FILE_NOT_FOUND;

   curIdx = (uint16_t) headIdx;

   while(curIdx != 0)
   {
      char_t cellName[FLASHLINK_NAME_SIZE];
      uint16_t nextIdx;
      uint16_t dataLen;

      error = flReadHeader(curIdx, cellName, &nextIdx, &dataLen);
      if(error)
         return error;

      if(dataLen > 0)
      {
         uint8_t tmp[FLASHLINK_CELL_DATA_SIZE];
         uint16_t toCopy = dataLen;

         if(toCopy > FLASHLINK_CELL_DATA_SIZE)
            toCopy = FLASHLINK_CELL_DATA_SIZE;

         error = flRead(flCellAddr(curIdx) + FLASHLINK_NAME_SIZE + 4,
                        tmp, toCopy);
         if(error)
            return error;

         if(toCopy > remaining)
            toCopy = (uint16_t)remaining;

         if(toCopy > 0)
         {
            osMemcpy(dst, tmp, toCopy);
            dst += toCopy;
            remaining -= toCopy;
            written += toCopy;
         }

         if(remaining == 0)
            break;
      }

      curIdx = nextIdx;
   }

   *outLen = written;
   return NO_ERROR;
}

error_t flashLinkDelete(const char_t *name)
{
   int headIdx;
   uint16_t curIdx;
   error_t error;

   if(!g_flashLink.ready || (NULL == name) || ('\0' == name[0]))
      return ERROR_INVALID_PARAMETER;

   headIdx = flFindHeadByName(name);
   if(headIdx < 0)
      return ERROR_FILE_NOT_FOUND;

   curIdx = (uint16_t)headIdx;

   while(curIdx != 0)
   {
      uint16_t nextIdx;
      char_t dummyName[FLASHLINK_NAME_SIZE];
      uint16_t dataLen;

      error = flReadHeader(curIdx, dummyName, &nextIdx, &dataLen);
      if(error)
         return error;

      // Clear header (name + nextIndex + dataLen)
      error = flClearCellHeader(curIdx);
      if(error)
         return error;

      if(nextIdx == 0)
         break;

      curIdx = nextIdx;
   }

   return NO_ERROR;
}

/**** fs_port API implementation ***/
static const char_t *flPathToName(const char_t *path, char_t *nameBuf, size_t bufLen)
{
   if((NULL == path) || (NULL == nameBuf) || (0 == bufLen))
   {
      return NULL;
   }

   while('/' == *path)
   {
      path++;
   }
     
   osStrncpy(nameBuf, path, (bufLen - 1));
   nameBuf[(bufLen - 1)] = '\0';

   if('\0' == nameBuf[0])
   {
      return NULL;
   }

   return nameBuf;
}

static error_t flGetTotalSizeEx(const char_t *name, uint32_t *outSize, int knownHeadIdx)
{
   int headIdx;
   uint16_t curIdx;
   uint32_t total = 0;
   error_t error;

   if((NULL == name) || (NULL == outSize))
      return ERROR_INVALID_PARAMETER;

   *outSize = 0;
   headIdx = (knownHeadIdx >= 0) ? knownHeadIdx : flFindHeadByName(name);
   if(headIdx < 0)
      return ERROR_FILE_NOT_FOUND;

   curIdx = (uint16_t)headIdx;
   while(curIdx != 0)
   {
      char_t dummyName[FLASHLINK_NAME_SIZE];
      uint16_t nextIdx;
      uint16_t dataLen;

      error = flReadHeader(curIdx, dummyName, &nextIdx, &dataLen);
      if(error)
         return error;

      total += dataLen;
      curIdx = nextIdx;
   }

   *outSize = total;
   return NO_ERROR;
}

static error_t flGetTotalSize(const char_t *name, uint32_t *outSize)
{
   return flGetTotalSizeEx(name, outSize, -1);
}

static void flSetUnknownModified(DateTime *dt)
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

static FlashLinkFileHandle *flAllocFileSlot(void)
{
   uint32_t i;
   for(i = 0; i < FLASHLINK_MAX_OPEN_FILES; i++)
   {
      if(!g_filePool[i].inUse)
      {
         g_filePool[i].inUse = TRUE;
         osMemset(&g_filePool[i].h, 0, sizeof(g_filePool[i].h));
         return &g_filePool[i].h;
      }
   }
   return NULL;
}

static void flFreeFileSlot(FlashLinkFileHandle *h)
{
   uint32_t i;
   for(i = 0; i < FLASHLINK_MAX_OPEN_FILES; i++)
   {
      if(&g_filePool[i].h == h)
      {
         g_filePool[i].inUse = FALSE;
         return;
      }
   }
}

static FlashLinkDirHandle *flAllocDirSlot(void)
{
   uint32_t i;
   for(i = 0; i < FLASHLINK_MAX_OPEN_DIRS; i++)
   {
      if(!g_dirPool[i].inUse)
      {
         g_dirPool[i].inUse = TRUE;
         osMemset(&g_dirPool[i].h, 0, sizeof(g_dirPool[i].h));
         return &g_dirPool[i].h;
      }
   }
   return NULL;
}

static void flFreeDirSlot(FlashLinkDirHandle *d)
{
   uint32_t i;
   for(i = 0; i < FLASHLINK_MAX_OPEN_DIRS; i++)
   {
      if(&g_dirPool[i].h == d)
      {
         g_dirPool[i].inUse = FALSE;
         return;
      }
   }
}

error_t fsInit(void)
{
   error_t retVal = NO_ERROR;

   if(!g_flashLink.ready)
   {
      retVal = ERROR_NOT_READY;
   }

   return retVal;
}

bool_t fsFileExists(const char_t *path)
{
   char_t name[FLASHLINK_NAME_SIZE + 1];

   if((NULL == path) || (FALSE == g_flashLink.ready))
      return FALSE;

   if(NULL == flPathToName(path, name, sizeof(name)))
      return FALSE;

   return (flFindHeadByName(name) >= 0) ? TRUE : FALSE;
}

error_t fsGetFileSize(const char_t *path, uint32_t *size)
{
   char_t name[FLASHLINK_NAME_SIZE + 1];

   if((NULL == path) || ((NULL == size) || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;
   if(NULL == flPathToName(path, name, sizeof(name)))
      return ERROR_INVALID_PARAMETER;

   return flGetTotalSize(name, size);
}

error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat)
{
   char_t name[FLASHLINK_NAME_SIZE + 1];
   uint32_t size;
   error_t error;

   if((NULL == path) || (NULL == fileStat) || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;

   osMemset(fileStat, 0, sizeof(FsFileStat));

   if(NULL == flPathToName(path, name, sizeof(name)))
   {
      if((0 == osStrcmp(path, "/")) || (0 == osStrcmp(path, "")))
      {
         fileStat->attributes = FS_FILE_ATTR_DIRECTORY;
         return NO_ERROR;
      }
      return ERROR_INVALID_PARAMETER;
   }

   error = flGetTotalSize(name, &size);
   if(error)
      return error;

   fileStat->attributes = 0;
   fileStat->size = size;
   flSetUnknownModified(&fileStat->modified);
   return NO_ERROR;
}

error_t fsRenameFile(const char_t *oldPath, const char_t *newPath)
{
   char_t oldName[FLASHLINK_NAME_SIZE + 1];
   char_t newName[FLASHLINK_NAME_SIZE + 1];
   int headIdx;
   error_t error;

   if((NULL == oldPath) || (NULL == newPath) || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;
   if(NULL == flPathToName(oldPath, oldName, sizeof(oldName)))
      return ERROR_INVALID_PARAMETER;
   if(NULL == flPathToName(newPath, newName, sizeof(newName)))
      return ERROR_INVALID_PARAMETER;
   
      if(flFindHeadByName(newName) >= 0)
      return ERROR_ALREADY_EXISTS;

   headIdx = flFindHeadByName(oldName);
   if(headIdx < 0)
      return ERROR_FILE_NOT_FOUND;

   /* just update the name in the head cell; the data remains the same */
   error = flProgName((uint16_t)headIdx, newName);
   if(error)
      return error;

   return NO_ERROR;
}

error_t fsDeleteFile(const char_t *path)
{
   char_t name[FLASHLINK_NAME_SIZE + 1];

   if((NULL == path) || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;
   if(NULL == flPathToName(path, name, sizeof(name)))
      return ERROR_INVALID_PARAMETER;

   return flashLinkDelete(name);
}

FsFile *fsOpenFile(const char_t *path, uint_t mode)
{
   char_t name[FLASHLINK_NAME_SIZE + 1];
   FlashLinkFileHandle *h;
   uint32_t size;
   error_t error;

   if((NULL == path) || (FALSE == g_flashLink.ready))
      return NULL;
   if(NULL == flPathToName(path, name, sizeof(name)))
      return NULL;

   int headIdx = -1;

   if(0 == (mode & FS_FILE_MODE_CREATE))
   {
      headIdx = flFindHeadByName(name);
      if(headIdx < 0)
      {
         if(0 == (mode & FS_FILE_MODE_WRITE))
            return NULL; /* Okuma modu, dosya yok */
         return NULL; /* WRITE var ama CREATE yok: olmayan dosyayı yazma için açmak geçersiz */
      }
   }
   else
   {
      //FS_FILE_MODE_CREATE active
      headIdx = flFindHeadByName(name); /* -1 = yeni dosya, >=0 = mevcut */
   }

   if((0 == (mode & FS_FILE_MODE_WRITE)) && (0 == (mode & FS_FILE_MODE_READ)))
      return NULL;

   h = flAllocFileSlot();
   if(h == NULL)
      return NULL;

   osStrncpy(h->name, name, FLASHLINK_NAME_SIZE);
   h->name[FLASHLINK_NAME_SIZE] = '\0';

   h->writeMode = (mode & FS_FILE_MODE_WRITE) ? TRUE : FALSE;

   if(h->writeMode && (mode & FS_FILE_MODE_TRUNC))
   {
      (void)flashLinkDelete(name);
      h->headIdx = -1;  /* Sıfırdan yazılacak */
      h->lastIdx = 0;   /* İlk fsWriteFile'da set edilecek */
   }
   else
   {
      h->headIdx = headIdx;
      if(h->writeMode && (headIdx >= 0))
         h->lastIdx = flFindLastInChain((uint16_t)headIdx); /* Append: son hücreyi önceden yükle */
      /* headIdx < 0 (yeni dosya): lastIdx=0 zaten (flAllocFileSlot sıfırladı) */
   }

   if(!h->writeMode)
   {
      error = flGetTotalSizeEx(name, &size, headIdx);
      if(error)
      {
         flFreeFileSlot(h);
         return NULL;
      }
      h->totalSize = size;
   }

   return (FsFile *)h;
}

error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin)
{
   FlashLinkFileHandle *h;
   int32_t newPos;
   int32_t base;

   if((NULL == file)  || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;

   h = (FlashLinkFileHandle *)file;
   if(h->writeMode)
      return ERROR_NOT_IMPLEMENTED;

   if(origin == FS_SEEK_CUR)
      base = (int32_t)h->readOff;
   else if(origin == FS_SEEK_END)
      base = (int32_t)h->totalSize;
   else
      base = 0;

   newPos = base + offset;
   if(newPos < 0)
      return ERROR_INVALID_PARAMETER;

   if((uint32_t)newPos > h->totalSize)
      return ERROR_INVALID_PARAMETER;

   h->readOff = (uint32_t)newPos;
   return NO_ERROR;
}

error_t fsWriteFile(FsFile *file, void *data, size_t length)
{
   FlashLinkFileHandle *h;
   error_t error;

   if((NULL == file) || (NULL == data) || (FALSE == g_flashLink.ready))
      return ERROR_INVALID_PARAMETER;

   h = (FlashLinkFileHandle *)file;
   if(!h->writeMode)
      return ERROR_ACCESS_DENIED;

   error = flWriteChain(h->name, data, length, &h->headIdx, &h->lastIdx);
   return error;
}

error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length)
{
   FlashLinkFileHandle *h;
   error_t error;
   uint8_t tmp[FLASHLINK_CELL_DATA_SIZE];
   size_t totalRead = 0;
   size_t toRead;
   int headIdx;
   uint16_t curIdx;
   uint32_t offInChain = 0;

   if(file == NULL || data == NULL || length == NULL || !g_flashLink.ready)
      return ERROR_INVALID_PARAMETER;

   h = (FlashLinkFileHandle *)file;
   *length = 0;

   if(h->writeMode)
      return ERROR_ACCESS_DENIED;

   if(h->readOff >= h->totalSize)
      return ERROR_END_OF_FILE;

   headIdx = h->headIdx;
   if(headIdx < 0)
      return ERROR_FILE_NOT_FOUND;

   curIdx = (uint16_t)headIdx;
   toRead = size;

   while(curIdx != 0 && toRead > 0)
   {
      char_t dummyName[FLASHLINK_NAME_SIZE];
      uint16_t nextIdx;
      uint16_t dataLen;
      uint32_t cellStart;
      uint32_t cellEnd;

      error = flReadHeader(curIdx, dummyName, &nextIdx, &dataLen);
      if(error)
         return error;

      cellStart = offInChain;
      cellEnd = offInChain + dataLen;
      offInChain = cellEnd;

      if(h->readOff >= cellEnd)
      {
         curIdx = nextIdx;
         continue;
      }

      if(dataLen > 0)
      {
         uint32_t skip = (h->readOff > cellStart) ? (h->readOff - cellStart) : 0;
         uint32_t avail = dataLen - skip;
         size_t n = (avail < toRead) ? (size_t)avail : toRead;

         if(n > 0)
         {
            error = flRead(flCellAddr(curIdx) + FLASHLINK_NAME_SIZE + 4 + skip,
                          tmp, (size_t)n);
            if(error)
               return error;

            osMemcpy((uint8_t *)data + totalRead, tmp, n);
            totalRead += n;
            toRead -= n;
            h->readOff += (uint32_t)n;
         }
      }

      curIdx = nextIdx;
   }

   *length = totalRead;
   return (totalRead > 0) ? NO_ERROR : ERROR_END_OF_FILE;
}

void fsCloseFile(FsFile *file)
{
   if(file != NULL)
      flFreeFileSlot((FlashLinkFileHandle *)file);
}

bool_t fsDirExists(const char_t *path)
{
   if(!g_flashLink.ready || path == NULL)
      return FALSE;
   return (osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0) ? TRUE : FALSE;
}

error_t fsCreateDir(const char_t *path)
{
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

FsDir *fsOpenDir(const char_t *path)
{
   FlashLinkDirHandle *d;

   if(!g_flashLink.ready || path == NULL)
      return NULL;
   if(osStrcmp(path, "/") != 0 && osStrcmp(path, "") != 0)
      return NULL;

   d = flAllocDirSlot();
   if(d == NULL)
      return NULL;

   d->cursor = 0;
   return (FsDir *)d;
}

error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry)
{
   FlashLinkDirHandle *d;
   uint16_t i;
   char_t cellName[FLASHLINK_NAME_SIZE];

   if(dir == NULL || dirEntry == NULL || !g_flashLink.ready)
      return ERROR_INVALID_PARAMETER;

   d = (FlashLinkDirHandle *)dir;
   osMemset(dirEntry, 0, sizeof(FsDirEntry));

   for(i = (uint16_t)(d->cursor + 1); i <= g_flashLink.cellCount; i++)
   {
      uint16_t nextIdx, dataLen;

      if(flCellIsEmpty(i))
         continue;

      if(flReadHeader(i, cellName, &nextIdx, &dataLen))
         continue;

      if(cellName[0] == '\0' || cellName[0] == (char_t)0xFF)
         continue;

      d->cursor = i;
      dirEntry->attributes = 0;
      dirEntry->size = 0;
      flSetUnknownModified(&dirEntry->modified);
      strSafeCopy(dirEntry->name, cellName, FS_MAX_NAME_LEN);

      if(flGetTotalSize(cellName, &dirEntry->size))
         dirEntry->size = 0;

      return NO_ERROR;
   }

   return ERROR_END_OF_STREAM;
}

void fsCloseDir(FsDir *dir)
{
   if(dir != NULL)
      flFreeDirSlot((FlashLinkDirHandle *)dir);
}

/******************************** End Of File *********************************/