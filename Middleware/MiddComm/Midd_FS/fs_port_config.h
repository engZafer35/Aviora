/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jun 7, 2024 - 3:17:58 PM
* #File Name    : fs_port_config.h
*******************************************************************************/

/******************************************************************************
*
{
  "customer": "New_Costomer_ZD",
  "version": 1.1,
  "releaseDate": "2026-01-26",   
  "store": {
    "useStore": true,
    "fsLib": {
      "name": "flashLink",
      "srcPath": "fs_port_flashLink",
      "cellSize": 196,
      "nameSize": 64,
      "cellDataSize": 128
    },
    "extFlsh": {
      "type": "spi",
      "driverSrcPath": "StorageFlashPort_flashLink_stm32f407",
      "flashStartAdd": "0x080A0000",
      "flashSize": "128K",
      "readFunc": "Storage_FlashLinkRead",
      "progFunc": "Storage_FlashLinkProg",
      "eraseFunc": "Storage_FlashLinkErase",
      "syncFunc": "Storage_FlashLinkSync"
    }
  }
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __FS_PORT_CONFIG_H__
#define __FS_PORT_CONFIG_H__
/*********************************INCLUDES*************************************/
#include "error.h"

/* Error codes used by fs_port but not in CycloneTCP error.h */
#ifndef ERROR_NOT_READY
   #define ERROR_NOT_READY  9999
#endif
#ifndef ERROR_ALREADY_EXISTS
   #define ERROR_ALREADY_EXISTS  9998
#endif

#define USE_FLASHLINK 1

//FatFs port?
#if defined(USE_FATFS)
   #include "fs_port_fatfs.h"
//littleFS port?
#elif defined(USE_LITTLEFS)
   #include "fs_port_littlefs.h"
#elif defined(USE_FLASHFS)
   #include "fs_port_flashFS.h"
//FlashLink port?
#elif defined(USE_FLASHLINK)
   #include "fs_port_flashLink.h"
//Windows port?
#elif defined(_WIN32)
   #include "fs_port_posix.h"
//POSIX port?
#elif defined(__linux__) || defined(__FreeBSD__)
   #include "fs_port_posix.h"
//Custom port?
#elif defined(USE_CUSTOM_FS)
   #include "fs_port_custom.h"
#endif

/******************************MACRO DEFINITIONS*******************************/
#define FLASHLINK_BASE_ADDR       0x080A0000
#define FLASHLINK_REGION_SIZE     128*1024
#define FLASHLINK_CELL_SIZE       196
#define FLASHLINK_NAME_SIZE       64
#define FLASHLINK_CELL_DATA_SIZE  128


#define STORAGE_FLASH_LINK_READ_FUNC    Storage_FlashLinkRead
#define STORAGE_FLASH_LINK_PROG_FUNC    Storage_FlashLinkProg
#define STORAGE_FLASH_LINK_ERASE_FUNC   Storage_FlashLinkErase
#define STORAGE_FLASH_LINK_SYNC_FUNC    Storage_FlashLinkSync

/*******************************TYPE DEFINITIONS ******************************/

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

#endif /* __FS_PORT_CONFIG_H__ */

/********************************* End Of File ********************************/
