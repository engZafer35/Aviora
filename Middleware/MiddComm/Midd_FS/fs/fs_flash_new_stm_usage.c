/**
 * @file  fs_flash_new_stm_usage.c
 * @brief fs_flash_new_stm kullanım örnekleri
 *
 * Bu dosya derlemeye dahil EDİLMEZ; yalnızca referans amaçlıdır.
 * Her bölüm bağımsız çağrılabilir; hata kontrolü gerçek uygulamada
 * şart olduğundan örneklerde de eksiksiz gösterilmiştir.
 *
 * İçerik:
 *   1. Sistem başlatma (bir kez yapılır)
 *   2. Sayaç (Meter) — ekleme, sorgulama, silme
 *   3. Direktif (Directive) — JSON ekleme, okuma, silme
 *   4. Log — fsWriteFile yazma, fsReadFile okuma, iterator, temizleme
 *   5. Konfigürasyon (Config) — fsWriteFile/fsReadFile, güncelleme, silme
 *   6. Sensor/Sayaç ölçüm verisi (Data):
 *        6a-c: flashNewStmWriteData + flashNewStmIterateData (tip bazlı)
 *        6d-f: fsWriteFile + fsReadFile + fsDeleteFile (path'li dosya)
 */

#include "fs_flash_new_stm.h"
#include "McuInternalFlashDrv_Stm32f407.h"
#include "AppMeterOperations.h"  /* MeterData_t, MeterTable_t */

#include <stdio.h>
#include <string.h>

/* =========================================================================
 * Yardımcı: hata kodu kontrolü
 * ====================================================================== */
#define CHECK(expr)                                         \
    do {                                                    \
        error_t _e = (expr);                                \
        if (_e != NO_ERROR) {                               \
            printf("[ERR] %s:%d → %d\n\r",                   \
                   __FILE__, __LINE__, (int)_e);            \
            return _e;                                      \
        }                                                   \
    } while (0)

/* =========================================================================
 * 1. SİSTEM BAŞLATMA
 *    flashNewStmConfigure → (ilk kullanımda) flashNewStmFormat → fsInit
 *    Bu sıra her açılışta tekrarlanır; Format yalnız ilk kez çağrılır.
 * ====================================================================== */

static error_t system_init(void)
{
    /* Donanım callback'leri ve flash geometrisi */
    static const FlashNewStmGeom geom = {
        .ops = {
            /* STM32F407 iç-flash sürücüsünden gelen fonksiyonlar */
            .read  = (error_t (*)(uint32_t, void *, size_t))
                     internalStm32f407FlashRead,
            .prog  = (error_t (*)(uint32_t, const void *, size_t))
                     internalStm32f407FlashProg,
            .erase = (error_t (*)(uint32_t))
                     internalStm32f407FlashErase,
            .sync  = (error_t (*)(void))
                     internalStm32f407FlashSync,
        },
        /* Sector 9  → CONFIG   (128 KB) */
        .cfgBase    = FSNEW_DEFAULT_CFG_BASE,
        .cfgSize    = FSNEW_DEFAULT_CFG_SIZE,
        /* Sector 10 → LOG      (128 KB) */
        .logBase    = FSNEW_DEFAULT_LOG_BASE,
        .logSize    = FSNEW_DEFAULT_LOG_SIZE,
        /* Sector 11 → DATA     (128 KB) */
        .dataBase   = FSNEW_DEFAULT_DATA_BASE,
        .dataSize   = FSNEW_DEFAULT_DATA_SIZE,
        .sectorSize = FSNEW_DEFAULT_SECTOR_SZ,
        .progMin    = FSNEW_DEFAULT_PROG_MIN,
    };

    /* Yapılandır (mutex'leri de oluşturur) */
    CHECK(flashNewStmConfigure(&geom));

    /*
     * İLK KULLANIM: Flash hiç formatlanmamışsa aşağıdaki satırı aktif et.
     * Dikkat: tüm kayıtlar silinir!
     *
     *   CHECK(flashNewStmFormat());
     *
     * Normalde bunu bir NVM flag'i ile kontrol edersin:
     *   if (ilkAcilis) { CHECK(flashNewStmFormat()); }
     */

    /* Her açılışta: mount et, RAM indeksini yeniden oluştur */
    error_t er = fsInit();

    printf("[OK] FS başlatıldı\n\r");
    return er;
}

/* =========================================================================
 * 2. SAYAÇ (METER) İŞLEMLERİ
 *
 * AppMeterOperations zaten yüksek seviyeli API'yi sağlıyor.
 * Aşağıda hem AppMeterOperations API'si hem de doğrudan FS API gösteriliyor.
 * ====================================================================== */

/* ── 2a. Sayaç Ekleme (AppMeterOperations API) ─────────────────────────── */
static error_t meter_ekle_yuksek_seviye(char *snumber)
{
    MeterData_t sayac = {0};

    strncpy(sayac.protocol,       "IEC62056",  sizeof(sayac.protocol)  - 1);
    strncpy(sayac.type,           "electricity",sizeof(sayac.type)     - 1);
    strncpy(sayac.brand,          "MKL",        sizeof(sayac.brand)    - 1);
    strncpy(sayac.customerNumber, "C001",       sizeof(sayac.customerNumber) - 1);
    strncpy(sayac.serialNumber,   snumber, sizeof(sayac.serialNumber)   - 1);
    strncpy(sayac.serialPort,     "rs485-1",    sizeof(sayac.serialPort)     - 1);
    sayac.initBaud = 300;
    sayac.fixBaud  = FALSE;
    strncpy(sayac.frame, "7E1", sizeof(sayac.frame) - 1);

    RETURN_STATUS st = appMeterOperationsAddMeter(&sayac);
    if (st != SUCCESS)
    {
        printf("[ERR] Sayaç eklenemedi (zaten var ya da alan yetersiz)\n\r");
        return ERROR_FAILURE;
    }

    printf("[OK] Sayaç eklendi: %s\n\r", sayac.serialNumber);

    /* Sonuç:
     *   CONFIG bölgesinde iki kayıt oluşur:
     *   1) "meter/reg/SN12345678"  → MeterData_t blobu
     *   2) "meter/meterList"       → [SN12345678] (binary, 16B/satır) */

    return NO_ERROR;
}

