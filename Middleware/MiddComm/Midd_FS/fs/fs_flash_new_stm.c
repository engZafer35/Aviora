/******************************************************************************
 * @file  fs_flash_new_stm.c
 * @brief STM32F407VGTA iç-flash çok-bölgeli dosya sistemi — implementasyon
 *
 * ─────────────────────────────────────────────────────────────────────────
 * FLASH'TAKİ KAYIT YAPILARI
 * ─────────────────────────────────────────────────────────────────────────
 *
 * Bölge başlığı  (RegHdr, 16 byte)
 *   magic    u32  — bölge tipini tanımlar (CFG / LOG / DATA)
 *   version  u32  — format sürümü (REG_VERSION)
 *   size     u32  — toplam bölge boyutu
 *   reserved u32
 *
 * CONFIG kaydı başlığı  (CfgRecHdr, 32 byte)  + yol + veri
 *   magic    u32  — CFG_REC_MAGIC
 *   seq      u32  — artan sıra numarası (yüksek = yeni sürüm)
 *   dataSize u32  — yük byte sayısı
 *   crc32    u32  — CRC32(yol || veri)
 *   flags    u32  — 0xFFFFFFFF=geçerli, 0x00000000=silinmiş
 *   pathLen  u32  — yol string uzunluğu
 *   totSize  u32  — toplam kayıt byte sayısı (4-byte hizalı, başlık dahil)
 *   reserved u32
 *   ardından: yol (pathLen byte, 4-byte'a doldurulmuş)
 *   ardından: veri (dataSize byte, 4-byte'a doldurulmuş)
 *
 * LOG girişi başlığı  (LogRecHdr, 16 byte)  + metin
 *   magic    u32  — LOG_REC_MAGIC
 *   seq      u32  — artan sıra numarası
 *   textSize u32  — metin byte sayısı
 *   totSize  u32  — toplam girdi byte sayısı (4-byte hizalı, başlık dahil)
 *   ardından: metin (textSize byte, 4-byte'a doldurulmuş)
 *
 * DATA girişi başlığı  (DatRecHdr, 32 byte)  + yük
 *   magic    u32  — DAT_REC_MAGIC
 *   type     u32  — uygulama tanımlı tip ID
 *   seq      u32  — artan sıra numarası
 *   dataSize u32  — yük byte sayısı
 *   crc32    u32  — CRC32(yük)
 *   flags    u32  — 0xFFFFFFFF=geçerli
 *   ts       u32  — Unix zaman damgası
 *   totSize  u32  — toplam girdi byte sayısı (4-byte hizalı, başlık dahil)
 *   ardından: yük (dataSize byte, 4-byte'a doldurulmuş)
 *
 * ─────────────────────────────────────────────────────────────────────────
 * GÜÇ KESİLMESİ GÜVENCESI
 * ─────────────────────────────────────────────────────────────────────────
 *  • Son yazılan alan daima magic alanıdır; okuyucu önce magic'i doğrular.
 *  • CRC32 kontrolü bozuk kayıtları elenebilmesini sağlar.
 *  • Silme: mevcut kaydın flags alanına 0x00000000 yazılır (erase gerektirmez).
 *
 * @author  Zafer Satılmış
 * @date    Apr 2026
 ******************************************************************************/

#include "fs_flash_new_stm.h"

#include <string.h>
#include <CycloneTcp/common/str.h>
#include <CycloneTcp/common/date_time.h>
#include "Midd_OSPort.h"

/* =========================================================================
 * İç sabitler
 * ====================================================================== */

#define REG_VERSION           0x00010001u

#define REG_MAGIC_CFG         0x5CF60001u
#define REG_MAGIC_LOG         0x10CF0001u
#define REG_MAGIC_DATA        0xDA7F0001u

#define CFG_REC_MAGIC         0xCF570001u
#define LOG_REC_MAGIC         0x10C00001u
#define DAT_REC_MAGIC         0xDA7A0001u

#define FLAGS_VALID           0xFFFFFFFFu
#define FLAGS_DELETED         0x00000000u
#define ERASED_U32            0xFFFFFFFFu

/* Tüm çıktı: ortak yol-uzunluğu kontrol makrosu */
#define PATH_VALID(p)         ((p) != NULL && osStrlen(p) > 0 && \
                               osStrlen(p) <= FSNEW_PATH_MAX)

/* =========================================================================
 * Flash'taki yapılar
 * ====================================================================== */

/* Bölge başlığı — formatlama sırasında bir kez yazılır */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t reserved;
} RegHdr;  /* 16 byte */

/* CONFIG kaydı başlığı */
typedef struct
{
    uint32_t magic;
    uint32_t seq;
    uint32_t dataSize;
    uint32_t crc32;
    uint32_t flags;
    uint32_t pathLen;
    uint32_t totSize;
    uint32_t reserved;
} CfgRecHdr;  /* 32 byte */

/* LOG girişi başlığı
 * Flash düzeni: [LogRecHdr 20B] [path align4(pathLen)] [veri align4(dataSize)]
 * pathLen == 0 → anonim giriş (doğrudan flashNewStmWriteLog çağrısı)
 * pathLen  > 0 → path'li giriş (fsOpenFile+fsWriteFile üzerinden) */
typedef struct
{
    uint32_t magic;
    uint32_t seq;
    uint32_t dataSize;   /* veri yük boyutu (eski adı: textSize) */
    uint32_t totSize;    /* başlık + align4(pathLen) + align4(dataSize) */
    uint32_t pathLen;    /* 0 = anonim, >0 = path string uzunluğu */
} LogRecHdr;  /* 20 byte */

/* DATA girişi başlığı
 * Flash düzeni: [DatRecHdr 36B] [path align4(pathLen)] [veri align4(dataSize)]
 * pathLen == 0 → tip bazlı ölçüm kaydı (flashNewStmWriteData)
 * pathLen  > 0 → path'li dosya kaydı  (fsOpenFile+fsWriteFile)
 * flashNewStmIterateData yalnızca pathLen==0 kayıtları iter eder. */
typedef struct
{
    uint32_t magic;
    uint32_t type;
    uint32_t seq;
    uint32_t dataSize;
    uint32_t crc32;
    uint32_t flags;
    uint32_t ts;
    uint32_t totSize;
    uint32_t pathLen;    /* 0 = tip bazlı kayıt, >0 = path string uzunluğu */
    uint32_t reserved;   /* hizalama dolgusu */
} DatRecHdr;  /* 40 byte */

/* =========================================================================
 * RAM durumu
 * ====================================================================== */

/* CONFIG bölgesi RAM indeks girişi */
typedef struct
{
    bool_t   valid;
    char_t   path[FSNEW_PATH_MAX + 1u];
    uint32_t seq;
    uint32_t recOff;   /* CfgRecHdr'nin bölge başından ofseti */
    uint32_t dataOff;  /* veri yükünün bölge başından ofseti */
    uint32_t dataSize;
} CfgIdx;

/* Bölge çalışma zamanı durumu */
typedef struct
{
    uint32_t base;
    uint32_t size;
    uint32_t writeOff;   /* sonraki yazma konumu (bölge başından ofset) */
    uint32_t seqNext;    /* bir sonraki sıra numarası */
} RegState;

/* Açık dosya tutacağı tipi */
typedef enum { FH_CONFIG = 0, FH_LOG = 1, FH_DATA = 2 } FhType;

/* Açık dosya tutacağı */
typedef struct
{
    bool_t   inUse;
    FhType   ftype;
    bool_t   readMode;
    bool_t   writeMode;
    char_t   path[FSNEW_PATH_MAX + 1u];

    /* Yazma modu: dinamik tampon (CONFIG için) */
    uint8_t *wBuf;
    uint32_t wAlloc;
    uint32_t wLen;

    /* Okuma modu: CONFIG — tek sürekli kayıt, doğrudan flash */
    uint32_t rAbsBase;    /* veri yükünün mutlak flash adresi */
    uint32_t rSize;       /* toplam veri byte sayısı */
    uint32_t rPos;        /* mevcut okuma konumu */

    /* Okuma modu: LOG / DATA — heap yok, doğrudan flash'tan lazy scan
     *
     *   rRegOff     : bölge içinde bir sonraki tarama başlangıcı (record ofset)
     *   rRecDataOff : mevcut eşleşen kaydın veri yükü mutlak flash adresi
     *   rRecRemain  : mevcut kayıtta henüz okunmamış byte sayısı
     *
     * fsReadFile içinde:
     *   rRecRemain > 0  → mevcut kayıttan oku
     *   rRecRemain == 0 → flash'ta rRegOff'tan itibaren bir sonraki eşleşeni bul
     *   Eşleşen kayıt yok → ERROR_END_OF_FILE
     *
     * Tek bir fsReadFile çağrısında birden fazla kayıt tüketilebilir;
     * tampon boyutu aşılana kadar kayıtlar arası geçiş otomatik yapılır. */
    uint32_t rRegOff;     /* sonraki tarama başlangıcı (bölge başından ofset) */
    uint32_t rRecDataOff; /* mevcut kaydın veri yükü mutlak flash adresi */
    uint32_t rRecRemain;  /* mevcut kayıtta kalan byte sayısı */
} FsHandle;

/* Dizin tutacağı */
typedef struct
{
    uint32_t cursor;
    bool_t   isRoot;
    char_t   prefix[FSNEW_PATH_MAX + 1u];
} DirHandle;

/* Global bağlam */
typedef struct
{
    bool_t          configured;
    bool_t          mounted;
    FlashNewStmGeom geom;

    RegState        cfg;
    RegState        log;
    RegState        data;

    CfgIdx          cfgIdx[FSNEW_CFG_INDEX_SIZE];

    FsHandle        handles[FSNEW_MAX_OPEN_FILES];

    /* Thread-safety: tek flash controller → tek mutex.
     * STM32F407'nin HAL_FLASH_Unlock/Program/Lock sırası global bir
     * donanım işlemi olduğu için tüm bölge erişimleri aynı mutex ile
     * serileştirilmelidir; farklı sektörlere eş zamanlı erişim güvenli
     * değildir.                                                           */
    ZOsMutex         flashMux;
} FsNewCtx;

