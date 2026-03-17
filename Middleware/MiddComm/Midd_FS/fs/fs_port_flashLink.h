/**
 * @file fs_port_flashLink.h
 * @brief Very simple linked-cell storage in MCU internal flash
 *
 * Fixed-size cells (196 bytes):
 *  - [ 64] name (ASCII, zero-terminated, left as 0xFF when unused)
 *  - [  2] next cell index (stored as ~nextIndex; erased 0xFFFF means nextIndex=0)
 *  - [  2] data length in this cell (stored as ~dataLen; erased 0xFFFF means dataLen=0)
 *  - [128] data payload
 *
 * Cells are indexed sequentially from 1..cellCount.
 * All operations are confined to a user-provided flash region.
 */

#ifndef _FS_PORT_FLASHLINK_H
#define _FS_PORT_FLASHLINK_H

#include "os_port.h"
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   error_t (*read)(uint32_t addr, void *data, size_t len);
   error_t (*prog)(uint32_t addr, const void *data, size_t len);
   error_t (*erase)(uint32_t addr);
   error_t (*sync)(void);
} FlashLinkOps;

typedef struct
{
   uint32_t baseAddr;   ///< Start of region in flash
   uint32_t regionSize; ///< Total bytes reserved for cells (must be multiple of 196)
   uint16_t cellCount;  ///< Optional; if 0, computed as regionSize / 196
   uint32_t eraseBlockSize; ///< Optional; if non-zero, can be used to format/erase region safely
} FlashLinkConfig;

/**
 * @brief Initialize FlashLink storage.
 */
error_t flashLinkInit(const FlashLinkOps *ops, const FlashLinkConfig *cfg);

/**
 * @brief Erase the whole FlashLink region (format).
 *
 * Requires ops->erase and cfg->eraseBlockSize.
 */
error_t flashLinkFormat(void);

/**
 * @brief Store or append data for given name.
 *
 * If no cell with this name exists, a new chain of cells is allocated.
 * If name already exists, data is appended to the existing chain
 * starting from the last cell (and using extra cells as needed).
 */
error_t flashLinkWrite(const char_t *name, const void *data, size_t len);

/**
 * @brief Read full data chain for given name.
 */
error_t flashLinkRead(const char_t *name, void *buf, size_t bufLen, size_t *outLen);

/**
 * @brief Delete data for given name (logical delete).
 *
 * Only the header part of each cell (name + nextIndex + dataLen) is cleared.
 * Data bytes remain in flash but are no longer reachable.
 */
error_t flashLinkDelete(const char_t *name);

#ifdef __cplusplus
}
#endif

#endif

