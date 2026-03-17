/**
 * @file fs_port_flashLink.c
 * @brief Very simple linked-cell storage in MCU internal flash
 */

#include <string.h>
#include "fs_port_flashLink.h"
#include "str.h"

typedef struct
{
   FlashLinkOps ops;
   uint32_t baseAddr;
   uint32_t regionSize;
   uint16_t cellCount;
   uint32_t eraseBlockSize;
   bool_t   ready;
} FlashLinkContext;

static FlashLinkContext g_flashLink;

// Internal helpers ----------------------------------------------------------//

static uint32_t flCellAddr(uint16_t index)
{
   // index is 1-based; 0 is "no cell"
   return g_flashLink.baseAddr + (uint32_t)(index - 1) * FLASHLINK_CELL_SIZE;
}

static error_t flRead(uint32_t addr, void *data, size_t len)
{
   if(g_flashLink.ops.read == NULL)
      return ERROR_NOT_READY;
   return g_flashLink.ops.read(addr, data, len);
}

static error_t flProg(uint32_t addr, const void *data, size_t len)
{
   if(g_flashLink.ops.prog == NULL)
      return ERROR_NOT_READY;
   return g_flashLink.ops.prog(addr, data, len);
}

static error_t flSync(void)
{
   if(g_flashLink.ops.sync == NULL)
      return NO_ERROR;
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
      strSafeCopy((char_t *)nameBuf, name, FLASHLINK_NAME_SIZE - 1);
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

   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;
   if(dataLen > FLASHLINK_CELL_DATA_SIZE)
      return ERROR_INVALID_PARAMETER;

   raw[0] = (uint8_t)(enc & 0xFF);
   raw[1] = (uint8_t)((enc >> 8) & 0xFF);

   addr = flCellAddr(index) + FLASHLINK_NAME_SIZE + 2;
   return flProg(addr, raw, sizeof(raw));
}
static error_t flWriteData(uint16_t index, uint16_t offset, const void *data, uint16_t len)
{
   uint32_t addr;

   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;
   if(offset + len > FLASHLINK_CELL_DATA_SIZE)
      return ERROR_INVALID_PARAMETER;

   addr = flCellAddr(index) + FLASHLINK_NAME_SIZE + 4 + offset;
   return flProg(addr, data, len);
}

static bool_t flCellIsEmpty(uint16_t index)
{
   uint8_t hdr[FLASHLINK_NAME_SIZE + 4];
   uint32_t addr;
   error_t error;
   size_t i;

   if(index == 0 || index > g_flashLink.cellCount)
      return FALSE;

   addr = flCellAddr(index);
   error = flRead(addr, hdr, sizeof(hdr));
   if(error)
      return FALSE;

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
      return TRUE;

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
      if(cellName[0] != '\0' && cellName[0] != (char_t)0xFF &&
         osStrcmp(cellName, name) == 0)
         return (int)i;
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
         break;
      if(next == 0)
         return cur;
      cur = next;
   }

   return headIndex;
}

static error_t flClearCellHeader(uint16_t index)
{
   if(index == 0 || index > g_flashLink.cellCount)
      return ERROR_INVALID_PARAMETER;
   // Tombstone delete for flash (only 1->0 allowed): program first name byte to 0x00.
   {
      uint8_t b = 0x00;
      uint32_t addr = flCellAddr(index);
      return flProg(addr, &b, 1);
   }
}

// Public API ----------------------------------------------------------------//

error_t flashLinkInit(const FlashLinkOps *ops, const FlashLinkConfig *cfg)
{
   if(ops == NULL || cfg == NULL)
      return ERROR_INVALID_PARAMETER;
   if(ops->read == NULL || ops->prog == NULL)
      return ERROR_INVALID_PARAMETER;
   if(cfg->regionSize == 0 || cfg->baseAddr == 0)
      return ERROR_INVALID_PARAMETER;

   osMemset(&g_flashLink, 0, sizeof(g_flashLink));

   g_flashLink.ops = *ops;
   g_flashLink.baseAddr = cfg->baseAddr;
   g_flashLink.regionSize = cfg->regionSize;
   g_flashLink.eraseBlockSize = cfg->eraseBlockSize;

   if(cfg->cellCount != 0)
   {
      g_flashLink.cellCount = cfg->cellCount;
   }
   else
   {
      g_flashLink.cellCount = (uint16_t)(cfg->regionSize / FLASHLINK_CELL_SIZE);
   }

   if(g_flashLink.cellCount == 0)
      return ERROR_INVALID_PARAMETER;

   g_flashLink.ready = TRUE;
   return NO_ERROR;
}