/* ── 2b. Sayaç Ekleme (doğrudan FS API — altta ne olduğunu gösterir) ─── */
static error_t meter_ekle_dusuk_seviye(void)
{
    const char *seriNo = "SN87654321";

    /* 1. Veri dosyasını kaydet */
    MeterData_t sayac = {0};
    strncpy(sayac.protocol,     "IEC62056",   sizeof(sayac.protocol)  - 1);
    strncpy(sayac.type,         "water",       sizeof(sayac.type)      - 1);
    strncpy(sayac.brand,        "BYL",         sizeof(sayac.brand)     - 1);
    strncpy(sayac.serialNumber, seriNo,        sizeof(sayac.serialNumber) - 1);
    strncpy(sayac.serialPort,   "rs485-2",     sizeof(sayac.serialPort)   - 1);
    sayac.initBaud = 9600;
    sayac.fixBaud  = TRUE;
    strncpy(sayac.frame, "8N1", sizeof(sayac.frame) - 1);

    char regPath[48];
    snprintf(regPath, sizeof(regPath), "meter/reg/%s", seriNo);

    CHECK(flashNewStmWriteConfig(regPath, &sayac, sizeof(sayac)));

    /* 2. Listeye ekle: önce mevcut listeyi oku, yeni satırı ekle, geri yaz */
    const char *listPath = "meter/meterList";
    uint32_t   mevSize   = 0;
    (void)fsGetFileSize(listPath, &mevSize);  /* dosya yoksa 0 döner */

    uint32_t rowCount = mevSize / (uint32_t)sizeof(MeterTable_t);

    /* Tüm listeyi + yeni satırı RAM'de hazırla */
    uint32_t newSize = (rowCount + 1u) * (uint32_t)sizeof(MeterTable_t);
    uint8_t  listBuf[64 * sizeof(MeterTable_t)];  /* maks 64 sayaç */

    if (newSize > sizeof(listBuf)) return ERROR_OUT_OF_RESOURCES;

    if (mevSize > 0u)
        CHECK(flashNewStmReadConfig(listPath, listBuf, mevSize, &mevSize));

    MeterTable_t *yeniSatir = (MeterTable_t *)(listBuf + rowCount * sizeof(MeterTable_t));
    memset(yeniSatir, 0, sizeof(MeterTable_t));
    strncpy(yeniSatir->serialNumber, seriNo, sizeof(yeniSatir->serialNumber) - 1);

    CHECK(flashNewStmWriteConfig(listPath, listBuf, newSize));

    printf("[OK] Sayaç eklendi (düşük seviye): %s\n\r", seriNo);
    return NO_ERROR;
}

/* ── 2c. Sayaç Sorgulama ─────────────────────────────────────────────── */
static error_t meter_oku(const char *seriNo)
{
    /* AppMeterOperations üzerinden */
    MeterData_t sayac;
    if (appMeterOperationsGetMeterData(seriNo, &sayac) != SUCCESS)
    {
        printf("[ERR] Sayaç bulunamadı: %s\n\r", seriNo);
        return ERROR_FILE_NOT_FOUND;
    }

    printf("[OK] Sayaç okundu: %s | protokol=%s | port=%s | baud=%d\n\r",
           sayac.serialNumber, sayac.protocol,
           sayac.serialPort, sayac.initBaud);

    return NO_ERROR;
}

/* ── 2d. Sayaç Silme ─────────────────────────────────────────────────── */
static error_t meter_sil(const char *seriNo)
{
    /* AppMeterOperations API'si: listeyi günceller + kayıt dosyasını siler */
    RETURN_STATUS st = appMeterOperationsDeleteMeter(seriNo);
    if (st != SUCCESS)
    {
        printf("[ERR] Sayaç silinemedi: %s\n\r", seriNo);
        return ERROR_FAILURE;
    }

    /* CONFIG bölgesinde ne oldu:
     *   "meter/reg/SN12345678" → flags = 0x00000000 (erase YOK)
     *   "meter/meterList"      → yeni sürüm (silinmiş satır hariç) */

    printf("[OK] Sayaç silindi: %s\n\r", seriNo);
    return NO_ERROR;
}

/* ── 2e. Tüm sayaçları listele ───────────────────────────────────────── */
static error_t meter_listele(void)
{
    uint32_t toplam = appMeterOperationsGetMeterCount();
    printf("[OK] Toplam sayaç: %u\n\r", (unsigned)toplam);

    for (uint32_t i = 0u; i < toplam; i++)
    {
        MeterData_t sayac;
        if (appMeterOperationsGetMeterDataByIndex(i, &sayac) == SUCCESS)
        {
            printf("  [%u] %s | %s | %s\n\r",
                   (unsigned)i,
                   sayac.serialNumber,
                   sayac.type,
                   sayac.protocol);
        }
    }

    return NO_ERROR;
}

/* =========================================================================
 * 3. DİREKTİF (DIRECTIVE) İŞLEMLERİ
 *
 * Direktifler JSON formatında gelir, "id" alanı anahtar olarak kullanılır.
 * AppMeterOperations bu işlemleri yönetir.
 * ====================================================================== */