static FsNewCtx g_ctx;

/* ─── Kilit makroları (hepsi aynı mutex'i kullanır) ───────────────────── */
#define CFG_LOCK()    zosAcquireMutex(&g_ctx.flashMux)
#define CFG_UNLOCK()  zosReleaseMutex(&g_ctx.flashMux)
#define LOG_LOCK()    zosAcquireMutex(&g_ctx.flashMux)
#define LOG_UNLOCK()  zosReleaseMutex(&g_ctx.flashMux)
#define DAT_LOCK()    zosAcquireMutex(&g_ctx.flashMux)
#define DAT_UNLOCK()  zosReleaseMutex(&g_ctx.flashMux)

/* =========================================================================
 * CRC32  (yarı-tablolu, küçük MCU'ya uygun)
 * ====================================================================== */

static uint32_t crc32Update(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    static const uint32_t tbl[16] = {
        0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu,
        0x76DC4190u, 0x6B6B51F4u, 0x4DB26158u, 0x5005713Cu,
        0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu,
        0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu
    };
    uint32_t i;

    for (i = 0u; i < len; i++)
    {
        crc ^= (uint32_t)buf[i];
        crc  = (crc >> 4u) ^ tbl[crc & 0x0Fu];
        crc  = (crc >> 4u) ^ tbl[crc & 0x0Fu];
    }
    return crc;
}

static uint32_t crc32Buf(const uint8_t *buf, uint32_t len)
{
    return ~crc32Update(0xFFFFFFFFu, buf, len);
}

/* =========================================================================
 * Flash I/O yardımcıları
 * ====================================================================== */

static error_t flRead(uint32_t absAddr, void *buf, uint32_t len)
{
    if (!g_ctx.configured || g_ctx.geom.ops.read == NULL)
        return ERROR_NOT_READY;
    return (error_t)g_ctx.geom.ops.read(absAddr, buf, len);
}

static error_t flProg(uint32_t absAddr, const void *buf, uint32_t len)
{
    if (!g_ctx.configured || g_ctx.geom.ops.prog == NULL)
        return ERROR_NOT_READY;
    /* STM32F4: adres 4-byte hizalı olmalı */
    if ((absAddr % g_ctx.geom.progMin) != 0u)
        return ERROR_INVALID_PARAMETER;
    return (error_t)g_ctx.geom.ops.prog(absAddr, buf, len);
}

static error_t flErase(uint32_t absAddr)
{
    if (!g_ctx.configured || g_ctx.geom.ops.erase == NULL)
        return ERROR_NOT_READY;
    return (error_t)g_ctx.geom.ops.erase(absAddr);
}

static error_t flSync(void)
{
    if (g_ctx.geom.ops.sync != NULL)
        return (error_t)g_ctx.geom.ops.sync();
    return NO_ERROR;
}

/* 4-byte'a yukarı hizala */
static uint32_t align4(uint32_t x)
{
    return (x + 3u) & ~3u;
}

/*
 * len byte'lık veri yaz; geri kalan kısmı 0xFF ile doldur (4-byte hizaya).
 * Adres zaten 4-byte hizalı olmalıdır.
 */
static error_t flProgPad4(uint32_t absAddr, const void *buf, uint32_t len)
{

    return flProg(absAddr, buf, len);



//    static const uint8_t pad[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
//    uint32_t aligned = align4(len);
//    error_t  err;
//
//    if (len > 0u)
//    {
//        err = flProg(absAddr, buf, len);
//        if (err != NO_ERROR) return err;
//    }
//    if (aligned > len)
//    {
//        err = flProg(absAddr + len, pad, aligned - len);
//        if (err != NO_ERROR) return err;
//    }
//    return NO_ERROR;
}

/*
 * Tüm bloğu doldurarak 32-bit word yaz (STM32F4: her zaman word hizalı).
 * src NULL ise, tüm alan 0xFF ile doldurulur.
 */
static error_t flProgStruct(uint32_t absAddr, const void *src, uint32_t size)
{
    /* Boyut zaten 4'ün katı olmalı; aksi hâlde çağrı hatası */
    if ((size % 4u) != 0u) return ERROR_INVALID_PARAMETER;
    return flProg(absAddr, src, size);
}

/* =========================================================================
 * Bölge başlığı yazma / doğrulama
 * ====================================================================== */

static error_t writeRegHdr(uint32_t base, uint32_t magic, uint32_t size)
{
    RegHdr hdr;
    osMemset(&hdr, 0xFF, sizeof(hdr));
    hdr.magic   = magic;
    hdr.version = REG_VERSION;
    hdr.size    = size;
    hdr.reserved = 0xFFFFFFFFu;
    return flProgStruct(base, &hdr, sizeof(hdr));
}

static bool_t readRegHdr(uint32_t base, uint32_t expectedMagic,
                          uint32_t expectedSize)
{
    RegHdr hdr;
    if (flRead(base, &hdr, sizeof(hdr)) != NO_ERROR)
        return FALSE;
    return (hdr.magic   == expectedMagic &&
            hdr.version == REG_VERSION   &&
            hdr.size    == expectedSize) ? TRUE : FALSE;
}

/* =========================================================================
 * CONFIG bölgesi — iç işlevler
 * ====================================================================== */

/* Geçersiz bir bayt dizisi belirteci: kayıt yükleme başarısız */
#define CFG_IDX_NONE  ((uint32_t)0xFFFFFFFFu)

/*
 * cfgIdx'teki tüm girişleri sıfırla.
 */
static void cfgIdxClear(void)
{
    osMemset(g_ctx.cfgIdx, 0, sizeof(g_ctx.cfgIdx));
}

/*
 * Yol için mevcut indeks girişini bul (>=0) veya -1 döndür.
 */
static int32_t cfgIdxFind(const char_t *path)
{
    uint32_t i;
    for (i = 0u; i < FSNEW_CFG_INDEX_SIZE; i++)
    {
        if (g_ctx.cfgIdx[i].valid &&
            osStrcmp(g_ctx.cfgIdx[i].path, path) == 0)
        {
            return (int32_t)i;
        }
    }
    return -1;
}

/*
 * Boş indeks slotu bul (>=0) veya -1.
 */
static int32_t cfgIdxFreeSlot(void)
{
    uint32_t i;
    for (i = 0u; i < FSNEW_CFG_INDEX_SIZE; i++)
    {
        if (!g_ctx.cfgIdx[i].valid)
            return (int32_t)i;
    }
    return -1;
}

/*
 * CONFIG bölgesini baştan tarayarak RAM indeksini yeniden oluştur.
 * writeOff ve seqNext da bu sırada güncellenir.
 */
static void cfgScanAndRebuild(void)
{
    uint32_t   off = sizeof(RegHdr);
    CfgRecHdr  hdr;

    cfgIdxClear();
    g_ctx.cfg.writeOff = sizeof(RegHdr);
    g_ctx.cfg.seqNext  = 1u;

    while (off + sizeof(CfgRecHdr) <= g_ctx.cfg.size)
    {
        if (flRead(g_ctx.cfg.base + off, &hdr, sizeof(hdr)) != NO_ERROR)
            break;

        /* Silinmiş flash == log sonu */
        if (hdr.magic == ERASED_U32)
            break;

        /* Bozuk veya taşan kayıt → dur */
        if (hdr.magic != CFG_REC_MAGIC ||
            hdr.totSize < sizeof(CfgRecHdr) ||
            off + hdr.totSize > g_ctx.cfg.size)
            break;

        /* Sıra sayacını ilerlet */
        if (hdr.seq >= g_ctx.cfg.seqNext)
            g_ctx.cfg.seqNext = hdr.seq + 1u;

        /* Yol geçerliyse işle */
        if (hdr.pathLen > 0u && hdr.pathLen <= FSNEW_PATH_MAX)
        {
            char_t   path[FSNEW_PATH_MAX + 1u];
            uint32_t pathAbsOff = g_ctx.cfg.base + off + (uint32_t)sizeof(CfgRecHdr);

            if (flRead(pathAbsOff, path, hdr.pathLen) == NO_ERROR)
            {
                path[hdr.pathLen] = '\0';

                int32_t found = cfgIdxFind(path);
                int32_t slot  = found;

                if (slot < 0)
                    slot = cfgIdxFreeSlot();

                if (slot >= 0)
                {
                    if (hdr.flags == FLAGS_DELETED)
                    {
                        /* Silme kaydı: daha yeniyse indeks girişini sıfırla */
                        if (found >= 0 && hdr.seq >= g_ctx.cfgIdx[found].seq)
                            osMemset(&g_ctx.cfgIdx[found], 0,
                                     sizeof(g_ctx.cfgIdx[found]));
                    }
                    else if (hdr.flags == FLAGS_VALID)
                    {
                        /* Geçerli kayıt: daha yeniyse indeks güncelle */
                        if (found < 0 || hdr.seq > g_ctx.cfgIdx[found].seq)
                        {
                            uint32_t paddedPath = align4(hdr.pathLen);
                            g_ctx.cfgIdx[slot].valid    = TRUE;
                            strSafeCopy(g_ctx.cfgIdx[slot].path, path,
                                        FSNEW_PATH_MAX);
                            g_ctx.cfgIdx[slot].seq      = hdr.seq;
                            g_ctx.cfgIdx[slot].recOff   = off;
                            g_ctx.cfgIdx[slot].dataOff  =
                                off + (uint32_t)sizeof(CfgRecHdr) + paddedPath;
                            g_ctx.cfgIdx[slot].dataSize = hdr.dataSize;
                        }
                    }
                }
            }
        }

        off += hdr.totSize;
    }

    g_ctx.cfg.writeOff = off;
}

/*
 * CONFIG bölgesinde yeni kayıt için gereken byte sayısını hesapla.
 */
static uint32_t cfgRecTotSize(uint32_t pathLen, uint32_t dataSize)
{
    return (uint32_t)sizeof(CfgRecHdr)
           + align4(pathLen)
           + align4(dataSize);
}