error_t flashLinkFormat(void)
{
   uint32_t addr;
   uint32_t end;
   error_t error;

   if(!g_flashLink.ready)
      return ERROR_NOT_READY;
   if(g_flashLink.ops.erase == NULL || g_flashLink.eraseBlockSize == 0)
      return ERROR_NOT_IMPLEMENTED;

   end = g_flashLink.baseAddr + g_flashLink.regionSize;
   for(addr = g_flashLink.baseAddr; addr < end; addr += g_flashLink.eraseBlockSize)
   {
      error = g_flashLink.ops.erase(addr);
      if(error)
         return error;
   }

   return flSync();
}

error_t flashLinkWrite(const char_t *name, const void *data, size_t len)
{
   const uint8_t *p = (const uint8_t *) data;
   size_t remain = len;
   int headIdx;
   uint16_t lastIdx;
   uint16_t curDataLen;
   uint16_t nextIdx;
   char_t tmpName[FLASHLINK_NAME_SIZE];
   error_t error;

   if(!g_flashLink.ready || name == NULL || name[0] == '\0' || data == NULL)
      return ERROR_INVALID_PARAMETER;

   // Find existing chain
   headIdx = flFindHeadByName(name);

   if(headIdx < 0)
   {
      // New chain: allocate first free cell
      headIdx = flFindFreeCell();
      if(headIdx < 0)
         return ERROR_OUT_OF_RESOURCES;

      // Program name only; next/len fields stay erased (0xFFFF => next=0,len=0)
      error = flProgName((uint16_t)headIdx, name);
      if(error)
         return error;
   }

   lastIdx = flFindLastInChain((uint16_t)headIdx);

   // Load last cell header
   if(flReadHeader(lastIdx, tmpName, &nextIdx, &curDataLen))
      return ERROR_FAILURE;

   // If this record has no data yet (len==0) we can fill the head cell once
   // and program its dataLen once (flash-friendly).
   if(curDataLen == 0 && remain > 0)
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

   // STM32F4 internal flash: we cannot increase dataLen in-place later.
   // Therefore append always allocates new cells (even if last cell has free space).

   // While data remains, allocate new cells and chain them
   while(remain > 0)
   {
      int freeIdx = flFindFreeCell();
      uint16_t chunk;

      if(freeIdx < 0)
         return ERROR_OUT_OF_RESOURCES;

      // Link previous last cell to new cell
      error = flReadHeader(lastIdx, tmpName, &nextIdx, &curDataLen);
      if(error)
         return error;
      nextIdx = (uint16_t) freeIdx;
      error = flProgNext(lastIdx, nextIdx);
      if(error)
         return error;

      chunk = (remain > FLASHLINK_CELL_DATA_SIZE) ?
              FLASHLINK_CELL_DATA_SIZE : (uint16_t)remain;

      // For chained cells, do not program name (stays 0xFF). Program len once.
      error = flProgLen((uint16_t)freeIdx, chunk);
      if(error)
         return error;

      error = flWriteData((uint16_t)freeIdx, 0, p, chunk);
      if(error)
         return error;

      lastIdx = (uint16_t)freeIdx;
      p += chunk;
      remain -= chunk;
   }

   return flSync();
}

error_t flashLinkRead(const char_t *name, void *buf, size_t bufLen, size_t *outLen)
{
   uint8_t *dst = (uint8_t *) buf;
   size_t remaining = bufLen;
   size_t written = 0;
   int headIdx;
   uint16_t curIdx;
   error_t error;

   if(!g_flashLink.ready || name == NULL || name[0] == '\0' || buf == NULL || outLen == NULL)
      return ERROR_INVALID_PARAMETER;

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

   if(!g_flashLink.ready || name == NULL || name[0] == '\0')
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

   return flSync();
}

