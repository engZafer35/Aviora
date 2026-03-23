/**
 * @file test_fs_port_flashLink.c
 * @brief Simple unit tests for fs_port_flashLink
 *
 * This is a standalone test file that can be compiled on host (POSIX/Windows)
 * together with fs_port_flashLink.c to verify basic behaviour.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fs_port_flashLink.h"
#include "error.h"

#define TEST_FLASH_SIZE   (196u * 32u)  /* 32 cells */

static uint8_t g_testFlash[TEST_FLASH_SIZE];

// -----------------------------------------------------------------------------
// Fake flash operations
// -----------------------------------------------------------------------------

static error_t testFlashRead(uint32_t addr, void *data, size_t len)
{
   if(addr + len > TEST_FLASH_SIZE)
      return ERROR_INVALID_PARAMETER;
   memcpy(data, &g_testFlash[addr], len);
   return NO_ERROR;
}

static error_t testFlashProg(uint32_t addr, const void *data, size_t len)
{
   size_t i;
   const uint8_t *p = (const uint8_t *)data;
   if(addr + len > TEST_FLASH_SIZE)
      return ERROR_INVALID_PARAMETER;
   /* Simulate real flash programming: only 1->0 transitions allowed */
   for(i = 0; i < len; i++)
   {
      g_testFlash[addr + i] = (uint8_t)(g_testFlash[addr + i] & p[i]);
   }
   return NO_ERROR;
}

static error_t testFlashErase(uint32_t addr)
{
   (void) addr;
   /* Not used in current tests; whole region is pre-erased at startup. */
   return NO_ERROR;
}

static error_t testFlashSync(void)
{
   return NO_ERROR;
}

static void flashClear(void)
{
   /* Simulate erased flash = 0xFF */
   memset(g_testFlash, 0xFF, sizeof(g_testFlash));
}

// -----------------------------------------------------------------------------
// Small assertion helpers
// -----------------------------------------------------------------------------

static int g_failed = 0;

static void assert_true(int cond, const char *msg)
{
   if(!cond)
   {
      printf("[FAIL] %s\n", msg);
      g_failed++;
   }
}

static void assert_eq_int(int exp, int got, const char *msg)
{
   if(exp != got)
   {
      printf("[FAIL] %s (exp=%d got=%d)\n", msg, exp, got);
      g_failed++;
   }
}