/*
 * Dahili: CONFIG bölgesine bir kayıt yaz (bayrak alanı da dahil).
 * writeOff güncellenir.
 */
static error_t cfgWriteRecord(const char_t *path,
                               const void   *data,
                               uint32_t      dataSize,
                               uint32_t      flags)
{
    CfgRecHdr hdr;
    uint32_t  pathLen = (uint32_t)osStrlen(path);
    uint32_t  totSize = cfgRecTotSize(pathLen, dataSize);
    uint32_t  base    = g_ctx.cfg.base + g_ctx.cfg.writeOff;
    error_t   err;

    /* Yeterli alan var mı? */
    if (g_ctx.cfg.writeOff + totSize > g_ctx.cfg.size)
        return ERROR_OUT_OF_RESOURCES;

    /* CRC: yol || veri üzerinden hesapla */
    uint32_t crc = crc32Update(0xFFFFFFFFu,
                               (const uint8_t *)path, pathLen);
    if (dataSize > 0u && data != NULL)
        crc = crc32Update(crc, (const uint8_t *)data, dataSize);
    crc = ~crc;

    osMemset(&hdr, 0xFF, sizeof(hdr));
    hdr.seq      = g_ctx.cfg.seqNext++;
    hdr.dataSize = dataSize;
    hdr.crc32    = crc;
    hdr.flags    = flags;
    hdr.pathLen  = pathLen;
    hdr.totSize  = totSize;
    hdr.reserved = 0xFFFFFFFFu;
    /* magic en son yazılır (güç kesilmesi güvencesi) */

    /* Başlığı magic hariç yaz */
    {
        /* magic alanı 0. offset; geçici olarak 0xFF bırak */
        uint32_t hdrNoMagic[8];
        osMemcpy(hdrNoMagic, &hdr, sizeof(hdr));
        hdrNoMagic[0] = 0xFFFFFFFFu;  /* magic geçici olarak 0xFF */
        err = flProgStruct(base, hdrNoMagic, sizeof(hdr));
        if (err != NO_ERROR) return err;
    }

    /* Yol yaz */
    err = flProgPad4(base + (uint32_t)sizeof(CfgRecHdr),
                     path, pathLen);
    if (err != NO_ERROR) return err;

    /* Veri yaz */
    if (dataSize > 0u && data != NULL)
    {
        err = flProgPad4(base + (uint32_t)sizeof(CfgRecHdr) + align4(pathLen),
                         data, dataSize);
        if (err != NO_ERROR) return err;
    }

    /* magic alanını en son yaz */
    uint32_t magic = CFG_REC_MAGIC;
    err = flProgStruct(base, &magic, sizeof(magic));
    if (err != NO_ERROR) return err;

    err = flSync();
    if (err != NO_ERROR) return err;

    g_ctx.cfg.writeOff += totSize;
    return NO_ERROR;
}

/* =========================================================================
 * CONFIG sıkıştırma  (compact)
 *
 * compactConfig_impl  — kilitsiz, zaten kilitli bağlamdan çağrılır
 * flashNewStmCompactConfig — public wrapper, flashMux alır
 * ====================================================================== */

static error_t compactConfig_impl(void)
{
    if (!g_ctx.mounted) return ERROR_NOT_READY;

    /* ── 1. Geçerli tüm girişleri dinamik bellekte topla ────────────────── */
    typedef struct { char_t path[FSNEW_PATH_MAX + 1u]; uint8_t *buf; uint32_t sz; } Entry;
    Entry *entries = NULL;
    uint32_t n = 0u;
    error_t err = NO_ERROR;
    uint32_t i;

    /* Kaç geçerli giriş var? */
    for (i = 0u; i < FSNEW_CFG_INDEX_SIZE; i++)
    {
        if (g_ctx.cfgIdx[i].valid) n++;
    }
    if (n == 0u)
    {
        /* Bölgeyi sil, başlık yeniden yaz, geri dön */
        goto do_erase;
    }

    entries = (Entry *)osAllocMem(n * sizeof(Entry));
    if (entries == NULL) return ERROR_OUT_OF_MEMORY;
    osMemset(entries, 0, n * sizeof(Entry));

    {
        uint32_t ei = 0u;
        for (i = 0u; i < FSNEW_CFG_INDEX_SIZE; i++)
        {
            CfgIdx *idx = &g_ctx.cfgIdx[i];
            if (!idx->valid) continue;

            strSafeCopy(entries[ei].path, idx->path, FSNEW_PATH_MAX);
            entries[ei].sz = idx->dataSize;

            if (idx->dataSize > 0u)
            {
                entries[ei].buf = (uint8_t *)osAllocMem(idx->dataSize);
                if (entries[ei].buf == NULL)
                {
                    err = ERROR_OUT_OF_MEMORY;
                    goto cleanup;
                }
                if (flRead(g_ctx.cfg.base + idx->dataOff,
                           entries[ei].buf, idx->dataSize) != NO_ERROR)
                {
                    err = ERROR_READ_FAILED;
                    goto cleanup;
                }
            }
            ei++;
        }
    }

do_erase:
    /* ── 2. Bölgeyi sektör sektör sil ──────────────────────────────────── */
    {
        uint32_t off;
        for (off = 0u; off < g_ctx.cfg.size; off += g_ctx.geom.sectorSize)
        {
            err = flErase(g_ctx.cfg.base + off);
            if (err != NO_ERROR) goto cleanup;
        }
    }

    /* ── 3. Bölge başlığını yeniden yaz ────────────────────────────────── */
    err = writeRegHdr(g_ctx.cfg.base, REG_MAGIC_CFG, g_ctx.cfg.size);
    if (err != NO_ERROR) goto cleanup;

    g_ctx.cfg.writeOff = sizeof(RegHdr);
    g_ctx.cfg.seqNext  = 1u;
    cfgIdxClear();

    /* ── 4. Geçerli girişleri geri yaz ─────────────────────────────────── */
    for (i = 0u; i < n; i++)
    {
        err = cfgWriteRecord(entries[i].path,
                             entries[i].buf,
                             entries[i].sz,
                             FLAGS_VALID);
        if (err != NO_ERROR) goto cleanup;

        /* RAM indeksini güncelle */
        {
            uint32_t pathLen    = (uint32_t)osStrlen(entries[i].path);
            uint32_t prevOff    = g_ctx.cfg.writeOff - cfgRecTotSize(pathLen, entries[i].sz);
            int32_t  slot       = cfgIdxFreeSlot();

            if (slot >= 0)
            {
                g_ctx.cfgIdx[slot].valid    = TRUE;
                strSafeCopy(g_ctx.cfgIdx[slot].path, entries[i].path,
                            FSNEW_PATH_MAX);
                g_ctx.cfgIdx[slot].seq      = g_ctx.cfg.seqNext - 1u;
                g_ctx.cfgIdx[slot].recOff   = prevOff;
                g_ctx.cfgIdx[slot].dataOff  =
                    prevOff + (uint32_t)sizeof(CfgRecHdr) + align4(pathLen);
                g_ctx.cfgIdx[slot].dataSize = entries[i].sz;
            }
        }
    }

cleanup:
    if (entries != NULL)
    {
        for (i = 0u; i < n; i++)
        {
            if (entries[i].buf != NULL)
                osFreeMem(entries[i].buf);
        }
        osFreeMem(entries);
    }
    return err;
}

/* Public wrapper */
error_t flashNewStmCompactConfig(void)
{
    error_t err;
    CFG_LOCK();
    err = compactConfig_impl();
    CFG_UNLOCK();
    return err;
}

/* =========================================================================
 * LOG / DATA bölgesi — iç işlevler
 * ====================================================================== */

/*
 * Bölgeyi tara: writeOff ve seqNext'i bul.
 * Log girişleri sadece eklenir; silme yoktur.
 */
static void logDataScanRegion(RegState *reg, uint32_t hdrMagic,
                               uint32_t hdrSize)
{
    uint32_t off = sizeof(RegHdr);
    uint32_t magic_field;

    reg->writeOff = sizeof(RegHdr);
    reg->seqNext  = 1u;

    while (off + hdrSize <= reg->size)
    {
        if (flRead(reg->base + off, &magic_field, sizeof(magic_field)) != NO_ERROR)
            break;

        if (magic_field == ERASED_U32)
            break;

        if (magic_field != hdrMagic)
            break;  /* Bilinmeyen magic → tara */

        if (hdrMagic == LOG_REC_MAGIC)
        {
            LogRecHdr lh;
            if (flRead(reg->base + off, &lh, sizeof(lh)) != NO_ERROR)
                break;
            if (lh.totSize < sizeof(lh) || off + lh.totSize > reg->size)
                break;
            if (lh.seq >= reg->seqNext)
                reg->seqNext = lh.seq + 1u;
            off += lh.totSize;
        }
        else /* DAT_REC_MAGIC */
        {
            DatRecHdr dh;
            if (flRead(reg->base + off, &dh, sizeof(dh)) != NO_ERROR)
                break;
            if (dh.totSize < sizeof(dh) || off + dh.totSize > reg->size)
                break;
            if (dh.seq >= reg->seqNext)
                reg->seqNext = dh.seq + 1u;
            off += dh.totSize;
        }
    }

    reg->writeOff = off;
}

/*
 * Bölgeyi sil + başlığı yeniden yaz (LOG veya DATA).
 */
static error_t eraseAndReinitRegion(RegState *reg, uint32_t magic)
{
    uint32_t off;
    error_t  err;

    for (off = 0u; off < reg->size; off += g_ctx.geom.sectorSize)
    {
        err = flErase(reg->base + off);
        if (err != NO_ERROR) return err;
    }

    err = writeRegHdr(reg->base, magic, reg->size);
    if (err != NO_ERROR) return err;

    reg->writeOff = sizeof(RegHdr);
    return flSync();
}

/* =========================================================================
 * Başlatma / biçimlendirme — genel
 * ====================================================================== */