/* ── 3a. Direktif Ekleme ─────────────────────────────────────────────── */
static error_t direktif_ekle(void)
{
    /*
     * JSON formatı:
     *   Zorunlu alan: "id" → direktifin eşsiz tanımlayıcısı
     *   "directive" dizisi → operasyon adımları
     */
    const char *direktifJson =
        "{\"id\":\"ReadoutDirective\",\"directive\":[{\"operation\":\"setBaud\",\"parameter\":\"300\"},{\"operation\":\"setFraming\",\"parameter\":\"7E1\"},{\"operation\":\"sendData\",\"parameter\":\"/?![0D][0A]\"},{\"operation\":\"wait\",\"parameter\":\"10\"},{\"operation\":\"readData\",\"parameter\":\"id\"},{\"operation\":\"sendData\",\"parameter\":\"[06]050[0D][0A]\"},{\"operation\":\"setBaud\",\"parameter\":\"9600\"},{\"operation\":\"wait\",\"parameter\":\"600\"},{\"operation\":\"readData\",\"parameter\":\"rawData\"}]}";

    S32 idx = appMeterOperationsAddDirective(direktifJson);
    if (idx < 0)
    {
        printf("[ERR] Direktif eklenemedi: kod=%d\n\r", (int)idx);
        return ERROR_FAILURE;
    }

    /* Flash'ta ne kaydedildi:
     *   "meter/directive/ReadoutDirective.json" → JSON içeriği
     *   "meter/directive/directive.idx"         → [magic|count|"ReadoutDirective"] */

    printf("[OK] Direktif eklendi, indeks=%d\n\r\r", (int)idx);
    return NO_ERROR;
}

/* ── 3b. İkinci direktif ekleme ─────────────────────────────────────── */
static error_t direktif_ekle_2(void)
{
    const char *profileJson =
        "{\"id\":\"ProfileDirective2\",\"directive\":[{\"operation\":\"setBaud\",\"parameter\":\"300\"},{\"operation\":\"sendData\",\"parameter\":\"/?![0D][0A]\"},{\"operation\":\"wait\",\"parameter\":\"10\"},{\"operation\":\"readData\",\"parameter\":\"id\"},{\"operation\":\"sendData\",\"parameter\":\"P.01(%s)(%s)[0D][0A]\"},{\"operation\":\"readData\",\"parameter\":\"rawData\"}]}";

    S32 idx = appMeterOperationsAddDirective(profileJson);
    printf("[OK] Direktif eklendi: ProfileDirective, idx=%d\n\r", (int)idx);
    return (idx >= 0) ? NO_ERROR : ERROR_FAILURE;
}

/* ── 3c. Direktif Okuma ─────────────────────────────────────────────── */
static error_t direktif_oku(const char *direktifID)
{
    char json[2048];
    memset(json, 0, sizeof(json));

    RETURN_STATUS st = appMeterOperationsGetDirective(direktifID,
                                                       json, sizeof(json));
    if (st != SUCCESS)
    {
        printf("[ERR] Direktif bulunamadı: %s\n\r", direktifID);
        return ERROR_FILE_NOT_FOUND;
    }

    printf("[OK] Direktif okundu: %s\n\r%s\n\r", direktifID, json);
    return NO_ERROR;
}

/* ── 3d. İndeks ile direktif okuma ──────────────────────────────────── */
static error_t direktif_oku_index(uint32_t index)
{
    char json[2048];
    memset(json, 0, sizeof(json));

    RETURN_STATUS st = appMeterOperationsGetDirectiveByIndex(index,
                                                              json, sizeof(json));
    if (st != SUCCESS)
    {
        printf("[ERR] Direktif bulunamadı, indeks=%u\n\r", (unsigned)index);
        return ERROR_FILE_NOT_FOUND;
    }

    printf("[OK] Direktif[%u] okundu:\n\r%s\n\r", (unsigned)index, json);
    return NO_ERROR;
}

/* ── 3e. Direktif Sayısı ─────────────────────────────────────────────── */
static error_t direktif_say(void)
{
    uint32_t toplam = appMeterOperationsGetDirectiveCount();
    printf("[OK] Toplam direktif: %u\n\r", (unsigned)toplam);

    /* Tümünü listele */
    for (uint32_t i = 0u; i < toplam; i++)
    {
        char json[256];
        if (appMeterOperationsGetDirectiveByIndex(i, json, sizeof(json)) == SUCCESS)
        {
            /* "id" alanını çıkar (basit parse) */
            printf("  [%u] ilk 80 karakter: %.80s...\n\r", (unsigned)i, json);
        }
    }

    return NO_ERROR;
}

/* ── 3f. Direktif Silme ──────────────────────────────────────────────── */
static error_t direktif_sil(const char *direktifID)
{
    /* Tek direktif sil */
    RETURN_STATUS st = appMeterOperationsDeleteDirective(direktifID);
    if (st != SUCCESS)
    {
        printf("[ERR] Direktif silinemedi: %s\n\r", direktifID);
        return ERROR_FAILURE;
    }

    /* Flash'ta ne oldu:
     *   "meter/directive/ReadoutDirective.json" → flags = 0x00000000
     *   "meter/directive/directive.idx"         → yeni sürüm (azaltılmış count) */

    printf("[OK] Direktif silindi: %s\n\r", direktifID);
    return NO_ERROR;
}

static error_t direktif_hepsini_sil(void)
{
    /* NULL veya "*" geçilirse tüm direktifler silinir */
    RETURN_STATUS st = appMeterOperationsDeleteDirective("*");
    printf("[OK] Tüm direktifler silindi, sonuç=%d\n\r", (int)st);
    return (st == SUCCESS) ? NO_ERROR : ERROR_FAILURE;
}

/* ── 3g. Direktif Güncelleme (sil + yeniden ekle) ───────────────────── */
static error_t direktif_guncelle(void)
{
    const char *id = "ReadoutDirective";

    /* Önce sil */
    appMeterOperationsDeleteDirective(id);

    /* Yeni içerikle ekle (aynı "id" ile) */
    const char *yeniJson =
        "{\"id\":\"ReadoutDirective\",\"directive\":[{\"operation\":\"setBaud\",\"parameter\":\"1200\"},{\"operation\":\"sendData\",\"parameter\":\"/?![0D][0A]\"},{\"operation\":\"readData\",\"parameter\":\"rawData\"}]}";

    S32 idx = appMeterOperationsAddDirective(yeniJson);
    printf("[OK] Direktif güncellendi: %s, idx=%d\n\r", id, (int)idx);
    return (idx >= 0) ? NO_ERROR : ERROR_FAILURE;
}