static void assert_eq_buf(const void *exp, const void *got, size_t len, const char *msg)
{
   if(memcmp(exp, got, len) != 0)
   {
      printf("[FAIL] %s (buffers differ)\n", msg);
      g_failed++;
   }
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

static void test_single_cell_write_read(void)
{
   FlashLinkOps ops;
   FlashLinkConfig cfg;
   error_t err;
   const char *name = "CONF";
   const uint8_t data[] = {1, 2, 3, 4, 5};
   uint8_t out[32];
   size_t outLen = 0;

   printf("test_single_cell_write_read...\n");

   flashClear();

   ops.read  = testFlashRead;
   ops.prog  = testFlashProg;
   ops.erase = testFlashErase;
   ops.sync  = testFlashSync;

   cfg.baseAddr   = 0;
   cfg.regionSize = TEST_FLASH_SIZE;
   cfg.cellCount  = 0; /* auto */
   cfg.eraseBlockSize = 0;

   err = flashLinkInit(&ops, &cfg);
   assert_eq_int(NO_ERROR, err, "flashLinkInit");

   err = flashLinkWrite(name, data, sizeof(data));
   assert_eq_int(NO_ERROR, err, "flashLinkWrite small");

   memset(out, 0, sizeof(out));
   err = flashLinkRead(name, out, sizeof(out), &outLen);
   assert_eq_int(NO_ERROR, err, "flashLinkRead small");
   assert_eq_int((int)sizeof(data), (int)outLen, "length match");
   assert_eq_buf(data, out, sizeof(data), "data match");
}

static void test_multi_cell_write_read(void)
{
   FlashLinkOps ops;
   FlashLinkConfig cfg;
   error_t err;
   const char *name = "BIGDATA";
   uint8_t data[400];
   uint8_t out[400];
   size_t outLen = 0;
   size_t i;

   printf("test_multi_cell_write_read...\n");

   flashClear();

   for(i = 0; i < sizeof(data); i++)
      data[i] = (uint8_t)(i & 0xFF);

   ops.read  = testFlashRead;
   ops.prog  = testFlashProg;
   ops.erase = testFlashErase;
   ops.sync  = testFlashSync;

   cfg.baseAddr   = 0;
   cfg.regionSize = TEST_FLASH_SIZE;
   cfg.cellCount  = 0;
   cfg.eraseBlockSize = 0;

   err = flashLinkInit(&ops, &cfg);
   assert_eq_int(NO_ERROR, err, "flashLinkInit");

   err = flashLinkWrite(name, data, sizeof(data));
   assert_eq_int(NO_ERROR, err, "flashLinkWrite multi");

   memset(out, 0, sizeof(out));
   err = flashLinkRead(name, out, sizeof(out), &outLen);
   assert_eq_int(NO_ERROR, err, "flashLinkRead multi");
   assert_eq_int((int)sizeof(data), (int)outLen, "multi length match");
   assert_eq_buf(data, out, sizeof(data), "multi data match");
}

static void test_append_existing(void)
{
   FlashLinkOps ops;
   FlashLinkConfig cfg;
   error_t err;
   const char *name = "APPEND";
   uint8_t first[100];
   uint8_t second[150];
   uint8_t expect[250];
   uint8_t out[250];
   size_t outLen = 0;
   size_t i;

   printf("test_append_existing...\n");

   flashClear();

   for(i = 0; i < sizeof(first); i++)
      first[i] = (uint8_t)(i + 1);
   for(i = 0; i < sizeof(second); i++)
      second[i] = (uint8_t)(0xA0 + i);

   memcpy(expect, first, sizeof(first));
   memcpy(expect + sizeof(first), second, sizeof(second));

   ops.read  = testFlashRead;
   ops.prog  = testFlashProg;
   ops.erase = testFlashErase;
   ops.sync  = testFlashSync;

   cfg.baseAddr   = 0;
   cfg.regionSize = TEST_FLASH_SIZE;
   cfg.cellCount  = 0;
   cfg.eraseBlockSize = 0;

   err = flashLinkInit(&ops, &cfg);
   assert_eq_int(NO_ERROR, err, "flashLinkInit");

   err = flashLinkWrite(name, first, sizeof(first));
   assert_eq_int(NO_ERROR, err, "flashLinkWrite first");

   err = flashLinkWrite(name, second, sizeof(second));
   assert_eq_int(NO_ERROR, err, "flashLinkWrite append");

   memset(out, 0, sizeof(out));
   err = flashLinkRead(name, out, sizeof(out), &outLen);
   assert_eq_int(NO_ERROR, err, "flashLinkRead append");
   assert_eq_int((int)sizeof(expect), (int)outLen, "append length match");
   assert_eq_buf(expect, out, sizeof(expect), "append data match");
}

static void test_delete(void)
{
   FlashLinkOps ops;
   FlashLinkConfig cfg;
   error_t err;
   const char *name = "TEMP";
   const uint8_t data[] = {10, 20, 30};
   uint8_t out[16];
   size_t outLen = 0;

   printf("test_delete...\n");

   flashClear();

   ops.read  = testFlashRead;
   ops.prog  = testFlashProg;
   ops.erase = testFlashErase;
   ops.sync  = testFlashSync;

   cfg.baseAddr   = 0;
   cfg.regionSize = TEST_FLASH_SIZE;
   cfg.cellCount  = 0;
   cfg.eraseBlockSize = 0;

   err = flashLinkInit(&ops, &cfg);
   assert_eq_int(NO_ERROR, err, "flashLinkInit");

   err = flashLinkWrite(name, data, sizeof(data));
   assert_eq_int(NO_ERROR, err, "flashLinkWrite");

   err = flashLinkDelete(name);
   assert_eq_int(NO_ERROR, err, "flashLinkDelete");

   memset(out, 0, sizeof(out));
   err = flashLinkRead(name, out, sizeof(out), &outLen);
   assert_true(err != NO_ERROR, "read after delete should fail");
}

int main(void)
{
   printf("Running fs_port_flashLink tests...\n");

   test_single_cell_write_read();
   test_multi_cell_write_read();
   test_append_existing();
   test_delete();

   if(g_failed == 0)
   {
      printf("All tests PASSED.\n");
      return 0;
   }
   else
   {
      printf("Tests FAILED: %d failures.\n", g_failed);
      return 1;
   }
}