error_t flashNewStmConfigure(const FlashNewStmGeom *geom)
{
    if (geom == NULL)                        return ERROR_INVALID_PARAMETER;
    if (geom->ops.read  == NULL ||
        geom->ops.prog  == NULL ||
        geom->ops.erase == NULL)             return ERROR_INVALID_PARAMETER;
    if (geom->cfgSize == 0u ||
        geom->logSize == 0u ||
        geom->dataSize == 0u)                return ERROR_INVALID_PARAMETER;
    if (geom->sectorSize == 0u ||
        (geom->sectorSize & (geom->sectorSize - 1u)) != 0u)
                                             return ERROR_INVALID_PARAMETER;
    if (geom->progMin == 0u ||
        (geom->progMin & (geom->progMin - 1u)) != 0u)
                                             return ERROR_INVALID_PARAMETER;

    osMemset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.geom       = *geom;
    g_ctx.configured = TRUE;

    g_ctx.cfg.base   = geom->cfgBase;
    g_ctx.cfg.size   = geom->cfgSize;
    g_ctx.log.base   = geom->logBase;
    g_ctx.log.size   = geom->logSize;
    g_ctx.data.base  = geom->dataBase;
    g_ctx.data.size  = geom->dataSize;

    /* Mutex'leri oluştur */
    if (zosCreateMutex(&g_ctx.flashMux) != TRUE) return ERROR_FAILURE;

    return NO_ERROR;
}

void flashNewStmDeleteMutexes(void)
{
    zosDeleteMutex(&g_ctx.flashMux);
}

error_t flashNewStmFormat(void)
{
    error_t err;

    if (!g_ctx.configured) return ERROR_NOT_READY;

    /* CONFIG bölgesini sil + başlık */
    {
        uint32_t off;
        for (off = 0u; off < g_ctx.cfg.size; off += g_ctx.geom.sectorSize)
        {
            err = flErase(g_ctx.cfg.base + off);
            if (err != NO_ERROR) return err;
        }
        err = writeRegHdr(g_ctx.cfg.base, REG_MAGIC_CFG, g_ctx.cfg.size);
        if (err != NO_ERROR) return err;
    }

    /* LOG bölgesini sil + başlık */
    err = eraseAndReinitRegion(&g_ctx.log, REG_MAGIC_LOG);
    if (err != NO_ERROR) return err;

    /* DATA bölgesini sil + başlık */
    err = eraseAndReinitRegion(&g_ctx.data, REG_MAGIC_DATA);
    if (err != NO_ERROR) return err;

    return flSync();
}

/* =========================================================================
 * fsInit — standart fs_port giriş noktası
 * ====================================================================== */

error_t fsInit(void)
{
    if (!g_ctx.configured) return ERROR_NOT_READY;

    /* CONFIG bölgesi başlığını doğrula; geçersizse formatla */
    if (!readRegHdr(g_ctx.cfg.base, REG_MAGIC_CFG, g_ctx.cfg.size))
    {
        error_t err = flashNewStmFormat();
        if (err != NO_ERROR) return err;
    }

    /* LOG ve DATA başlıklarını doğrula; geçersizse ilgili bölgeyi yeniden başlat */
    if (!readRegHdr(g_ctx.log.base, REG_MAGIC_LOG, g_ctx.log.size))
    {
        error_t err = eraseAndReinitRegion(&g_ctx.log, REG_MAGIC_LOG);
        if (err != NO_ERROR) return err;
    }

    if (!readRegHdr(g_ctx.data.base, REG_MAGIC_DATA, g_ctx.data.size))
    {
        error_t err = eraseAndReinitRegion(&g_ctx.data, REG_MAGIC_DATA);
        if (err != NO_ERROR) return err;
    }

    /* RAM indekslerini ve yazma işaretçilerini yeniden oluştur */
    cfgScanAndRebuild();
    logDataScanRegion(&g_ctx.log,  LOG_REC_MAGIC, sizeof(LogRecHdr));
    logDataScanRegion(&g_ctx.data, DAT_REC_MAGIC, sizeof(DatRecHdr));

    g_ctx.mounted = TRUE;
    return NO_ERROR;
}

/* =========================================================================
 * Yüksek seviyeli CONFIG yardımcıları
 *
 * *_impl   → kilitsiz; zaten flashMux'un tutulduğu bağlamdan çağrılır
 * public   → flashMux alır, _impl'i çağırır, flashMux'u bırakır
 * ====================================================================== */

/* ── writeConfig_impl ─────────────────────────────────────────────────── */
static error_t writeConfig_impl(const char *path,
                                 const void *data, uint32_t size)
{
    if (!g_ctx.mounted)                   return ERROR_NOT_READY;
    if (!PATH_VALID(path))                return ERROR_INVALID_PARAMETER;
    if (size > 0u && data == NULL)        return ERROR_INVALID_PARAMETER;
    if (size > FSNEW_CFG_MAX_PAYLOAD)     return ERROR_INVALID_PARAMETER;

    uint32_t pathLen = (uint32_t)osStrlen(path);
    uint32_t need    = cfgRecTotSize(pathLen, size);

    /* Yer yoksa compact et (zaten kilitliyiz, _impl'i çağır) */
    if (g_ctx.cfg.writeOff + need > g_ctx.cfg.size)
    {
        error_t err = compactConfig_impl();
        if (err != NO_ERROR) return err;

        if (g_ctx.cfg.writeOff + need > g_ctx.cfg.size)
            return ERROR_OUT_OF_RESOURCES;
    }

    error_t err = cfgWriteRecord(path, data, size, FLAGS_VALID);
    if (err != NO_ERROR) return err;

    /* RAM indeksini güncelle */
    {
        int32_t slot = cfgIdxFind(path);
        if (slot < 0) slot = cfgIdxFreeSlot();
        if (slot >= 0)
        {
            uint32_t prevOff = g_ctx.cfg.writeOff - need;
            g_ctx.cfgIdx[slot].valid    = TRUE;
            strSafeCopy(g_ctx.cfgIdx[slot].path, path, FSNEW_PATH_MAX);
            g_ctx.cfgIdx[slot].seq      = g_ctx.cfg.seqNext - 1u;
            g_ctx.cfgIdx[slot].recOff   = prevOff;
            g_ctx.cfgIdx[slot].dataOff  =
                prevOff + (uint32_t)sizeof(CfgRecHdr) + align4(pathLen);
            g_ctx.cfgIdx[slot].dataSize = size;
        }
    }

    return NO_ERROR;
}

/* ── deleteConfig_impl ────────────────────────────────────────────────── */
static error_t deleteConfig_impl(const char *path)
{
    if (!g_ctx.mounted)    return ERROR_NOT_READY;
    if (!PATH_VALID(path)) return ERROR_INVALID_PARAMETER;

    int32_t slot = cfgIdxFind(path);
    if (slot < 0) return ERROR_FILE_NOT_FOUND;

    CfgIdx  *idx      = &g_ctx.cfgIdx[slot];
    uint32_t flagsOff = g_ctx.cfg.base + idx->recOff +
                        (uint32_t)offsetof(CfgRecHdr, flags);
    uint32_t deleted  = FLAGS_DELETED;

    /* Bayrak alanına 0x00000000 yaz (erase gerekmez — bit 1→0) */
    error_t err = flProgStruct(flagsOff, &deleted, sizeof(deleted));
    if (err != NO_ERROR) return err;

    err = flSync();
    if (err != NO_ERROR) return err;

    osMemset(&g_ctx.cfgIdx[slot], 0, sizeof(g_ctx.cfgIdx[slot]));
    return NO_ERROR;
}

/* ── Public wrappers ──────────────────────────────────────────────────── */

error_t flashNewStmWriteConfig(const char *path,
                                const void *data, uint32_t size)
{
    error_t err;
    CFG_LOCK();
    err = writeConfig_impl(path, data, size);
    CFG_UNLOCK();
    return err;
}

error_t flashNewStmReadConfig(const char *path,
                               void *buf, uint32_t maxSize,
                               uint32_t *outSize)
{
    if (!g_ctx.mounted)                  return ERROR_NOT_READY;
    if (!PATH_VALID(path))               return ERROR_INVALID_PARAMETER;
    if (buf == NULL || outSize == NULL)  return ERROR_INVALID_PARAMETER;

    CFG_LOCK();

    int32_t slot = cfgIdxFind(path);
    if (slot < 0)
    {
        CFG_UNLOCK();
        return ERROR_FILE_NOT_FOUND;
    }

    CfgIdx  *idx     = &g_ctx.cfgIdx[slot];
    uint32_t readLen = (idx->dataSize < maxSize) ? idx->dataSize : maxSize;
    error_t  err     = NO_ERROR;

    if (readLen > 0u)
        err = flRead(g_ctx.cfg.base + idx->dataOff, buf, readLen);

    if (err == NO_ERROR)
        *outSize = readLen;

    CFG_UNLOCK();
    return err;
}

error_t flashNewStmDeleteConfig(const char *path)
{
    error_t err;
    CFG_LOCK();
    err = deleteConfig_impl(path);
    CFG_UNLOCK();
    return err;
}

error_t flashNewStmCfgFreeSpace(uint32_t *freeBytes)
{
    if (!g_ctx.mounted || freeBytes == NULL) return ERROR_INVALID_PARAMETER;
    CFG_LOCK();
    *freeBytes = (g_ctx.cfg.writeOff < g_ctx.cfg.size)
                  ? (g_ctx.cfg.size - g_ctx.cfg.writeOff)
                  : 0u;
    CFG_UNLOCK();
    return NO_ERROR;
}

/* =========================================================================
 * LOG bölgesi — yüksek seviyeli işlevler
 * ====================================================================== */

/*
 * writeLog_impl — kilitsiz LOG kayıt yazıcı (LOG_LOCK dışarıda tutulmalı).
 *
 *   path    : NULL veya "" → anonim kayıt (pathLen=0)
 *   pathLen : path string uzunluğu (path NULL ise 0 geç)
 *   data    : yazılacak veri
 *   dataLen : veri boyutu
 *
 * Flash düzeni:
 *   [LogRecHdr 20B] [path align4(pathLen)] [veri align4(dataLen)]
 */