/* =========================================================================
 * 4. LOG İŞLEMLERİ
 *
 * İki yol var:
 *   A) AppLogRecorder servisi üzerinden (önerilir — kuyruk + görev yönetir)
 *   B) flashNewStmWriteLog() ile doğrudan (sadece ihtiyaç halinde)
 * ====================================================================== */

/* ── 4a. Doğrudan log yazma ──────────────────────────────────────────── */
static error_t log_yaz(void)
{
    /* Tek satır log */
    const char *mesaj = "20260417 12:00:01 - Sistem başlatıldı";
    CHECK(flashNewStmWriteLog(mesaj, (uint32_t)strlen(mesaj)));

    /* Birden fazla log */
    const char *hataMesaji = "20260417 12:00:02 - [WARN] RS485 timeout";
    CHECK(flashNewStmWriteLog(hataMesaji, (uint32_t)strlen(hataMesaji)));

    const char *veri = "20260417 12:00:03 - [INFO] Sayaç SN12345 okundu";
    CHECK(flashNewStmWriteLog(veri, (uint32_t)strlen(veri)));

    printf("[OK] 3 log girişi yazıldı\n\r");
    return NO_ERROR;
}

/* ── 4b. fsOpenFile / fsWriteFile yoluyla log yazma ──────────────────── */
/*
 * AppLogRecorder bu yolu kullanır.
 * "log/" ile başlayan yollar otomatik olarak LOG bölgesine yönlendirilir.
 * Her fsWriteFile çağrısı, path bilgisini de içeren ayrı bir LOG kaydı yazar.
 * fsCloseFile çağrısına gerek yoktur — her fsWriteFile doğrudan flash'a gider.
 *
 * Yazılan kayıtlar sonradan aynı path ile fsOpenFile(READ) + fsReadFile
 * kullanılarak sırayla okunabilir.
 */
