/**
 * @file fs_port_ZB.h
 * @brief Very simple flash-backed storage for logs, config and large data blobs.
 *
 * This module is independent from CycloneTCP fs_port_* back-ends. It provides
 * a small API to:
 *  - append logs in a cyclic region,
 *  - store versioned configuration structs (always return latest valid),
 *  - store large sensor / external data streams (chunked append, keep last N),
 *  - store temporary named blobs (key/value-like).
 *
 * All flash access is delegated to user-provided callbacks (MCU-specific).
 **/

#ifndef _FS_PORT_ZB_H
#define _FS_PORT_ZB_H

#include "os_port.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------//
// Flash callbacks and geometry
// ---------------------------------------------------------------------------//

typedef struct
{
   error_t (*read)(uint32_t addr, void *data, size_t len);
   error_t (*prog)(uint32_t addr, const void *data, size_t len);
   error_t (*erase)(uint32_t addr);
   error_t (*sync)(void);
} ZbFlashOps;

typedef struct
{
   uint32_t baseAddr;
   uint32_t totalSize;

   // Region split: [baseAddr, baseAddr + totalSize)
   // [baseAddr + logOffset,  baseAddr + logOffset  + logSize)  : LOG
   // [baseAddr + confOffset, baseAddr + confOffset + confSize) : CONF
   // [baseAddr + dataOffset, baseAddr + dataOffset + dataSize) : DATA
   uint32_t logOffset;
   uint32_t logSize;

   uint32_t confOffset;
   uint32_t confSize;

   uint32_t dataOffset;
   uint32_t dataSize;

   uint32_t eraseBlockSize;
   uint32_t progGranularity;
} ZbFlashLayout;

error_t zbInit(const ZbFlashOps *ops, const ZbFlashLayout *layout);

// Optional helpers to fully erase whole regions (dangerous)
error_t zbFormatLogRegion(void);
error_t zbFormatConfRegion(void);
error_t zbFormatDataRegion(void);

// ---------------------------------------------------------------------------//
// LOG region API (cyclic)
// ---------------------------------------------------------------------------//

/**
 * @brief Append log bytes into LOG region (cyclic).
 * When region is full, the oldest data is overwritten.
 */
error_t zbLogWrite(const void *data, size_t len);

// ---------------------------------------------------------------------------//
// CONF region API (versioned config)
// ---------------------------------------------------------------------------//

/**
 * @brief Store a new configuration record (append-only).
 * Old records are kept, latest valid record is returned by zbConfRead().
 */
error_t zbConfWrite(const void *conf, size_t len);

/**
 * @brief Read the latest valid configuration record.
 */
error_t zbConfRead(void *buf, size_t bufLen, size_t *outLen);

// ---------------------------------------------------------------------------//
// DATA region API (large streams and named blobs)
// ---------------------------------------------------------------------------//

typedef enum
{
   ZB_DATA_TYPE_SENSOR = 1,
   ZB_DATA_TYPE_BLOB   = 2
} ZbDataType;

typedef struct
{
   ZbDataType type;
   uint32_t   recOffset;   // absolute region offset of record header
   uint32_t   dataOffset;  // absolute region offset of payload
   uint32_t   totalLen;
   uint32_t   written;
   bool_t     complete;
} ZbDataHandle;

/**
 * @brief Begin a new data record (sensor or blob).
 * @param[in] name Optional name (can be NULL or empty for unnamed sensor data)
 * @param[in] type Record type (sensor/blob)
 * @param[out] handle Data handle used for subsequent append/read operations
 */
error_t zbDataBegin(const char_t *name, ZbDataType type, ZbDataHandle *handle);

/**
 * @brief Append a chunk to an open data record.
 */
error_t zbDataAppend(ZbDataHandle *handle, const void *chunk, size_t len);

/**
 * @brief Mark a data record as complete (finalize header).
 */
error_t zbDataEnd(ZbDataHandle *handle);

/**
 * @brief Open last completed record of given name (for blobs) or last sensor record.
 * If name is NULL or empty and type is SENSOR, the latest sensor record is opened.
 */
error_t zbDataOpenLast(const char_t *name, ZbDataType type, ZbDataHandle *handle);

/**
 * @brief Read from an opened data record (sequential).
 */
error_t zbDataRead(ZbDataHandle *handle, void *buf, size_t bufLen, size_t *outLen);

/**
 * @brief Convenience helpers for named blobs.
 */
error_t zbBlobWrite(const char_t *name, const void *data, size_t len);
error_t zbBlobRead(const char_t *name, void *buf, size_t bufLen, size_t *outLen);

#ifdef __cplusplus
}
#endif

#endif

