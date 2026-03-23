#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define FLASH_FILE "flash.bin"

static FILE *flashFile = NULL;

/* Flash init */
int simFlashInit(uint32_t size)
{
    flashFile = fopen(FLASH_FILE, "r+b");

    if (!flashFile)
    {
        flashFile = fopen(FLASH_FILE, "w+b");
        if (!flashFile) return -1;

        /* Fill with 0xFF */
        uint8_t ff = 0xFF;
        for (uint32_t i = 0; i < size; i++)
            fwrite(&ff, 1, 1, flashFile);
    }

    return 0;
}

/* READ */
error_t myFlashRead(uint32_t addr, void *data, size_t len)
{
    fseek(flashFile, addr, SEEK_SET);
    fread(data, 1, len, flashFile);
    return NO_ERROR;
}

/* PROGRAM (Flash rule: only 1→0 allowed!) */
error_t myFlashProg(uint32_t addr, const void *data, size_t len)
{
    uint8_t old, new;

    fseek(flashFile, addr, SEEK_SET);

    for (size_t i = 0; i < len; i++)
    {
        fread(&old, 1, 1, flashFile);

        new = ((uint8_t*)data)[i];

        /* Flash constraint check */
        if ((~old) & new)
        {
            printf("Flash write error: trying 0->1\n");
            return ERROR_FAILURE;
        }

        fseek(flashFile, addr + i, SEEK_SET);
        uint8_t programmed = old & new;
        fwrite(&programmed, 1, 1, flashFile);
    }

    return NO_ERROR;
}

/* ERASE (block → all FF) */
error_t myFlashErase(uint32_t addr)
{
    uint8_t ff = 0xFF;

    fseek(flashFile, addr, SEEK_SET);

    for (int i = 0; i < 4096; i++) // eraseBlockSize
        fwrite(&ff, 1, 1, flashFile);

    return NO_ERROR;
}

/* SYNC */
error_t myFlashSync(void)
{
    fflush(flashFile);
    return NO_ERROR;
}

FlashLinkOps ops = {
    .read  = myFlashRead,
    .prog  = myFlashProg,
    .erase = myFlashErase,
    .sync  = myFlashSync
};

FlashLinkConfig cfg = {
    .baseAddr = 0,              // FILE'da offset 0
    .regionSize = 128 * 1024,
    .cellCount = 1024,
    .eraseBlockSize = 4096
};

int main()
{
    simFlashInit(cfg.regionSize);

    flashLinkInit(&ops, &cfg);
    flashLinkFormat();

    fsInit();

    FsFile *f = fsOpenFile("test.txt", FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);

    char msg[] = "Hello Linux Flash Sim!";
    fsWriteFile(f, msg, sizeof(msg));

    fsCloseFile(f);

    char buf[64];
    size_t len;

    f = fsOpenFile("test.txt", FS_FILE_MODE_READ);
    fsReadFile(f, buf, sizeof(buf), &len);
    fsCloseFile(f);

    printf("Read: %.*s\n", (int)len, buf);

    return 0;
}