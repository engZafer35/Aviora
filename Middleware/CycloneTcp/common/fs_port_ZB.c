/**
 * @file fs_port_ZB.c
 * @brief Very simple flash-backed storage for logs, config and large blobs.
 */

#include <string.h>
#include "fs_port_ZB.h"
#include "str.h"

// Internal constants
#define ZB_MAGIC_LOG   0x5A424C4Fu /* 'ZBLO' */
#define ZB_MAGIC_CONF  0x5A424346u /* 'ZBCF' */
#define ZB_MAGIC_DATA  0x5A424441u /* 'ZBDA' */

#define ZB_CONF_MAX_RECORDS_SCAN  64
#define ZB_DATA_MAX_INDEX         20
#define ZB_NAME_MAX               64

typedef struct
{
   uint32_t magic;
   uint32_t version;
   uint32_t writeOff;    // offset inside region (for cyclic log)
   uint32_t wrapCount;
   uint32_t reserved[4];
} ZbLogHeader;

typedef struct
{
   uint32_t magic;
   uint32_t length;
   uint32_t seqNo;
   uint32_t crc;
} ZbConfHeader;

typedef struct
{
   uint32_t magic;
   uint32_t type;      // ZbDataType
   uint32_t seqNo;
   uint32_t totalLen;  // intended total length
   uint32_t written;   // bytes actually written
   uint32_t flags;     // bit0: complete
   uint32_t nameLen;
   uint32_t reserved;
} ZbDataHeader;

typedef struct
{
   bool_t     valid;
   ZbDataType type;
   char_t     name[ZB_NAME_MAX + 1];
   uint32_t   hdrOff;     // absolute region offset of header
   uint32_t   dataOff;    // absolute region offset of payload
   uint32_t   totalLen;
   uint32_t   written;
   bool_t     complete;
} ZbDataIndexEntry;

static struct
{
   ZbFlashOps   ops;
   ZbFlashLayout layout;
   bool_t       ready;

   // LOG
   ZbLogHeader  logHdr;

   // CONF
   uint32_t     confNextSeq;

   // DATA
   uint32_t     dataNextSeq;
   uint32_t     dataWriteOff; // offset inside data region
   ZbDataIndexEntry dataIndex[ZB_DATA_MAX_INDEX];
} zbCtx;

// ---------------------------------------------------------------------------//
// Small helpers
// ---------------------------------------------------------------------------//

static uint32_t zbAlignUp(uint32_t x, uint32_t a)
{
   if(a == 0)
      return x;
   return (x + (a - 1)) & ~(a - 1);
}

static error_t zbFlashRead(uint32_t absAddr, void *data, size_t len)
{
   if(zbCtx.ops.read == NULL)
      return ERROR_NOT_READY;
   return zbCtx.ops.read(absAddr, data, len);
}

static error_t zbFlashProg(uint32_t absAddr, const void *data, size_t len)
{
   if(zbCtx.ops.prog == NULL)
      return ERROR_NOT_READY;
   return zbCtx.ops.prog(absAddr, data, len);
}

static error_t zbFlashEraseBlock(uint32_t absAddr)
{
   if(zbCtx.ops.erase == NULL)
      return ERROR_NOT_READY;
   return zbCtx.ops.erase(absAddr);
}

static error_t zbFlashSync(void)
{
   if(zbCtx.ops.sync == NULL)
      return NO_ERROR;
   return zbCtx.ops.sync();
}