static error_t writeLog_impl(const char *path, uint32_t pathLen,
                              const char *data,  uint32_t dataLen)
{
    uint32_t totSize = (uint32_t)sizeof(LogRecHdr)
                     + align4(pathLen)
                     + align4(dataLen);

    /* Yer yoksa bölgeyi sil */
    if (g_ctx.log.writeOff + totSize > g_ctx.log.size)
    {
        error_t err = eraseAndReinitRegion(&g_ctx.log, REG_MAGIC_LOG);
        if (err != NO_ERROR) return err;
        g_ctx.log.seqNext = 1u;
    }

    uint32_t  base = g_ctx.log.base + g_ctx.log.writeOff;
    LogRecHdr lh;
    error_t   err;

    osMemset(&lh, 0xFF, sizeof(lh));
    lh.seq      = g_ctx.log.seqNext++;
    lh.dataSize = dataLen;
    lh.totSize  = totSize;
    lh.pathLen  = pathLen;

    /* 1. Başlığı magic hariç yaz */
    {
        uint32_t tmp[5];
        osMemcpy(tmp, &lh, sizeof(lh));
        tmp[0] = 0xFFFFFFFFu;
        err = flProgStruct(base, tmp, sizeof(lh));
        if (err != NO_ERROR) return err;
    }

    /* 2. Path yaz (varsa) */
    if (pathLen > 0u && path != NULL)
    {
        err = flProgPad4(base + (uint32_t)sizeof(LogRecHdr), path, pathLen);
        if (err != NO_ERROR) return err;
    }

    /* 3. Veri yaz */
    err = flProgPad4(base + (uint32_t)sizeof(LogRecHdr) + align4(pathLen),
                     data, dataLen);
    if (err != NO_ERROR) return err;

    /* 4. Magic yaz (en son — güç kesintisi güvenliği) */
    uint32_t magic = LOG_REC_MAGIC;
    err = flProgStruct(base, &magic, sizeof(magic));
    if (err != NO_ERROR) return err;

    g_ctx.log.writeOff += totSize;
    return flSync();
}

/* Genel API: anonim log yazma (AppLogRecorder iterator akışı için) */
error_t flashNewStmWriteLog(const char *text, uint32_t len)
{
    if (!g_ctx.mounted)            return ERROR_NOT_READY;
    if (text == NULL || len == 0u) return ERROR_INVALID_PARAMETER;

    LOG_LOCK();
    error_t err = writeLog_impl(NULL, 0u, text, len);
    LOG_UNLOCK();
    return err;
}

error_t flashNewStmIterateLogs(
        uint32_t startSeq,
        void (*cb)(uint32_t seq, const char *text, uint32_t len, void *ctx),
        void *ctx)
{
    if (!g_ctx.mounted || cb == NULL) return ERROR_INVALID_PARAMETER;

    LOG_LOCK();

    uint32_t off = sizeof(RegHdr);

    while (off + sizeof(LogRecHdr) <= g_ctx.log.size)
    {
        LogRecHdr lh;
        if (flRead(g_ctx.log.base + off, &lh, sizeof(lh)) != NO_ERROR)
            break;
        if (lh.magic == ERASED_U32)
            break;
        if (lh.magic != LOG_REC_MAGIC ||
            lh.totSize < sizeof(lh) ||
            off + lh.totSize > g_ctx.log.size)
            break;

        if (lh.seq >= startSeq)
        {
            /* Veriyi stack tampona oku (en fazla 256 byte'lık parçalar).
             * Path'li kayıtlarda veri, path'in hizalanmış boyutu kadar
             * ileride başlar. Anonim kayıtlarda (pathLen=0) hemen başlar. */
            uint32_t remaining = lh.dataSize;
            uint32_t dataOff   = off + (uint32_t)sizeof(LogRecHdr)
                                     + align4(lh.pathLen);
            char_t   chunk[256];

            while (remaining > 0u)
            {
                uint32_t n = (remaining > sizeof(chunk))
                              ? (uint32_t)sizeof(chunk) : remaining;
                if (flRead(g_ctx.log.base + dataOff, chunk, n) != NO_ERROR)
                    break;
                /* Callback mutex tutulurken çağrılıyor — callback içinde
                 * flashNewStmWriteLog çağırmak deadlock oluşturur! */
                cb(lh.seq, chunk, n, ctx);
                dataOff   += n;
                remaining -= n;
            }
        }

        off += lh.totSize;
    }

    LOG_UNLOCK();
    return NO_ERROR;
}

error_t flashNewStmClearLogs(void)
{
    if (!g_ctx.mounted) return ERROR_NOT_READY;
    LOG_LOCK();
    error_t err = eraseAndReinitRegion(&g_ctx.log, REG_MAGIC_LOG);
    if (err == NO_ERROR) g_ctx.log.seqNext = 1u;
    LOG_UNLOCK();
    return err;
}

/* =========================================================================
 * DATA bölgesi — yüksek seviyeli işlevler
 * ====================================================================== */

/*
 * writeData_impl — kilitsiz DATA kayıt yazıcı (DAT_LOCK dışarıda tutulmalı).
 *
 *   type      : kayıt tipi (FSNEW_DATA_SENSOR vb.); path'li kayıtlarda 0u
 *   path      : NULL veya "" → pathLen=0 (tip bazlı kayıt)
 *   pathLen   : path string uzunluğu
 *   data      : yazılacak veri
 *   dataSize  : veri boyutu
 *   timestamp : Unix zaman damgası (tip bazlı için); path'li kayıtlarda 0u
 *
 * Flash düzeni:
 *   [DatRecHdr 40B] [path align4(pathLen)] [veri align4(dataSize)]
 */
static error_t writeData_impl(uint32_t type,
                               const char *path,  uint32_t pathLen,
                               const void *data,  uint32_t dataSize,
                               uint32_t timestamp)
{
    uint32_t totSize = (uint32_t)sizeof(DatRecHdr)
                     + align4(pathLen)
                     + align4(dataSize);

    /* Yer yoksa bölgeyi sil */
    if (g_ctx.data.writeOff + totSize > g_ctx.data.size)
    {
        error_t err = eraseAndReinitRegion(&g_ctx.data, REG_MAGIC_DATA);
        if (err != NO_ERROR) return err;
        g_ctx.data.seqNext = 1u;
    }

    uint32_t  base = g_ctx.data.base + g_ctx.data.writeOff;
    DatRecHdr dh;
    error_t   err;

    uint32_t crc = crc32Buf((const uint8_t *)data, dataSize);

    osMemset(&dh, 0xFF, sizeof(dh));
    dh.type     = type;
    dh.seq      = g_ctx.data.seqNext++;
    dh.dataSize = dataSize;
    dh.crc32    = crc;
    dh.flags    = FLAGS_VALID;
    dh.ts       = timestamp;
    dh.totSize  = totSize;
    dh.pathLen  = pathLen;
    dh.reserved = 0u;

    /* 1. Başlığı magic hariç yaz */
    {
        uint32_t tmp[10];
        osMemcpy(tmp, &dh, sizeof(dh));
        tmp[0] = 0xFFFFFFFFu;
        err = flProgStruct(base, tmp, sizeof(dh));
        if (err != NO_ERROR) return err;
    }

    /* 2. Path yaz (varsa) */
    if (pathLen > 0u && path != NULL)
    {
        err = flProgPad4(base + (uint32_t)sizeof(DatRecHdr), path, pathLen);
        if (err != NO_ERROR) return err;
    }

    /* 3. Veri yaz */
    err = flProgPad4(base + (uint32_t)sizeof(DatRecHdr) + align4(pathLen),
                     data, dataSize);
    if (err != NO_ERROR) return err;

    /* 4. Magic yaz (en son — güç kesintisi güvenliği) */
    uint32_t magic = DAT_REC_MAGIC;
    err = flProgStruct(base, &magic, sizeof(magic));
    if (err != NO_ERROR) return err;

    g_ctx.data.writeOff += totSize;
    return flSync();
}

/* Genel API: tiplendirilmiş binary ölçüm kaydı (pathLen=0) */
error_t flashNewStmWriteData(uint32_t type,
                              const void *data, uint32_t size,
                              uint32_t timestamp)
{
    if (!g_ctx.mounted)             return ERROR_NOT_READY;
    if (data == NULL || size == 0u) return ERROR_INVALID_PARAMETER;

    DAT_LOCK();
    error_t err = writeData_impl(type, NULL, 0u, data, size, timestamp);
    DAT_UNLOCK();
    return err;
}

error_t flashNewStmIterateData(
        uint32_t type, uint32_t startSeq,
        void (*cb)(uint32_t seq, uint32_t ts, uint32_t type,
                   const void *data, uint32_t size, void *ctx),
        void *ctx)
{
    if (!g_ctx.mounted || cb == NULL) return ERROR_INVALID_PARAMETER;

    DAT_LOCK();

    uint32_t off = sizeof(RegHdr);
    uint8_t  buf[128];

    while (off + sizeof(DatRecHdr) <= g_ctx.data.size)
    {
        DatRecHdr dh;
        if (flRead(g_ctx.data.base + off, &dh, sizeof(dh)) != NO_ERROR)
            break;
        if (dh.magic == ERASED_U32)
            break;
        if (dh.magic != DAT_REC_MAGIC ||
            dh.totSize < sizeof(dh) ||
            off + dh.totSize > g_ctx.data.size)
            break;
        if (dh.flags != FLAGS_VALID)
        {
            off += dh.totSize;
            continue;
        }

        /* Path'li kayıtlar (pathLen>0) fsReadFile üzerinden erişilir;
         * tip bazlı iterator bu kayıtları atlar. */
        if (dh.pathLen > 0u)
        {
            off += dh.totSize;
            continue;
        }

        if (dh.seq >= startSeq && (type == 0u || dh.type == type))
        {
            /* pathLen==0 olduğundan veri hemen DatRecHdr'ın arkasında başlar */
            uint32_t payloadOff  = off + (uint32_t)sizeof(DatRecHdr);
            uint32_t remaining   = dh.dataSize;

            /* Küçük kayıtlar için tek seferlik callback */
            if (dh.dataSize <= sizeof(buf))
            {
                if (flRead(g_ctx.data.base + payloadOff,
                           buf, dh.dataSize) == NO_ERROR)
                {
                    /* CRC doğrula */
                    if (crc32Buf(buf, dh.dataSize) == dh.crc32)
                        cb(dh.seq, dh.ts, dh.type, buf, dh.dataSize, ctx);
                }
            }
            else
            {
                /* Büyük kayıtlar: CRC doğrulaması parçalı yapılamaz,
                 * önce CRC kontrol et, sonra callback çağır */
                uint32_t   crcAcc = 0xFFFFFFFFu;
                uint32_t   tmpOff = payloadOff;
                bool_t     crcOK  = TRUE;

                while (remaining > 0u)
                {
                    uint32_t n = (remaining > sizeof(buf))
                                  ? (uint32_t)sizeof(buf) : remaining;
                    if (flRead(g_ctx.data.base + tmpOff, buf, n) != NO_ERROR)
                    { crcOK = FALSE; break; }
                    crcAcc    = crc32Update(crcAcc, buf, n);
                    tmpOff   += n;
                    remaining -= n;
                }

                if (crcOK && (~crcAcc == dh.crc32))
                {
                    /* Callback'i tek parça veriyle çağıramıyoruz;
                     * uygulama ihtiyacına göre genişletilebilir */
                    cb(dh.seq, dh.ts, dh.type, NULL, dh.dataSize, ctx);
                }
            }
        }

        off += dh.totSize;
    }

    DAT_UNLOCK();
    return NO_ERROR;
}