static error_t log_yaz_dosya_api(void)
{
    FsFile *f = fsOpenFile("log/SysLogger/20260417-120001.log",
                           FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
    if (f == NULL)
    {
        printf("[ERR] Log dosyası açılamadı\n\r");
        return ERROR_FAILURE;
    }

    /* Her çağrı: [LogRecHdr | path | satır] şeklinde flash'a yazılır */
    const char *satir1 = "20260417 12:00:01 - Protokol servisi başladı\n";
    const char *satir2 = "20260417 12:00:02 - Bağlantı kuruldu: 192.168.1.100\n";
    const char *satir3 = "20260417 12:00:03 - İlk sayaç okuma görevi kuyruğa alındı\n";

    (void)fsWriteFile(f, (void *)satir1, strlen(satir1));
    (void)fsWriteFile(f, (void *)satir2, strlen(satir2));
    (void)fsWriteFile(f, (void *)satir3, strlen(satir3));

    fsCloseFile(f);
    printf("[OK] Log dosyası yazıldı (3 kayıt, her biri path etiketli)\n\r");
    return NO_ERROR;
}

/* ── 4c. Log okuma — YOL 1: fsOpenFile + fsReadFile (belirli dosya) ──── */
/*
 * fsOpenFile(path, READ) sırasında LOG bölgesi taranır; bu path'e ait
 * tüm kayıtlar birleştirilerek RAM tampona (rBuf) alınır.
 * fsReadFile çağrıları bu tampondan chunk chunk okur.
 * fsCloseFile tampon belleği serbest bırakır.
 */
static error_t log_oku_dosya_api(void)
{
    FsFile *f = fsOpenFile("log/SysLogger/20260417-120001.log",
                           FS_FILE_MODE_READ);
    if (f == NULL)
    {
        printf("[ERR] Log dosyası bulunamadı\n\r");
        return ERROR_FILE_NOT_FOUND;
    }

    printf("[OK] Log içeriği:\n\r");
    char    buf[128];
    size_t  okunan = 0u;
    error_t err;

    while ((err = fsReadFile(f, buf, sizeof(buf) - 1u, &okunan)) == NO_ERROR
           && okunan > 0u)
    {
        buf[okunan] = '\0';
        printf("%s\n\r", buf);
    }

    fsCloseFile(f);
    return (err == ERROR_END_OF_FILE || err == NO_ERROR) ? NO_ERROR : err;
}

/* ── 4d. Log okuma — YOL 2: flashNewStmIterateLogs (tüm bölge, seq filtreli) */
/*
 * Tüm LOG bölgesindeki kayıtları (hem path'li hem anonim) sırayla
 * callback üzerinden iter eder. startSeq ile kesim noktası belirlenebilir.
 * Belirli bir log dosyasını değil, tüm LOG bölgesini okumak için kullanılır.
 */
static void log_callback(uint32_t seq, const char *text,
                          uint32_t len, void *ctx)
{
    (void)ctx;
    printf("  [seq=%u] %.*s\n\r", (unsigned)seq, (int)len, text);
}

static error_t log_oku_hepsini(void)
{
    printf("[OK] Tüm LOG bölgesi (iterator):\n\r");
    CHECK(flashNewStmIterateLogs(0u, log_callback, NULL));
    return NO_ERROR;
}

static error_t log_oku_belirli_seq(uint32_t basSeq)
{
    printf("[OK] seq >= %u olan kayıtlar:\n\r", (unsigned)basSeq);
    CHECK(flashNewStmIterateLogs(basSeq, log_callback, NULL));
    return NO_ERROR;
}

/* ── 4d. Log bölgesini temizle ────────────────────────────────────────── */
/*
 * Dikkat: tüm 128 KB LOG sektörü silinir — tüm geçmiş kayıtlar gider.
 * fsWriteFile yolunda yer dolduğunda FS bunu otomatik yapar.
 * Elle temizleme gerekiyorsa (örn. fabrika sıfırlama):
 */
static error_t log_temizle(void)
{
    CHECK(flashNewStmClearLogs());
    printf("[OK] LOG bölgesi temizlendi\n\r");
    return NO_ERROR;
}

/* ── 4e. Boş alan kontrolü ───────────────────────────────────────────── */
static error_t log_bos_alan(void)
{
    uint32_t logFree = 0u;
    CHECK(flashNewStmGetFreeSpace(NULL, &logFree, NULL));
    printf("[OK] LOG bölgesi boş alan: %u byte (%.1f KB)\n\r",
           (unsigned)logFree, (float)logFree / 1024.0f);
    return NO_ERROR;
}

/* =========================================================================
 * 5. KONFİGÜRASYON (CONFIG) İŞLEMLERİ
 *
 * Sistem konfigürasyonu, protokol ayarları, ağ ayarları gibi
 * küçük boyutlu ama kalıcı JSON/binary veriler buraya kaydedilir.
 * ====================================================================== */

/* ── 5a. JSON konfigürasyon yazma ────────────────────────────────────── */
static error_t conf_yaz_json(void)
{
    /* Protokol konfigürasyonu — JSON string */
    const char *protoConf =
        "{\"registered\":true,\"serverId\":\"srv-001\",\"serverIp\":\"192.168.1.200\",\"serverPort\":8883,\"mqttTopic\":\"aviora/meter/SN12345\",\"heartbeatInterval\":60,\"reconnectDelay\":5}";

    CHECK(flashNewStmWriteConfig("proto/config.json",
                                  protoConf,
                                  (uint32_t)strlen(protoConf)));

    printf("[OK] Protokol konfigürasyonu yazıldı (%u byte)\n\r",
           (unsigned)strlen(protoConf));
    return NO_ERROR;
}

/* ── 5b. Sistem konfigürasyonu yazma (binary struct) ─────────────────── */
typedef struct
{
    char  deviceId[32];
    char  timezone[16];
    int   logLevel;
    int   readIntervalSec;
    int   uploadIntervalSec;
} SysConf_t;

static error_t conf_yaz_binary(void)
{
    SysConf_t sysConf = {0};
    strncpy(sysConf.deviceId,  "AVIORA-GW-001", sizeof(sysConf.deviceId) - 1);
    strncpy(sysConf.timezone,  "Europe/Istanbul", sizeof(sysConf.timezone) - 1);
    sysConf.logLevel           = 2;   /* INFO */
    sysConf.readIntervalSec    = 900; /* 15 dakika */
    sysConf.uploadIntervalSec  = 300; /* 5 dakika */

    CHECK(flashNewStmWriteConfig("sys/config",
                                  &sysConf,
                                  sizeof(sysConf)));

    printf("[OK] Sistem konfigürasyonu yazıldı (%u byte)\n\r",
           (unsigned)sizeof(sysConf));
    return NO_ERROR;
}

/* ── 5c. Konfigürasyon okuma (JSON) ──────────────────────────────────── */
static error_t conf_oku_json(void)
{
    char    buf[512];
    uint32_t okunan = 0u;

    error_t err = flashNewStmReadConfig("proto/config.json",
                                         buf, sizeof(buf) - 1u,
                                         &okunan);
    if (err != NO_ERROR)
    {
        printf("[ERR] proto/config.json bulunamadı\n\r");
        return err;
    }

    buf[okunan] = '\0';  /* null-sonlandır */
    printf("[OK] proto/config.json (%u byte):\n\r%s\n\r", (unsigned)okunan, buf);
    return NO_ERROR;
}

/* ── 5d. Konfigürasyon okuma (binary struct) ─────────────────────────── */
static error_t conf_oku_binary(void)
{
    SysConf_t sysConf;
    uint32_t  okunan = 0u;

    CHECK(flashNewStmReadConfig("sys/config",
                                 &sysConf,
                                 sizeof(sysConf),
                                 &okunan));

    if (okunan < sizeof(sysConf))
    {
        printf("[ERR] sys/config boyutu hatalı: %u != %u\n\r",
               (unsigned)okunan, (unsigned)sizeof(sysConf));
        return ERROR_FAILURE;
    }

    printf("[OK] sys/config:\n\r"
           "  deviceId           = %s\n\r"
           "  timezone           = %s\n\r"
           "  logLevel           = %d\n\r"
           "  readIntervalSec    = %d\n\r"
           "  uploadIntervalSec  = %d\n\r",
           sysConf.deviceId,
           sysConf.timezone,
           sysConf.logLevel,
           sysConf.readIntervalSec,
           sysConf.uploadIntervalSec);

    return NO_ERROR;
}

/* ── 5e. Konfigürasyon Güncelleme ────────────────────────────────────── */
/*
 * CONFIG bölgesi append-only log'dur; "güncelleme" = yeni sürüm yaz.
 * Eski sürüm otomatik olarak geçersiz sayılır (RAM indeksinde üst yazılır).
 */
static error_t conf_guncelle(void)
{
    /* Mevcut konfigürasyonu oku */
    SysConf_t sysConf;
    uint32_t  okunan = 0u;

    error_t err = flashNewStmReadConfig("sys/config",
                                         &sysConf,
                                         sizeof(sysConf),
                                         &okunan);
    if (err != NO_ERROR)
    {
        /* Daha önce hiç yazılmamışsa varsayılanı kullan */
        memset(&sysConf, 0, sizeof(sysConf));
        strncpy(sysConf.deviceId, "AVIORA-GW-001", sizeof(sysConf.deviceId) - 1);
    }

    /* Güncelle */
    sysConf.logLevel          = 3;   /* DEBUG seviyesine çık */
    sysConf.readIntervalSec   = 600; /* 10 dakikaya düşür */

    /* Aynı path'e yaz → yeni sürüm oluşur, eski sürüm geçersiz olur */
    CHECK(flashNewStmWriteConfig("sys/config",
                                  &sysConf,
                                  sizeof(sysConf)));

    printf("[OK] sys/config güncellendi: logLevel=%d, interval=%d\n\r",
           sysConf.logLevel, sysConf.readIntervalSec);
    return NO_ERROR;
}

/* ── 5f. Konfigürasyon Silme ─────────────────────────────────────────── */
static error_t conf_sil(void)
{
    /* flags alanı 0x00000000 yapılır — flash erase gerektirmez */
    CHECK(flashNewStmDeleteConfig("proto/config.json"));
    printf("[OK] proto/config.json silindi\n\r");

    /* Silme sonrası okumaya çalışırsak ERROR_FILE_NOT_FOUND döner */
    char buf[16];
    uint32_t n = 0u;
    error_t err = flashNewStmReadConfig("proto/config.json",
                                         buf, sizeof(buf), &n);
    printf("[OK] Silme sonrası okuma: %s\n\r",
           (err == ERROR_FILE_NOT_FOUND) ? "ERROR_FILE_NOT_FOUND (beklenen)" : "HATA!");

    return NO_ERROR;
}

/* ── 5g. Konfigürasyon varlık kontrolü ──────────────────────────────── */
static error_t conf_varlik_kontrol(void)
{
    if (fsFileExists("sys/config"))
        printf("[OK] sys/config mevcut\n\r");
    else
        printf("[OK] sys/config YOK (ilk kurulum yapılmalı)\n\r");

    uint32_t boyut = 0u;
    if (fsGetFileSize("sys/config", &boyut) == NO_ERROR)
        printf("[OK] sys/config boyutu: %u byte\n\r", (unsigned)boyut);

    return NO_ERROR;
}

/* ── 5h. fsOpenFile / fsWriteFile / fsReadFile yoluyla config ─────────── */
/*
 * AppMeterOperations ve diğer servisler bu API'yi kullanır.
 * CONFIG dosyaları için yazma tampona alınır, fsCloseFile'da flash'a yazılır.
 */
static error_t conf_dosya_api(void)
{
    /* YAZMA: ağ ayarları */
    {
        FsFile *f = fsOpenFile("net/settings.json",
                               FS_FILE_MODE_WRITE  |
                               FS_FILE_MODE_CREATE |
                               FS_FILE_MODE_TRUNC);
        if (f == NULL) { printf("[ERR] Açılamadı\n\r"); return ERROR_FAILURE; }

        const char *json =
            "{\"dhcp\":true,\"ip\":\"\",\"gw\":\"\",\"dns\":\"8.8.8.8\"}";
        (void)fsWriteFile(f, (void *)json, strlen(json));

        fsCloseFile(f);   /* ← burada flash'a yazılır */
        printf("[OK] net/settings.json yazıldı\n\r");
    }

    /* OKUMA */
    {
        FsFile *f = fsOpenFile("net/settings.json", FS_FILE_MODE_READ);
        if (f == NULL) { printf("[ERR] Açılamadı\n\r"); return ERROR_FAILURE; }

        char buf[256];
        size_t okunan = 0u;
        (void)fsReadFile(f, buf, sizeof(buf) - 1u, &okunan);
        buf[okunan] = '\0';
        fsCloseFile(f);

        printf("[OK] net/settings.json (%u byte): %s\n\r",
               (unsigned)okunan, buf);
    }

    return NO_ERROR;
}

/* ── 5i. CONFIG bölgesi sıkıştırma (Compact) ─────────────────────────── */
/*
 * Compact ne zaman gerekir?
 *   - flashNewStmWriteConfig zaten otomatik compact tetikler (alan dolunca).
 *   - Elle tetiklemek istersen (örn. bakım penceresi):
 */
static error_t conf_compact(void)
{
    uint32_t once = 0u, sonra = 0u;
    (void)flashNewStmCfgFreeSpace(&once);

    CHECK(flashNewStmCompactConfig());

    (void)flashNewStmCfgFreeSpace(&sonra);
    printf("[OK] Compact: önce=%u byte, sonra=%u byte (kazanılan=%d byte)\n\r",
           (unsigned)once, (unsigned)sonra, (int)(sonra - once));
    return NO_ERROR;
}

/* =========================================================================
 * 6. SENSOR / SAYAÇ ÖLÇÜM VERİSİ (DATA bölgesi)
 *
 * DATA bölgesi iki farklı kayıt türünü destekler:
 *
 *  A) Tiplendirilmiş binary ölçüm kaydı (pathLen=0)
 *     → flashNewStmWriteData() ile yaz
 *     → flashNewStmIterateData() ile oku (callback)
 *     → Örnek: periyodik enerji ölçümleri
 *
 *  B) Path'li dosya kaydı (pathLen>0)
 *     → fsOpenFile + fsWriteFile ile yaz (her çağrı bir DATA kaydı)
 *     → fsOpenFile + fsReadFile ile oku (eşleşen kayıtlar rBuf'a toplanır)
 *     → fsDeleteFile ile sil (flags geçersiz yapılır, sector erase yok)
 *     → Örnek: meter/meterDataOut/<taskId>_readout.txt
 *
 *  flashNewStmIterateData() yalnızca tip bazlı kayıtları (pathLen=0) döner;
 *  path'li dosya kayıtlarını atlar.
 * ====================================================================== */

/* Uygulama özel veri yapısı (tip bazlı ölçüm) */
typedef struct
{
    char     serialNumber[16];
    float    activeEnergy_kWh;
    float    reactiveEnergy_kVArh;
    float    voltage_V;
    float    current_A;
    uint32_t readTime;           /* Unix timestamp */
} MeterReading_t;

/* ── 6a. Tiplendirilmiş ölçüm verisi yaz (flashNewStmWriteData) ──────── */
static error_t data_yaz(void)
{
    MeterReading_t okuma = {0};
    strncpy(okuma.serialNumber, "SN12345678", sizeof(okuma.serialNumber) - 1);
    okuma.activeEnergy_kWh     = 12345.67f;
    okuma.reactiveEnergy_kVArh = 456.78f;
    okuma.voltage_V             = 230.5f;
    okuma.current_A             = 5.2f;
    okuma.readTime              = 1745000000u;

    CHECK(flashNewStmWriteData(FSNEW_DATA_METER,
                                &okuma,
                                sizeof(okuma),
                                okuma.readTime));

    printf("[OK] Ölçüm verisi yazıldı: %s → %.2f kWh\n\r",
           okuma.serialNumber, okuma.activeEnergy_kWh);
    return NO_ERROR;
}

/* ── 6b. Birden fazla tiplendirilmiş ölçüm kaydet ───────────────────── */
static error_t data_coklu_yaz(void)
{
    const char *sayaclar[] = {"SN00001", "SN00002", "SN00003"};
    float       enerjiler[] = {100.0f, 250.5f, 87.3f};

    for (int i = 0; i < 3; i++)
    {
        MeterReading_t okuma = {0};
        strncpy(okuma.serialNumber, sayaclar[i],
                sizeof(okuma.serialNumber) - 1);
        okuma.activeEnergy_kWh = enerjiler[i];
        okuma.voltage_V        = 231.0f;
        okuma.current_A        = (float)(i + 1) * 2.5f;
        okuma.readTime         = 1745000000u + (uint32_t)(i * 900u);

        error_t err = flashNewStmWriteData(FSNEW_DATA_METER,
                                            &okuma, sizeof(okuma),
                                            okuma.readTime);
        if (err != NO_ERROR)
            printf("[ERR] %s yazılamadı: %d\n\r", sayaclar[i], (int)err);
        else
            printf("[OK] Yazıldı: %s\n\r", sayaclar[i]);
    }
    return NO_ERROR;
}

/* ── 6c. Tiplendirilmiş ölçüm oku — flashNewStmIterateData (callback) ── */
static void data_callback(uint32_t seq, uint32_t ts, uint32_t type,
                           const void *data, uint32_t size, void *ctx)
{
    (void)ctx; (void)type;
    if (data == NULL || size < sizeof(MeterReading_t))
    {
        printf("  [seq=%u, ts=%u] ham boyut=%u\n\r",
               (unsigned)seq, (unsigned)ts, (unsigned)size);
        return;
    }
    const MeterReading_t *r = (const MeterReading_t *)data;
    printf("  [seq=%u, ts=%u] %s → %.2f kWh, %.1f V, %.2f A\n\r",
           (unsigned)seq, (unsigned)ts,
           r->serialNumber, r->activeEnergy_kWh, r->voltage_V, r->current_A);
}

static error_t data_oku_iterator(void)
{
    printf("[OK] Tüm METER ölçümleri (iterator):\n\r");
    CHECK(flashNewStmIterateData(FSNEW_DATA_METER, 0u, data_callback, NULL));
    return NO_ERROR;
}

static error_t data_oku_son_N(uint32_t basSeq)
{
    printf("[OK] seq >= %u olan ölçümler:\n\r", (unsigned)basSeq);
    CHECK(flashNewStmIterateData(FSNEW_DATA_METER, basSeq, data_callback, NULL));
    return NO_ERROR;
}

/* ── 6d. Path'li dosya yaz — fsOpenFile + fsWriteFile ───────────────── */
/*
 * meter/meterDataOut/ yolları DATA bölgesine yönlendirilir.
 * Her fsWriteFile çağrısı path bilgisini de içeren ayrı bir DATA kaydı yazar.
 * flashNewStmIterateData bu kayıtları görmez (pathLen>0 olanları atlar).
 */
static error_t data_yaz_dosya_api(void)
{
    const char *path = "meter/meterDataOut/1_readout.txt";

    FsFile *f = fsOpenFile(path, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
    if (f == NULL)
    {
        printf("[ERR] %s açılamadı\n\r", path);
        return ERROR_FAILURE;
    }

    /* IEC 62056-21 tipik readout satırları */
    const char *satirlar[] = {
        "/ISk5MT382-1000\r\n",
        "0.0.1(SN12345678)\r\n",
        "1.8.0(012345.678*kWh)\r\n",
        "1.8.1(006789.123*kWh)\r\n",
        "2.8.0(000456.789*kWh)\r\n",
        "!\r\n",
        NULL
    };

    for (int i = 0; satirlar[i] != NULL; i++)
        (void)fsWriteFile(f, (void *)satirlar[i], strlen(satirlar[i]));

    fsCloseFile(f);
    printf("[OK] %s yazıldı (6 satır, her biri path etiketli DATA kaydı)\n\r",
           path);
    return NO_ERROR;
}

/* ── 6e. Path'li dosya oku — fsOpenFile + fsReadFile ────────────────── */
/*
 * fsOpenFile(READ) sırasında DATA bölgesi taranır; bu path'e ait
 * tüm kayıtlar (pathLen>0) birleştirilerek RAM tampona (rBuf) alınır.
 * fsReadFile çağrıları bu tampondan chunk chunk okur.
 * fsCloseFile tampon belleği serbest bırakır.
 */
static error_t data_oku_dosya_api(void)
{
    const char *path = "meter/meterDataOut/1_readout.txt";

    FsFile *f = fsOpenFile(path, FS_FILE_MODE_READ);
    if (f == NULL)
    {
        printf("[ERR] %s bulunamadı\n\r", path);
        return ERROR_FILE_NOT_FOUND;
    }

    printf("[OK] %s içeriği:\n\r", path);
    char   buf[128];
    size_t okunan = 0u;
    error_t err;

    while ((err = fsReadFile(f, buf, sizeof(buf) - 1u, &okunan)) == NO_ERROR
           && okunan > 0u)
    {
        buf[okunan] = '\0';
        printf("%s", buf);
    }

    fsCloseFile(f);
    return (err == ERROR_END_OF_FILE || err == NO_ERROR) ? NO_ERROR : err;
}

/* ── 6f. Path'li dosya sil — fsDeleteFile ────────────────────────────── */
/*
 * flags alanı 0x00000000 yapılır — sector silme gerekmez.
 * fsFileExists ile silindiği doğrulanabilir.
 */
static error_t data_sil_dosya(void)
{
    const char *path = "meter/meterDataOut/1_readout.txt";

    error_t err = fsDeleteFile(path);
    if (err != NO_ERROR)
    {
        printf("[ERR] Silme başarısız: %s → %d\n\r", path, (int)err);
        return err;
    }

    printf("[OK] Silindi: %s\n\r", path);
    printf("[OK] Varlık kontrolü: %s\n\r",
           fsFileExists(path) ? "HÂLÂ VAR (hata!)" : "bulunamadı (beklenen)");
    return NO_ERROR;
}

/* ── 6g. DATA bölgesini temizle ─────────────────────────────────────── */
static error_t data_temizle(void)
{
    CHECK(flashNewStmClearData());
    printf("[OK] DATA bölgesi temizlendi (tüm kayıtlar silindi)\n\r");
    return NO_ERROR;
}

/* ── 6e. Tüm bölgelerin boş alan raporu ─────────────────────────────── */
static error_t bos_alan_raporu(void)
{
    uint32_t cfgFree = 0u, logFree = 0u, dataFree = 0u;
    CHECK(flashNewStmGetFreeSpace(&cfgFree, &logFree, &dataFree));

    printf("[OK] Boş Alan Raporu:\n\r"
           "  CONFIG  : %6u byte (%5.1f KB / 128 KB)\n\r"
           "  LOG     : %6u byte (%5.1f KB / 128 KB)\n\r"
           "  DATA    : %6u byte (%5.1f KB / 128 KB)\n\r",
           (unsigned)cfgFree,  (float)cfgFree  / 1024.0f,
           (unsigned)logFree,  (float)logFree  / 1024.0f,
           (unsigned)dataFree, (float)dataFree / 1024.0f);

    return NO_ERROR;
}

/* =========================================================================
 * 7. TAM SENARYO — Tümünü bir arada dene
 * ====================================================================== */

void fs_flash_new_stm_demo(void)
{
    printf("\n\r══════════════════════════════════════════════\n\r\r");
    printf(" fs_flash_new_stm kullanım örneği\n\r");
    printf("══════════════════════════════════════════════\n\r");

    /* 1. Başlat */
    if (system_init() != NO_ERROR) return;

    /* 2. Boş alan (başlangıç) */
    bos_alan_raporu();

    /* ── CONFIG ── */
    printf("\n--- KONFİGÜRASYON ---\n\r");
    conf_yaz_json();
    conf_yaz_binary();
    conf_oku_json();
    conf_oku_binary();
    conf_guncelle();
    conf_oku_binary();        /* güncellenmiş hali oku */
    conf_varlik_kontrol();
    conf_dosya_api();
    conf_sil();               /* proto/config.json sil */

    /* ── METER ── */
    printf("\n--- SAYAÇ İŞLEMLERİ ---\n\r");
    initMeterTest();
    meter_listele();
    meter_ekle_yuksek_seviye("SN12345678");
    meter_ekle_yuksek_seviye("SN12345666");
    meter_ekle_yuksek_seviye("SN12345646");
//    meter_ekle_dusuk_seviye();
    meter_listele();
    meter_oku("SN12345678");
    meter_sil("SN12345678");
    meter_listele();          /* 1 sayaç kaldı */

    /* ── DİREKTİF ── */
    printf("\n--- DİREKTİF İŞLEMLERİ ---\n\r");
    direktif_say();
    direktif_ekle();
    direktif_ekle_2();
    direktif_say();
    direktif_oku("ReadoutDirective");
    direktif_oku_index(1u);
    direktif_guncelle();
    direktif_oku("ReadoutDirective");  /* güncellenmiş hali */
    direktif_sil("ReadoutDirective");
    direktif_say();           /* 1 direktif kaldı */

    /* ── LOG ── */
    printf("\n--- LOG İŞLEMLERİ ---\n\r");
    /* Yazma: anonim (flashNewStmWriteLog) */
    log_yaz();
    /* Yazma: path'li (fsOpenFile + fsWriteFile) */
    log_yaz_dosya_api();
    log_bos_alan();
    /* Okuma 1: belirli dosyayı fsReadFile ile oku */
    log_oku_dosya_api();
    /* Okuma 2: tüm bölgeyi iterator ile tara */
    log_oku_hepsini();
    log_oku_belirli_seq(3u);
    log_temizle();
    log_oku_hepsini();        /* boş olmalı */

    /* ── DATA ── */
    printf("\n--- ÖLÇÜM VERİSİ (tip bazlı) ---\n\r");
    /* Yazma: flashNewStmWriteData (pathLen=0, iterator ile okunur) */
    data_yaz();
    data_coklu_yaz();
    /* Okuma: flashNewStmIterateData callback */
    data_oku_iterator();
    data_oku_son_N(3u);

    printf("\n--- ÖLÇÜM VERİSİ (path'li dosya) ---\n\r");
    /* Yazma: fsOpenFile + fsWriteFile (pathLen>0, meterDataOut gibi) */
    data_yaz_dosya_api();
    /* Okuma: fsOpenFile + fsReadFile */
    data_oku_dosya_api();
    /* Silme: fsDeleteFile (flags geçersiz, sector erase yok) */
    data_sil_dosya();

    data_temizle();
    data_oku_iterator();      /* boş olmalı */

    /* Compact (isteğe bağlı bakım) */
    printf("\n--- CONFIG COMPACT ---\n\r");
    conf_compact();

    /* Son boş alan raporu */
    printf("\n--- SONUÇ ---\n");
    bos_alan_raporu();

    printf("\n══════════════════════════════════════════════\n\r");
    printf(" Demo tamamlandı\n\r");
    printf("════════════════════════════════====\n\r");
}

/******************************** End Of File ********************************/