static uint32_t zbCrc32(const uint8_t *data, size_t len)
{
   uint32_t crc = 0xFFFFFFFFu;
   size_t i, j;

   for(i = 0; i < len; i++)
   {
      crc ^= data[i];
      for(j = 0; j < 8; j++)
      {
         if(crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320u;
         else
            crc >>= 1;
      }
   }

   return ~crc;
}

// ---------------------------------------------------------------------------//
// LOG region
// ---------------------------------------------------------------------------//

static error_t zbLogLoadHeader(void)
{
   error_t error;
   uint32_t abs = zbCtx.layout.baseAddr + zbCtx.layout.logOffset;

   error = zbFlashRead(abs, &zbCtx.logHdr, sizeof(zbCtx.logHdr));
   if(error)
      return error;

   if(zbCtx.logHdr.magic != ZB_MAGIC_LOG)
   {
      osMemset(&zbCtx.logHdr, 0, sizeof(zbCtx.logHdr));
      zbCtx.logHdr.magic = ZB_MAGIC_LOG;
      zbCtx.logHdr.version = 1;
      zbCtx.logHdr.writeOff = sizeof(ZbLogHeader);
      zbCtx.logHdr.wrapCount = 0;
      return zbFlashProg(abs, &zbCtx.logHdr, sizeof(zbCtx.logHdr));
   }

   return NO_ERROR;
}

static error_t zbLogSaveHeader(void)
{
   uint32_t abs = zbCtx.layout.baseAddr + zbCtx.layout.logOffset;
   return zbFlashProg(abs, &zbCtx.logHdr, sizeof(zbCtx.logHdr));
}

error_t zbLogWrite(const void *data, size_t len)
{
   const uint8_t *p = (const uint8_t *) data;
   size_t remain = len;
   uint32_t regionStart = zbCtx.layout.baseAddr + zbCtx.layout.logOffset;
   uint32_t regionEnd   = regionStart + zbCtx.layout.logSize;

   if(!zbCtx.ready || data == NULL || len == 0)
      return ERROR_INVALID_PARAMETER;

   if(zbCtx.layout.logSize <= sizeof(ZbLogHeader))
      return ERROR_FAILURE;

   while(remain > 0)
   {
      uint32_t absWrite = regionStart + zbCtx.logHdr.writeOff;
      uint32_t space = (uint32_t)(regionEnd - absWrite);

      if(space <= 0 || space == 0xFFFFFFFFu)
      {
         // wrap to start of data area
         zbCtx.logHdr.writeOff = (uint32_t) sizeof(ZbLogHeader);
         absWrite = regionStart + zbCtx.logHdr.writeOff;
         space = (uint32_t)(regionEnd - absWrite);
         zbCtx.logHdr.wrapCount++;
      }

      if(space > remain)
         space = (uint32_t) remain;

      // We do not erase here; assume log region formatted or erased beforehand.
      if(space == 0)
         break;

      (void) zbFlashProg(absWrite, p, space);

      zbCtx.logHdr.writeOff += space;
      p += space;
      remain -= space;
   }

   (void) zbLogSaveHeader();
   return zbFlashSync();
}

// ---------------------------------------------------------------------------//
// CONF region
// ---------------------------------------------------------------------------//

static error_t zbConfScan(void)
{
   error_t error;
   uint32_t off = zbCtx.layout.confOffset;
   uint32_t end = zbCtx.layout.confOffset + zbCtx.layout.confSize;
   uint32_t base = zbCtx.layout.baseAddr;
   uint32_t seqMax = 0;
   uint32_t scanCount = 0;

   while(off + sizeof(ZbConfHeader) <= end && scanCount < ZB_CONF_MAX_RECORDS_SCAN)
   {
      ZbConfHeader hdr;

      error = zbFlashRead(base + off, &hdr, sizeof(hdr));
      if(error)
         return error;

      if(hdr.magic != ZB_MAGIC_CONF || hdr.length == 0xFFFFFFFFu)
         break;

      if(hdr.length == 0 || off + sizeof(ZbConfHeader) + hdr.length > end)
         break;

      if(hdr.seqNo > seqMax)
         seqMax = hdr.seqNo;

      off += sizeof(ZbConfHeader) + zbAlignUp(hdr.length, zbCtx.layout.progMinSize);
      scanCount++;
   }

   zbCtx.confNextSeq = seqMax + 1;
   return NO_ERROR;
}

error_t zbConfWrite(const void *conf, size_t len)
{
   error_t error;
   uint32_t off = zbCtx.layout.confOffset;
   uint32_t end = zbCtx.layout.confOffset + zbCtx.layout.confSize;
   uint32_t base = zbCtx.layout.baseAddr;
   ZbConfHeader hdr;
   uint32_t crc;
   uint32_t needed;

   if(!zbCtx.ready || conf == NULL || len == 0)
      return ERROR_INVALID_PARAMETER;

   // Find end of existing records (simple linear scan)
   while(off + sizeof(ZbConfHeader) <= end)
   {
      ZbConfHeader cur;

      error = zbFlashRead(base + off, &cur, sizeof(cur));
      if(error)
         return error;

      if(cur.magic != ZB_MAGIC_CONF || cur.length == 0xFFFFFFFFu)
         break;

      if(cur.length == 0 || off + sizeof(ZbConfHeader) + cur.length > end)
         break;

      off += sizeof(ZbConfHeader) + zbAlignUp(cur.length, zbCtx.layout.progMinSize);
   }

   needed = (uint32_t) len;
   needed = sizeof(ZbConfHeader) + zbAlignUp(needed, zbCtx.layout.progMinSize);

   if(off + needed > end)
   {
      // Not enough room; upper layer should call zbFormatConfRegion() then retry
      return ERROR_OUT_OF_RESOURCES;
   }

   crc = zbCrc32((const uint8_t *)conf, len);

   osMemset(&hdr, 0xFF, sizeof(hdr));
   hdr.magic  = ZB_MAGIC_CONF;
   hdr.length = (uint32_t) len;
   hdr.seqNo  = zbCtx.confNextSeq++;
   hdr.crc    = crc;

   error = zbFlashProg(base + off, &hdr, sizeof(hdr));
   if(error)
      return error;

   error = zbFlashProg(base + off + sizeof(ZbConfHeader), conf, len);
   if(error)
      return error;

   return zbFlashSync();
}

error_t zbConfRead(void *buf, size_t bufLen, size_t *outLen)
{
   error_t error;
   uint32_t off = zbCtx.layout.confOffset;
   uint32_t end = zbCtx.layout.confOffset + zbCtx.layout.confSize;
   uint32_t base = zbCtx.layout.baseAddr;
   ZbConfHeader bestHdr;
   uint32_t bestOff = 0;
   bool_t found = FALSE;

   if(!zbCtx.ready || buf == NULL || outLen == NULL)
      return ERROR_INVALID_PARAMETER;

   osMemset(&bestHdr, 0, sizeof(bestHdr));
   *outLen = 0;

   while(off + sizeof(ZbConfHeader) <= end)
   {
      ZbConfHeader hdr;

      error = zbFlashRead(base + off, &hdr, sizeof(hdr));
      if(error)
         return error;

      if(hdr.magic != ZB_MAGIC_CONF || hdr.length == 0xFFFFFFFFu)
         break;

      if(hdr.length == 0 || off + sizeof(ZbConfHeader) + hdr.length > end)
         break;

      if(!found || hdr.seqNo > bestHdr.seqNo)
      {
         bestHdr = hdr;
         bestOff = off;
         found = TRUE;
      }

      off += sizeof(ZbConfHeader) + zbAlignUp(hdr.length, zbCtx.layout.progMinSize);
   }

   if(!found)
      return ERROR_FILE_NOT_FOUND;

   if(bestHdr.length > bufLen)
      return ERROR_BUFFER_OVERFLOW;

   {
      uint8_t *tmp = buf;
      error = zbFlashRead(base + bestOff + sizeof(ZbConfHeader), tmp, bestHdr.length);
      if(error)
         return error;
      if(zbCrc32(tmp, bestHdr.length) != bestHdr.crc)
         return ERROR_INVALID_SIGNATURE;
      *outLen = bestHdr.length;
   }

   return NO_ERROR;
}

// ---------------------------------------------------------------------------//
// DATA region
// ---------------------------------------------------------------------------//

static void zbDataIndexReset(void)
{
   osMemset(zbCtx.dataIndex, 0, sizeof(zbCtx.dataIndex));
}

static int zbDataIndexFindSlot(const char_t *name, ZbDataType type)
{
   uint_t i;

   if(name != NULL && name[0] != '\0')
   {
      for(i = 0; i < ZB_DATA_MAX_INDEX; i++)
      {
         if(zbCtx.dataIndex[i].valid &&
            zbCtx.dataIndex[i].type == type &&
            osStrcmp(zbCtx.dataIndex[i].name, name) == 0)
            return (int)i;
      }
   }

   // Find free slot
   for(i = 0; i < ZB_DATA_MAX_INDEX; i++)
   {
      if(!zbCtx.dataIndex[i].valid)
         return (int)i;
   }

   return -1;
}

static void zbDataIndexInsert(const char_t *name, ZbDataType type,
   uint32_t hdrOff, uint32_t dataOff, uint32_t totalLen, uint32_t written, bool_t complete)
{
   int idx = zbDataIndexFindSlot(name, type);
   if(idx < 0)
      return;

   zbCtx.dataIndex[idx].valid    = TRUE;
   zbCtx.dataIndex[idx].type     = type;
   zbCtx.dataIndex[idx].hdrOff   = hdrOff;
   zbCtx.dataIndex[idx].dataOff  = dataOff;
   zbCtx.dataIndex[idx].totalLen = totalLen;
   zbCtx.dataIndex[idx].written  = written;
   zbCtx.dataIndex[idx].complete = complete;

   if(name != NULL)
      strSafeCopy(zbCtx.dataIndex[idx].name, name, ZB_NAME_MAX);
   else
      zbCtx.dataIndex[idx].name[0] = '\0';
}

static int zbDataIndexFindLast(const char_t *name, ZbDataType type)
{
   uint_t i;
   int bestIdx = -1;
   uint32_t bestHdrOff = 0;

   for(i = 0; i < ZB_DATA_MAX_INDEX; i++)
   {
      if(!zbCtx.dataIndex[i].valid)
         continue;
      if(zbCtx.dataIndex[i].type != type)
         continue;
      if(name != NULL && name[0] != '\0')
      {
         if(osStrcmp(zbCtx.dataIndex[i].name, name) != 0)
            continue;
      }

      if(!zbCtx.dataIndex[i].complete)
         continue;

      if(!bestIdx || zbCtx.dataIndex[i].hdrOff > bestHdrOff)
      {
         bestHdrOff = zbCtx.dataIndex[i].hdrOff;
         bestIdx = (int)i;
      }
   }

   return bestIdx;
}

static error_t zbDataScan(void)
{
   error_t error;
   uint32_t off = zbCtx.layout.dataOffset;
   uint32_t end = zbCtx.layout.dataOffset + zbCtx.layout.dataSize;
   uint32_t base = zbCtx.layout.baseAddr;
   uint32_t seqMax = 0;

   zbCtx.dataWriteOff = zbCtx.layout.dataOffset;
   zbDataIndexReset();

   while(off + sizeof(ZbDataHeader) <= end)
   {
      ZbDataHeader hdr;

      error = zbFlashRead(base + off, &hdr, sizeof(hdr));
      if(error)
         return error;

      if(hdr.magic != ZB_MAGIC_DATA || hdr.type == 0xFFFFFFFFu)
         break;

      if(hdr.totalLen == 0 || hdr.nameLen > ZB_NAME_MAX)
         break;

      {
         uint32_t headerSize = sizeof(ZbDataHeader) + hdr.nameLen;
         uint32_t payloadOff = off + headerSize;
         uint32_t recEnd = payloadOff + hdr.totalLen;

         if(recEnd > end)
            break;

         if(hdr.seqNo > seqMax)
            seqMax = hdr.seqNo;

         // load name if any
         char_t name[ZB_NAME_MAX + 1];
         if(hdr.nameLen > 0)
         {
            error = zbFlashRead(base + off + sizeof(ZbDataHeader), name, hdr.nameLen);
            if(error)
               return error;
            name[hdr.nameLen] = '\0';
         }
         else
         {
            name[0] = '\0';
         }

         zbDataIndexInsert((hdr.nameLen > 0) ? name : NULL,
                           (ZbDataType)hdr.type,
                           off,
                           payloadOff,
                           hdr.totalLen,
                           hdr.written,
                           (hdr.flags & 0x1) ? TRUE : FALSE);

         off = recEnd;
      }
   }

   zbCtx.dataNextSeq = seqMax + 1;
   zbCtx.dataWriteOff = off;
   return NO_ERROR;
}

error_t zbDataBegin(const char_t *name, ZbDataType type, ZbDataHandle *handle)
{
   error_t error;
   uint32_t nameLen = 0;
   uint32_t headerSize;
   uint32_t off = zbCtx.dataWriteOff;
   uint32_t end = zbCtx.layout.dataOffset + zbCtx.layout.dataSize;
   uint32_t base = zbCtx.layout.baseAddr;
   ZbDataHeader hdr;

   if(!zbCtx.ready || handle == NULL)
      return ERROR_INVALID_PARAMETER;

   if(name != NULL)
      nameLen = (uint32_t) osStrlen(name);
   if(nameLen > ZB_NAME_MAX)
      return ERROR_INVALID_NAME;

   if(off == 0)
      off = zbCtx.layout.dataOffset;

   headerSize = sizeof(ZbDataHeader) + nameLen;
   if(off + headerSize >= end)
      return ERROR_OUT_OF_RESOURCES;

   osMemset(&hdr, 0xFF, sizeof(hdr));
   hdr.magic   = ZB_MAGIC_DATA;
   hdr.type    = (uint32_t) type;
   hdr.seqNo   = zbCtx.dataNextSeq++;
   hdr.totalLen = 0;     // unknown yet
   hdr.written  = 0;
   hdr.flags    = 0;     // incomplete
   hdr.nameLen  = nameLen;

   error = zbFlashProg(base + off, &hdr, sizeof(hdr));
   if(error)
      return error;

   if(nameLen > 0)
   {
      error = zbFlashProg(base + off + sizeof(ZbDataHeader), name, nameLen);
      if(error)
         return error;
   }

   handle->type      = type;
   handle->recOffset = off;
   handle->dataOffset = off + headerSize;
   handle->totalLen  = 0;
   handle->written   = 0;
   handle->complete  = FALSE;

   zbCtx.dataWriteOff = handle->dataOffset;
   zbFlashSync();
   return NO_ERROR;
}

error_t zbDataAppend(ZbDataHandle *handle, const void *chunk, size_t len)
{
   error_t error;
   uint32_t end = zbCtx.layout.dataOffset + zbCtx.layout.dataSize;
   uint32_t base = zbCtx.layout.baseAddr;

   if(!zbCtx.ready || handle == NULL || chunk == NULL || len == 0)
      return ERROR_INVALID_PARAMETER;

   if(handle->complete)
      return ERROR_FAILURE;

   if(zbCtx.dataWriteOff + (uint32_t)len > end)
      return ERROR_OUT_OF_RESOURCES;

   error = zbFlashProg(base + zbCtx.dataWriteOff, chunk, len);
   if(error)
      return error;

   handle->written   += (uint32_t)len;
   handle->totalLen  += (uint32_t)len;
   zbCtx.dataWriteOff += (uint32_t)len;

   return zbFlashSync();
}

error_t zbDataEnd(ZbDataHandle *handle)
{
   error_t error;
   ZbDataHeader hdr;
   uint32_t base = zbCtx.layout.baseAddr;

   if(!zbCtx.ready || handle == NULL)
      return ERROR_INVALID_PARAMETER;

   error = zbFlashRead(base + handle->recOffset, &hdr, sizeof(hdr));
   if(error)
      return error;

   if(hdr.magic != ZB_MAGIC_DATA)
      return ERROR_INVALID_SIGNATURE;

   hdr.totalLen = handle->totalLen;
   hdr.written  = handle->written;
   hdr.flags   |= 0x1; // complete

   error = zbFlashProg(base + handle->recOffset, &hdr, sizeof(hdr));
   if(error)
      return error;

   handle->complete = TRUE;

   // update index
   {
      char_t name[ZB_NAME_MAX + 1];
      if(hdr.nameLen > 0 && hdr.nameLen <= ZB_NAME_MAX)
      {
         error = zbFlashRead(base + handle->recOffset + sizeof(ZbDataHeader),
                             name, hdr.nameLen);
         if(error)
            return error;
         name[hdr.nameLen] = '\0';
      }
      else
      {
         name[0] = '\0';
      }

      zbDataIndexInsert((hdr.nameLen > 0) ? name : NULL,
                        (ZbDataType)hdr.type,
                        handle->recOffset,
                        handle->dataOffset,
                        hdr.totalLen,
                        hdr.written,
                        TRUE);
   }

   return zbFlashSync();
}

error_t zbDataOpenLast(const char_t *name, ZbDataType type, ZbDataHandle *handle)
{
   int idx;
   if(!zbCtx.ready || handle == NULL)
      return ERROR_INVALID_PARAMETER;

   idx = zbDataIndexFindLast(name, type);
   if(idx < 0)
      return ERROR_FILE_NOT_FOUND;

   handle->type       = zbCtx.dataIndex[idx].type;
   handle->recOffset  = zbCtx.dataIndex[idx].hdrOff;
   handle->dataOffset = zbCtx.dataIndex[idx].dataOff;
   handle->totalLen   = zbCtx.dataIndex[idx].totalLen;
   handle->written    = 0;
   handle->complete   = zbCtx.dataIndex[idx].complete;

   return NO_ERROR;
}

error_t zbDataRead(ZbDataHandle *handle, void *buf, size_t bufLen, size_t *outLen)
{
   error_t error;
   uint32_t base = zbCtx.layout.baseAddr;
   uint32_t remaining;
   uint32_t n;

   if(!zbCtx.ready || handle == NULL || buf == NULL || outLen == NULL)
      return ERROR_INVALID_PARAMETER;

   *outLen = 0;

   if(handle->written >= handle->totalLen)
      return ERROR_END_OF_FILE;

   remaining = handle->totalLen - handle->written;
   n = (remaining < (uint32_t)bufLen) ? remaining : (uint32_t)bufLen;

   error = zbFlashRead(base + handle->dataOffset + handle->written, buf, n);
   if(error)
      return error;

   handle->written += n;
   *outLen = n;
   return NO_ERROR;
}

error_t zbBlobWrite(const char_t *name, const void *data, size_t len)
{
   ZbDataHandle h;
   error_t error;

   if(name == NULL || name[0] == '\0')
      return ERROR_INVALID_NAME;

   error = zbDataBegin(name, ZB_DATA_TYPE_BLOB, &h);
   if(error)
      return error;

   error = zbDataAppend(&h, data, len);
   if(error)
      return error;

   return zbDataEnd(&h);
}

error_t zbBlobRead(const char_t *name, void *buf, size_t bufLen, size_t *outLen)
{
   ZbDataHandle h;
   error_t error;

   if(name == NULL || name[0] == '\0' || buf == NULL || outLen == NULL)
      return ERROR_INVALID_PARAMETER;

   error = zbDataOpenLast(name, ZB_DATA_TYPE_BLOB, &h);
   if(error)
      return error;

   // simple one-shot read
   return zbDataRead(&h, buf, bufLen, outLen);
}

// ---------------------------------------------------------------------------//
// Formatting helpers
// ---------------------------------------------------------------------------//

error_t zbFormatLogRegion(void)
{
   uint32_t start = zbCtx.layout.baseAddr + zbCtx.layout.logOffset;
   uint32_t end   = start + zbCtx.layout.logSize;
   uint32_t addr;
   error_t error;

   if(!zbCtx.ready)
      return ERROR_NOT_READY;

   for(addr = start; addr < end; addr += zbCtx.layout.eraseBlockSize)
   {
      error = zbFlashEraseBlock(addr);
      if(error)
         return error;
   }

   return zbLogLoadHeader();
}

error_t zbFormatConfRegion(void)
{
   uint32_t start = zbCtx.layout.baseAddr + zbCtx.layout.confOffset;
   uint32_t end   = start + zbCtx.layout.confSize;
   uint32_t addr;
   error_t error;

   if(!zbCtx.ready)
      return ERROR_NOT_READY;

   for(addr = start; addr < end; addr += zbCtx.layout.eraseBlockSize)
   {
      error = zbFlashEraseBlock(addr);
      if(error)
         return error;
   }

   zbCtx.confNextSeq = 1;
   return NO_ERROR;
}

error_t zbFormatDataRegion(void)
{
   uint32_t start = zbCtx.layout.baseAddr + zbCtx.layout.dataOffset;
   uint32_t end   = start + zbCtx.layout.dataSize;
   uint32_t addr;
   error_t error;

   if(!zbCtx.ready)
      return ERROR_NOT_READY;

   for(addr = start; addr < end; addr += zbCtx.layout.eraseBlockSize)
   {
      error = zbFlashEraseBlock(addr);
      if(error)
         return error;
   }

   zbCtx.dataNextSeq = 1;
   zbCtx.dataWriteOff = zbCtx.layout.dataOffset;
   zbDataIndexReset();
   return NO_ERROR;
}

// ---------------------------------------------------------------------------//
// Init
// ---------------------------------------------------------------------------//

error_t zbInit(const ZbFlashOps *ops, const ZbFlashLayout *layout)
{
   error_t error;

   if(ops == NULL || layout == NULL)
      return ERROR_INVALID_PARAMETER;

   osMemset(&zbCtx, 0, sizeof(zbCtx));

   zbCtx.ops    = *ops;
   zbCtx.layout = *layout;
   zbCtx.ready  = TRUE;

   // Load/init LOG
   error = zbLogLoadHeader();
   if(error)
      return error;

   // CONF scan
   error = zbConfScan();
   if(error)
      return error;

   // DATA scan
   error = zbDataScan();
   if(error)
      return error;

   return NO_ERROR;
}