error_t flashNewStmClearData(void)
{
    if (!g_ctx.mounted) return ERROR_NOT_READY;
    DAT_LOCK();
    error_t err = eraseAndReinitRegion(&g_ctx.data, REG_MAGIC_DATA);
    if (err == NO_ERROR) g_ctx.data.seqNext = 1u;
    DAT_UNLOCK();
    return err;
    return NO_ERROR;
}

/* =========================================================================
 * Tanılama
 * ====================================================================== */

error_t flashNewStmGetFreeSpace(uint32_t *cfgFree,
                                 uint32_t *logFree,
                                 uint32_t *dataFree)
{
    if (!g_ctx.mounted) return ERROR_NOT_READY;

    if (cfgFree != NULL)
    {
        CFG_LOCK();
        *cfgFree  = (g_ctx.cfg.writeOff < g_ctx.cfg.size)
                     ? g_ctx.cfg.size  - g_ctx.cfg.writeOff : 0u;
        CFG_UNLOCK();
    }
    if (logFree != NULL)
    {
        LOG_LOCK();
        *logFree  = (g_ctx.log.writeOff < g_ctx.log.size)
                     ? g_ctx.log.size  - g_ctx.log.writeOff : 0u;
        LOG_UNLOCK();
    }
    if (dataFree != NULL)
    {
        DAT_LOCK();
        *dataFree = (g_ctx.data.writeOff < g_ctx.data.size)
                     ? g_ctx.data.size - g_ctx.data.writeOff : 0u;
        DAT_UNLOCK();
    }

    return NO_ERROR;
}

/* =========================================================================
 * Standart fs_port API — yardımcılar
 * ====================================================================== */

static bool_t isLogPath(const char_t *path)
{
    /* Hem "/log/..." hem de "log/..." (başta slash olmayan) biçimlerini kabul et.
     * AppLogRecorder.c "log/ServiceName/date-hhmmss.log" şeklinde yol oluşturur.
     * meter/meterDataOut/ CONFIG bölgesine yönlendirilir: bu dosyalar yazıldıktan
     * sonra fsReadFile ile okunması gerekir (sendFilePacketised). LOG bölgesinin
     * LogRecHdr'ında path alanı olmadığından kayıtlar path'e göre bulunamaz ve
     * fsReadFile zaten LOG tipini desteklemez; CONFIG bu pattern'i tam karşılar. */
    return (osStrncmp(path, "/log/",  5) == 0 ||
            osStrncmp(path, "/logs/", 6) == 0 ||
            osStrncmp(path, "log/",   4) == 0 ||
            osStrncmp(path, "logs/",  5) == 0) ? TRUE : FALSE;
}

static bool_t isDataPath(const char_t *path)
{
    /* "data/" ve "/data/" yolları: tiplendirilmiş binary ölçüm kayıtları.
     * "meter/meterDataOut/": sayaç okuma transkripsiyon dosyaları.
     * Her iki grup da DATA bölgesine yönlendirilir; yalnızca pathLen alanı
     * farklıdır (ölçümler=0, dosyalar>0). fsReadFile ve flashNewStmIterateData
     * bu ayrımı otomatik olarak değerlendirir. */
    return (osStrncmp(path, "/data/",             6)  == 0 ||
            osStrncmp(path, "data/",              5)  == 0 ||
            osStrncmp(path, "meter/meterDataOut/", 19) == 0) ? TRUE : FALSE;
}

static FsHandle *findFreeHandle(void)
{
    uint32_t i;
    for (i = 0u; i < FSNEW_MAX_OPEN_FILES; i++)
    {
        if (!g_ctx.handles[i].inUse)
            return &g_ctx.handles[i];
    }
    return NULL;
}

static void setUnknownDateTime(DateTime *dt)
{
    if (dt == NULL) return;
    dt->year = 1970; dt->month = 1; dt->day = 1;
    dt->dayOfWeek = 0;
    dt->hours = 0; dt->minutes = 0; dt->seconds = 0;
    dt->milliseconds = 0;
}

/* =========================================================================
 * Standart fs_port API — implementasyon
 * ====================================================================== */

bool_t fsFileExists(const char_t *path)
{
    if (path == NULL || !g_ctx.mounted) return FALSE;

    /* DATA bölgesi: path'li kayıt var mı tara */
    if (isDataPath(path))
    {
        uint32_t plen = (uint32_t)osStrlen(path);
        DAT_LOCK();
        uint32_t off   = sizeof(RegHdr);
        bool_t   found = FALSE;
        while (off + sizeof(DatRecHdr) <= g_ctx.data.size && !found)
        {
            DatRecHdr dh;
            if (flRead(g_ctx.data.base + off, &dh, sizeof(dh)) != NO_ERROR) break;
            if (dh.magic == ERASED_U32) break;
            if (dh.magic != DAT_REC_MAGIC ||
                dh.totSize < sizeof(dh) ||
                off + dh.totSize > g_ctx.data.size) break;
            if (dh.pathLen == plen && dh.pathLen > 0u && dh.flags == FLAGS_VALID)
            {
                char_t recPath[FSNEW_PATH_MAX + 1u];
                if (flRead(g_ctx.data.base + off + sizeof(DatRecHdr),
                           recPath, plen) == NO_ERROR)
                {
                    recPath[plen] = '\0';
                    if (osStrncmp(recPath, path, FSNEW_PATH_MAX) == 0)
                        found = TRUE;
                }
            }
            off += dh.totSize;
        }
        DAT_UNLOCK();
        return found;
    }

    if (isLogPath(path)) return FALSE;  /* LOG: path bazlı varlık kontrolü yok */

    CFG_LOCK();
    bool_t found = (cfgIdxFind(path) >= 0) ? TRUE : FALSE;
    CFG_UNLOCK();
    return found;
}

error_t fsGetFileSize(const char_t *path, uint32_t *size)
{
    if (path == NULL || size == NULL || !g_ctx.mounted)
        return ERROR_INVALID_PARAMETER;

    CFG_LOCK();
    int32_t slot = cfgIdxFind(path);
    if (slot < 0) { CFG_UNLOCK(); return ERROR_FILE_NOT_FOUND; }
    *size = g_ctx.cfgIdx[slot].dataSize;
    CFG_UNLOCK();
    return NO_ERROR;
}

error_t fsGetFileStat(const char_t *path, FsFileStat *fileStat)
{
    if (path == NULL || fileStat == NULL || !g_ctx.mounted)
        return ERROR_INVALID_PARAMETER;

    osMemset(fileStat, 0, sizeof(FsFileStat));

    /* Kök dizin — mutex gerektirmez */
    if (osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
    {
        fileStat->attributes = FS_FILE_ATTR_DIRECTORY;
        setUnknownDateTime(&fileStat->modified);
        return NO_ERROR;
    }

    CFG_LOCK();
    int32_t slot = cfgIdxFind(path);
    if (slot < 0) { CFG_UNLOCK(); return ERROR_FILE_NOT_FOUND; }

    fileStat->attributes = 0u;
    fileStat->size       = g_ctx.cfgIdx[slot].dataSize;
    CFG_UNLOCK();

    setUnknownDateTime(&fileStat->modified);
    return NO_ERROR;
}

error_t fsRenameFile(const char_t *oldPath, const char_t *newPath)
{
    /* İki aşama tek kilit altında: yeni isimle yaz + eskiyi sil.
     * Kilit tutulurken _impl versiyonlarını çağırırız — deadlock olmaz. */
    if (!g_ctx.mounted) return ERROR_NOT_READY;
    if (!PATH_VALID(oldPath) || !PATH_VALID(newPath))
        return ERROR_INVALID_PARAMETER;

    CFG_LOCK();

    int32_t oldSlot = cfgIdxFind(oldPath);
    if (oldSlot < 0) { CFG_UNLOCK(); return ERROR_FILE_NOT_FOUND; }
    if (cfgIdxFind(newPath) >= 0) { CFG_UNLOCK(); return ERROR_ALREADY_EXISTS; }

    CfgIdx  *idx  = &g_ctx.cfgIdx[oldSlot];
    uint32_t sz   = idx->dataSize;
    uint8_t *buf  = NULL;
    error_t  err  = NO_ERROR;

    if (sz > 0u)
    {
        buf = (uint8_t *)osAllocMem(sz);
        if (buf == NULL) { CFG_UNLOCK(); return ERROR_OUT_OF_MEMORY; }
        if (flRead(g_ctx.cfg.base + idx->dataOff, buf, sz) != NO_ERROR)
        {
            osFreeMem(buf);
            CFG_UNLOCK();
            return ERROR_READ_FAILED;
        }
    }

    err = writeConfig_impl(newPath, buf, sz);
    if (err == NO_ERROR)
        err = deleteConfig_impl(oldPath);

    if (buf != NULL) osFreeMem(buf);
    CFG_UNLOCK();
    return err;
}

error_t fsDeleteFile(const char_t *path)
{
    if (path == NULL) return ERROR_INVALID_PARAMETER;

    /* DATA bölgesi: path eşleşen kayıtların flags alanını geçersiz yap */
    if (isDataPath(path))
    {
        DAT_LOCK();
        uint32_t off     = sizeof(RegHdr);
        uint32_t plen    = (uint32_t)osStrlen(path);
        bool_t   deleted = FALSE;

        while (off + sizeof(DatRecHdr) <= g_ctx.data.size)
        {
            DatRecHdr dh;
            if (flRead(g_ctx.data.base + off, &dh, sizeof(dh)) != NO_ERROR) break;
            if (dh.magic == ERASED_U32) break;
            if (dh.magic != DAT_REC_MAGIC ||
                dh.totSize < sizeof(dh) ||
                off + dh.totSize > g_ctx.data.size) break;

            if (dh.pathLen == plen && dh.pathLen > 0u &&
                dh.flags == FLAGS_VALID)
            {
                char_t recPath[FSNEW_PATH_MAX + 1u];
                if (flRead(g_ctx.data.base + off + sizeof(DatRecHdr),
                           recPath, plen) == NO_ERROR)
                {
                    recPath[plen] = '\0';
                    if (osStrncmp(recPath, path, FSNEW_PATH_MAX) == 0)
                    {
                        /* flags alanını geçersiz yap (sector erase gerektirmez) */
                        uint32_t flagsOff = off + offsetof(DatRecHdr, flags);
                        uint32_t inv = 0x00000000u;
                        (void)flProgStruct(g_ctx.data.base + flagsOff,
                                           &inv, sizeof(inv));
                        deleted = TRUE;
                    }
                }
            }
            off += dh.totSize;
        }

        DAT_UNLOCK();
        return deleted ? NO_ERROR : ERROR_FILE_NOT_FOUND;
    }

    /* LOG bölgesi: silme desteklenmiyor (append-only circular) */
    if (isLogPath(path))
        return ERROR_INVALID_TYPE;

    /* CONFIG bölgesi */
    error_t err;
    CFG_LOCK();
    err = deleteConfig_impl(path);
    CFG_UNLOCK();
    return err;
}

FsFile *fsOpenFile(const char_t *path, uint_t mode)
{
    if (path == NULL || !g_ctx.mounted) return NULL;
    if (osStrlen(path) == 0u || osStrlen(path) > FSNEW_PATH_MAX) return NULL;

    /* Hangi bölge? */
    FhType ftype;
    if      (isLogPath(path))  ftype = FH_LOG;
    else if (isDataPath(path)) ftype = FH_DATA;
    else                       ftype = FH_CONFIG;

    /* CONFIG için kilidi al (handle tablosu + cfgIdx erişimi) */
    if (ftype == FH_CONFIG) CFG_LOCK();

    FsHandle *h = findFreeHandle();
    if (h == NULL)
    {
        if (ftype == FH_CONFIG) CFG_UNLOCK();
        return NULL;
    }

    osMemset(h, 0, sizeof(FsHandle));
    strSafeCopy(h->path, path, FSNEW_PATH_MAX);
    h->ftype     = ftype;
    h->readMode  = ((mode & (uint_t)FS_FILE_MODE_READ)  != 0u) ? TRUE : FALSE;
    h->writeMode = ((mode & (uint_t)FS_FILE_MODE_WRITE) != 0u) ? TRUE : FALSE;

    /* ── CONFIG okuma: flash konumunu bul ─────────────────────────────── */
    if (h->readMode && ftype == FH_CONFIG)
    {
        int32_t slot = cfgIdxFind(path);
        if (slot < 0 && (mode & (uint_t)FS_FILE_MODE_CREATE) == 0u)
        {
            CFG_UNLOCK();
            return NULL;
        }
        if (slot >= 0)
        {
            h->rAbsBase = g_ctx.cfg.base + g_ctx.cfgIdx[slot].dataOff;
            h->rSize    = g_ctx.cfgIdx[slot].dataSize;
        }
    }

    /* ── CONFIG yazma: mevcut içeriği tampona al (TRUNC değilse) ─────── */
    if (h->writeMode && ftype == FH_CONFIG)
    {
        int32_t slot = cfgIdxFind(path);
        if (slot >= 0 && (mode & (uint_t)FS_FILE_MODE_TRUNC) == 0u)
        {
            uint32_t existSz = g_ctx.cfgIdx[slot].dataSize;
            if (existSz > 0u)
            {
                h->wBuf   = (uint8_t *)osAllocMem(existSz + 256u);
                h->wAlloc = existSz + 256u;
                if (h->wBuf == NULL)
                {
                    CFG_UNLOCK();
                    return NULL;
                }
                if (flRead(g_ctx.cfg.base + g_ctx.cfgIdx[slot].dataOff,
                           h->wBuf, existSz) != NO_ERROR)
                {
                    osFreeMem(h->wBuf);
                    CFG_UNLOCK();
                    return NULL;
                }
                h->wLen = existSz;
            }
        }
    }

    /* ── LOG / DATA okuma: heap yok — lazy scan başlangıç konumunu ayarla ─
     * Gerçek tarama fsReadFile içinde talep geldiğinde yapılır.
     * Flash'tan doğrudan okuma; büyük dosyalarda RAM sorun olmaz.          */
    if (h->readMode && (ftype == FH_LOG || ftype == FH_DATA))
    {
        h->rRegOff     = (uint32_t)sizeof(RegHdr);  /* bölge başından ilk kayıt */
        h->rRecDataOff = 0u;
        h->rRecRemain  = 0u;                         /* ilk okumada scan başlar */
    }

    h->inUse = TRUE;
    if (ftype == FH_CONFIG) CFG_UNLOCK();
    return (FsFile *)h;
}

error_t fsSeekFile(FsFile *file, int_t offset, uint_t origin)
{
    FsHandle *h;
    int32_t   base;
    int32_t   newPos;

    if (file == NULL || !g_ctx.mounted) return ERROR_INVALID_PARAMETER;
    h = (FsHandle *)file;
    if (h->ftype != FH_CONFIG) return ERROR_NOT_IMPLEMENTED;

    CFG_LOCK();

    /* ── Yazma modu: FS_SEEK_END + offset=0 → no-op (tampon zaten sonda) */
    if (h->writeMode)
    {
        error_t ret = (origin == (uint_t)FS_SEEK_END && offset == 0)
                       ? NO_ERROR : ERROR_NOT_IMPLEMENTED;
        CFG_UNLOCK();
        return ret;
    }

    /* ── Okuma modu ──────────────────────────────────────────────────── */
    if (!h->readMode) { CFG_UNLOCK(); return ERROR_NOT_IMPLEMENTED; }

    if (origin == (uint_t)FS_SEEK_CUR)
        base = (int32_t)h->rPos;
    else if (origin == (uint_t)FS_SEEK_END)
        base = (int32_t)h->rSize;
    else
        base = 0;

    newPos = base + offset;
    if (newPos < 0 || (uint32_t)newPos > h->rSize)
    {
        CFG_UNLOCK();
        return ERROR_INVALID_PARAMETER;
    }

    h->rPos = (uint32_t)newPos;
    CFG_UNLOCK();
    return NO_ERROR;
}

error_t fsWriteFile(FsFile *file, void *data, size_t length)
{
    FsHandle *h;

    if (file == NULL || data == NULL || !g_ctx.mounted)
        return ERROR_INVALID_PARAMETER;

    h = (FsHandle *)file;
    if (!h->writeMode) return ERROR_ACCESS_DENIED;

    if (h->ftype == FH_LOG)
    {
        /* Her fsWriteFile çağrısı path bilgisini de içeren bir LOG kaydı yazar.
         * Bu sayede aynı path ile fsOpenFile(READ) yapıldığında tüm kayıtlar
         * bulunup birleştirilebilir (fsReadFile desteği). */
        uint32_t plen = (uint32_t)osStrlen(h->path);
        LOG_LOCK();
        error_t err = writeLog_impl(h->path, plen, (const char *)data,
                                    (uint32_t)length);
        LOG_UNLOCK();
        return err;
    }

    if (h->ftype == FH_DATA)
    {
        /* Her fsWriteFile çağrısı path bilgisini de içeren bir DATA kaydı yazar.
         * type=0, ts=0 ile işaretlenir; flashNewStmIterateData bu kayıtları atlar.
         * fsOpenFile(READ) ile aynı path'e sahip tüm kayıtlar toplanıp okunur. */
        uint32_t plen = (uint32_t)osStrlen(h->path);
        DAT_LOCK();
        error_t err = writeData_impl(0u, h->path, plen,
                                     data, (uint32_t)length, 0u);
        DAT_UNLOCK();
        return err;
    }

    /* CONFIG: tampona ekle (handle'a ait bellek, kilit gerekli) */
    CFG_LOCK();

    if (h->wLen + (uint32_t)length > FSNEW_CFG_MAX_PAYLOAD)
    {
        CFG_UNLOCK();
        return ERROR_OUT_OF_RESOURCES;
    }

    if (h->wLen + (uint32_t)length > h->wAlloc)
    {
        uint32_t newAlloc = h->wAlloc + (uint32_t)length + 256u;
        uint8_t *newBuf   = (uint8_t *)osAllocMem(newAlloc);
        if (newBuf == NULL) { CFG_UNLOCK(); return ERROR_OUT_OF_MEMORY; }

        if (h->wBuf != NULL && h->wLen > 0u)
            osMemcpy(newBuf, h->wBuf, h->wLen);
        if (h->wBuf != NULL)
            osFreeMem(h->wBuf);

        h->wBuf   = newBuf;
        h->wAlloc = newAlloc;
    }

    osMemcpy(h->wBuf + h->wLen, data, length);
    h->wLen += (uint32_t)length;

    CFG_UNLOCK();
    return NO_ERROR;
}

error_t fsReadFile(FsFile *file, void *data, size_t size, size_t *length)
{
    FsHandle *h;
    uint32_t  remain;
    uint32_t  n;

    if (file == NULL || data == NULL || length == NULL || !g_ctx.mounted)
        return ERROR_INVALID_PARAMETER;

    h = (FsHandle *)file;
    if (!h->readMode) return ERROR_ACCESS_DENIED;

    /* ── LOG / DATA: heap yok, doğrudan flash'tan lazy scan ────────────
     *
     * Her fsReadFile çağrısında:
     *   1. rRecRemain > 0  → mevcut kaydın kalan kısmından oku (flash)
     *   2. rRecRemain == 0 → rRegOff'tan itibaren sonraki eşleşen kaydı tara
     *   3. Tek çağrıda birden fazla kayıt tüketilebilir (tampon dolana kadar)
     *   4. Eşleşen kayıt kalmadıysa ERROR_END_OF_FILE döner              */
    if (h->ftype == FH_LOG || h->ftype == FH_DATA)
    {
        LOG_LOCK();   /* tek mutex (flashMux) hem LOG hem DATA'yı korur */
        *length = 0u;

        uint8_t *dst     = (uint8_t *)data;
        uint32_t want    = (uint32_t)size;
        uint32_t filled  = 0u;
        error_t  err     = NO_ERROR;

        while (filled < want)
        {
            /* Mevcut kayıtta veri kalmadıysa bir sonraki eşleşeni bul */
            if (h->rRecRemain == 0u)
            {
                bool_t found = FALSE;

                if (h->ftype == FH_LOG)
                {
                    while (h->rRegOff + sizeof(LogRecHdr) <= g_ctx.log.size)
                    {
                        LogRecHdr lh;
                        uint32_t  cur = h->rRegOff;
                        if (flRead(g_ctx.log.base + cur, &lh, sizeof(lh)) != NO_ERROR) break;
                        if (lh.magic == ERASED_U32) break;
                        if (lh.magic != LOG_REC_MAGIC ||
                            lh.totSize < sizeof(lh) ||
                            cur + lh.totSize > g_ctx.log.size) break;

                        h->rRegOff += lh.totSize; /* bir sonraki kayda geç */

                        if (lh.pathLen > 0u && lh.pathLen <= FSNEW_PATH_MAX)
                        {
                            char_t recPath[FSNEW_PATH_MAX + 1u];
                            if (flRead(g_ctx.log.base + cur + sizeof(LogRecHdr),
                                       recPath, lh.pathLen) == NO_ERROR)
                            {
                                recPath[lh.pathLen] = '\0';
                                if (osStrncmp(recPath, h->path, FSNEW_PATH_MAX) == 0)
                                {
                                    h->rRecDataOff = g_ctx.log.base + cur
                                                   + sizeof(LogRecHdr)
                                                   + align4(lh.pathLen);
                                    h->rRecRemain  = lh.dataSize;
                                    found = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                }
                else /* FH_DATA */
                {
                    while (h->rRegOff + sizeof(DatRecHdr) <= g_ctx.data.size)
                    {
                        DatRecHdr dh;
                        uint32_t  cur = h->rRegOff;
                        if (flRead(g_ctx.data.base + cur, &dh, sizeof(dh)) != NO_ERROR) break;
                        if (dh.magic == ERASED_U32) break;
                        if (dh.magic != DAT_REC_MAGIC ||
                            dh.totSize < sizeof(dh) ||
                            cur + dh.totSize > g_ctx.data.size) break;

                        h->rRegOff += dh.totSize;

                        if (dh.pathLen > 0u && dh.pathLen <= FSNEW_PATH_MAX &&
                            dh.flags == FLAGS_VALID)
                        {
                            char_t recPath[FSNEW_PATH_MAX + 1u];
                            if (flRead(g_ctx.data.base + cur + sizeof(DatRecHdr),
                                       recPath, dh.pathLen) == NO_ERROR)
                            {
                                recPath[dh.pathLen] = '\0';
                                if (osStrncmp(recPath, h->path, FSNEW_PATH_MAX) == 0)
                                {
                                    h->rRecDataOff = g_ctx.data.base + cur
                                                   + sizeof(DatRecHdr)
                                                   + align4(dh.pathLen);
                                    h->rRecRemain  = dh.dataSize;
                                    found = TRUE;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (!found) break;   /* başka kayıt yok → döngüden çık */
            }

            /* Mevcut kayıttan oku (flash → doğrudan kullanıcı tamponuna) */
            n   = (h->rRecRemain < (want - filled)) ? h->rRecRemain : (want - filled);
            err = flRead(h->rRecDataOff, dst + filled, n);
            if (err != NO_ERROR) break;

            h->rRecDataOff += n;
            h->rRecRemain  -= n;
            filled         += n;
        }

        *length = (size_t)filled;
        LOG_UNLOCK();

        if (filled == 0u && err == NO_ERROR)
            return ERROR_END_OF_FILE;
        return err;
    }

    /* ── CONFIG: doğrudan flash'tan oku ─────────────────────────────── */
    CFG_LOCK();

    *length = 0u;
    if (h->rPos >= h->rSize) { CFG_UNLOCK(); return ERROR_END_OF_FILE; }

    remain = h->rSize - h->rPos;
    n      = (remain < (uint32_t)size) ? remain : (uint32_t)size;

    error_t err = flRead(h->rAbsBase + h->rPos, data, n);
    if (err == NO_ERROR)
    {
        h->rPos += n;
        *length  = (size_t)n;
    }

    CFG_UNLOCK();
    return err;
}

void fsCloseFile(FsFile *file)
{
    FsHandle *h;

    if (file == NULL || !g_ctx.mounted) return;
    h = (FsHandle *)file;

    if (h->ftype == FH_CONFIG)
    {
        CFG_LOCK();

        /* CONFIG yazma modunda: tamponu flash'a kaydet (_impl — kilit zaten açık) */
        if (h->writeMode && h->wLen > 0u)
            (void)writeConfig_impl(h->path, h->wBuf, h->wLen);

        if (h->wBuf != NULL)
        {
            osFreeMem(h->wBuf);
            h->wBuf = NULL;
        }

        osMemset(h, 0, sizeof(FsHandle));
        CFG_UNLOCK();
    }
    else
    {
        /* LOG / DATA:
         *   - Okuma: heap ayrılmadı (lazy scan), temizlenecek bir şey yok.
         *   - Yazma: wBuf CONFIG için kullanılır, LOG/DATA için NULL'dur. */
        LOG_LOCK();
        if (h->wBuf != NULL)
        {
            osFreeMem(h->wBuf);
            h->wBuf = NULL;
        }
        osMemset(h, 0, sizeof(FsHandle));
        LOG_UNLOCK();
    }
}

/* ─── Dizin işlemleri ─────────────────────────────────────────────────── */

bool_t fsDirExists(const char_t *path)
{
    uint32_t i;

    if (!g_ctx.mounted || path == NULL) return FALSE;
    if (osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0) return TRUE;

    CFG_LOCK();
    bool_t found = FALSE;
    for (i = 0u; i < FSNEW_CFG_INDEX_SIZE; i++)
    {
        if (g_ctx.cfgIdx[i].valid &&
            osStrncmp(g_ctx.cfgIdx[i].path, path, osStrlen(path)) == 0)
        {
            found = TRUE;
            break;
        }
    }
    CFG_UNLOCK();
    return found;
}

error_t fsCreateDir(const char_t *path)
{
    if (path == NULL) return ERROR_INVALID_PARAMETER;
    if (osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0)
        return NO_ERROR;
    return ERROR_NOT_IMPLEMENTED;
}

error_t fsRemoveDir(const char_t *path)
{
    if (path == NULL)                                          return ERROR_INVALID_PARAMETER;
    if (osStrcmp(path, "/") == 0 || osStrcmp(path, "") == 0) return ERROR_ACCESS_DENIED;
    return ERROR_NOT_IMPLEMENTED;
}

FsDir *fsOpenDir(const char_t *path)
{
    DirHandle *d;

    if (!g_ctx.mounted || path == NULL) return NULL;

    d = (DirHandle *)osAllocMem(sizeof(DirHandle));
    if (d == NULL) return NULL;
    osMemset(d, 0, sizeof(DirHandle));

    if (osStrcmp(path, "") == 0 || osStrcmp(path, "/") == 0)
    {
        d->isRoot     = TRUE;
        d->prefix[0]  = '\0';
    }
    else
    {
        d->isRoot = FALSE;
        strSafeCopy(d->prefix, path, FSNEW_PATH_MAX);
    }

    return (FsDir *)d;
}

error_t fsReadDir(FsDir *dir, FsDirEntry *dirEntry)
{
    DirHandle *d;
    uint32_t   i;

    if (!g_ctx.mounted || dir == NULL || dirEntry == NULL)
        return ERROR_INVALID_PARAMETER;

    d = (DirHandle *)dir;
    osMemset(dirEntry, 0, sizeof(FsDirEntry));

    CFG_LOCK();

    for (i = d->cursor; i < FSNEW_CFG_INDEX_SIZE; i++)
    {
        if (!g_ctx.cfgIdx[i].valid) continue;

        if (d->isRoot ||
            osStrncmp(g_ctx.cfgIdx[i].path,
                      d->prefix, osStrlen(d->prefix)) == 0)
        {
            dirEntry->attributes = 0u;
            dirEntry->size       = g_ctx.cfgIdx[i].dataSize;
            setUnknownDateTime(&dirEntry->modified);
            strSafeCopy(dirEntry->name, g_ctx.cfgIdx[i].path, FS_MAX_NAME_LEN);
            d->cursor = i + 1u;
            CFG_UNLOCK();
            return NO_ERROR;
        }
    }

    CFG_UNLOCK();
    return ERROR_END_OF_STREAM;
}

void fsCloseDir(FsDir *dir)
{
    if (dir != NULL)
        osFreeMem(dir);
}

/******************************** End Of File ********************************/
