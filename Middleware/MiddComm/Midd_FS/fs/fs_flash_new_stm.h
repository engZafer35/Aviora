/**
 * @file  fs_flash_new_stm.h
 * @brief STM32F407VGTA iç-flash çok-bölgeli dosya sistemi
 *
 * ─────────────────────────────────────────────────────────────────────────
 * FLASH LAYOUT  (varsayılan: sector 9-11, 3 × 128 KB)
 * ─────────────────────────────────────────────────────────────────────────
 *
 *  Sector  9  (128 KB @ 0x080A0000)  ──►  CONFIG bölgesi
 *    • Sistem konfigürasyonu (JSON)
 *    • Protokol konfigürasyonu (JSON)
 *    • Meter listesi
 *    • Direktif listesi
 *    Strateji: sıralı-ekleme kaydı (append log); silme: mevcut kaydın
 *    bayrak alanı 0x00000000 yazılarak geçersiz kılınır (erase gerekmez,
 *    bit 1→0); alan dolunca compact (erase + son sürümleri yeniden yaz).
 *
 *  Sector 10  (128 KB @ 0x080C0000)  ──►  LOG bölgesi
 *    • AppLogRecorder text log girişleri
 *    Strateji: dairesel ekleme; sector dolunca tüm sector silinip baştan
 *    yazılır (tek-sector varsayılan).
 *
 *  Sector 11  (128 KB @ 0x080E0000)  ──►  DATA bölgesi
 *    • Sensor ölçümleri, sayaç verileri (tipli ikili kayıtlar)
 *    Strateji: LOG ile aynı.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * FLASH KISITLARI  (STM32F407, VRANGE = 3)
 * ─────────────────────────────────────────────────────────────────────────
 *  • Minimum yazma birimi : 4 bayt (32-bit word, 4-bayt hizalı adres)
 *  • Silinmiş hücre değeri: 0xFF
 *  • Silme granülasyonu   : tam sector (128 KB sektörler 9-11 için)
 *  • Üzerine yazma YOK   : bit yalnız 1→0 yönünde programlanabilir;
 *                           0→1 için önce sector erase gereklidir.
 *
 * ─────────────────────────────────────────────────────────────────────────
 * KULLANIM
 * ─────────────────────────────────────────────────────────────────────────
 *  1. flashNewStmConfigure() ile flash ops + adres bilgilerini ver.
 *  2. İlk kullanımda flashNewStmFormat() çağır (tüm bölgeleri siler).
 *  3. Her açılışta fsInit() çağır → bölgeleri tarar, RAM indeksini kurar.
 *  4. Konfigürasyon dosyaları için standart fs_port API kullan:
 *       fsOpenFile / fsWriteFile / fsReadFile / fsCloseFile
 *  5. Log girişleri için: flashNewStmWriteLog()
 *  6. Sensör/sayaç verisi için: flashNewStmWriteData()
 *
 * ─────────────────────────────────────────────────────────────────────────
 * PATH KURALI  (fsOpenFile için)
 * ─────────────────────────────────────────────────────────────────────────
 *  "/log/…"  veya  "/logs/…"  → LOG bölgesi  (sadece yazma, okuma YOK)
 *  "/data/…"                  → DATA bölgesi (sadece yazma, okuma YOK)
 *  diğer tüm yollar           → CONFIG bölgesi (okuma + yazma)
 *
 * @author  Zafer Satılmış
 * @date    Apr 2026
 */

#ifndef FS_FLASH_NEW_STM_H
#define FS_FLASH_NEW_STM_H

#include "fs_port.h"
//#include "fs_port_flashFS.h"   /* FlashFsOps yeniden kullanılıyor */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Derleme-zamanı ayarları  (ihtiyaç halinde derleme ortamından geçersiz
 * kılınabilir)
 * ====================================================================== */

/** CONFIG bölgesindeki bir kayıt için maksimum yol uzunluğu (byte) */
#ifndef FSNEW_PATH_MAX
#  define FSNEW_PATH_MAX          64u
#endif

/** RAM'de tutulan config-dosyası indeks girişi sayısı */
#ifndef FSNEW_CFG_INDEX_SIZE
#  define FSNEW_CFG_INDEX_SIZE    24u
#endif

/** Aynı anda açık tutulabilecek maksimum dosya tutacağı sayısı */
#ifndef FSNEW_MAX_OPEN_FILES
#  define FSNEW_MAX_OPEN_FILES    6u
#endif

/**
 * Tek bir config kaydının maksimum payload boyutu (byte).
 * Bu değer fsCloseFile'da RAM'de tutulacak tamponu sınırlar.
 */
#ifndef FSNEW_CFG_MAX_PAYLOAD
#  define FSNEW_CFG_MAX_PAYLOAD   (8u * 1024u)
#endif

/* =========================================================================
 * STM32F407VGTA varsayılan flash adresleri  (sector 9-11)
 * ====================================================================== */
#define FSNEW_DEFAULT_CFG_BASE    0x080A0000u
#define FSNEW_DEFAULT_CFG_SIZE    (128u * 1024u)
#define FSNEW_DEFAULT_LOG_BASE    0x080C0000u
#define FSNEW_DEFAULT_LOG_SIZE    (128u * 1024u)
#define FSNEW_DEFAULT_DATA_BASE   0x080E0000u
#define FSNEW_DEFAULT_DATA_SIZE   (128u * 1024u)
#define FSNEW_DEFAULT_SECTOR_SZ   (128u * 1024u)
#define FSNEW_DEFAULT_PROG_MIN    4u

/* =========================================================================
 * Veri tipleri
 * ====================================================================== */

typedef struct
{
   /**
    * @brief Read bytes from flash
    * @param[in] addr Absolute flash address
    * @param[out] data Destination buffer
    * @param[in] len Number of bytes
    * @return Error code
    **/
   error_t (*read)(uint32_t addr, void *data, size_t len);

   /**
    * @brief Program bytes to flash (must respect target's programming rules)
    * @param[in] addr Absolute flash address
    * @param[in] data Source buffer
    * @param[in] len Number of bytes
    * @return Error code
    **/
   error_t (*prog)(uint32_t addr, const void *data, size_t len);

   /**
    * @brief Erase one erase-block containing addr (or erase at exact addr)
    * @param[in] addr Absolute flash address aligned to erase block
    * @return Error code
    **/
   error_t (*erase)(uint32_t addr);

   /**
    * @brief Optional sync / flush
    * @return Error code
    **/
   error_t (*sync)(void);
} FlashStmOps;


/**
 * @brief Donanım operasyonları ve bölge geometrisi.
 *
 * Üç bölge aynı flash ops'u paylaşır; farklı adres aralıklarında bulunur.
 * Her bölgenin boyutu sectorSize'ın katı olmalıdır.
 */
typedef struct
{
    FlashStmOps ops;        /**< flash read / prog / erase / sync callback'leri  */

    uint32_t   cfgBase;    /**< CONFIG bölgesi başlangıç adresi                */
    uint32_t   cfgSize;    /**< CONFIG bölgesi toplam boyutu (byte)            */

    uint32_t   logBase;    /**< LOG bölgesi başlangıç adresi                   */
    uint32_t   logSize;    /**< LOG bölgesi toplam boyutu (byte)               */

    uint32_t   dataBase;   /**< DATA bölgesi başlangıç adresi                  */
    uint32_t   dataSize;   /**< DATA bölgesi toplam boyutu (byte)              */

    uint32_t   sectorSize; /**< Silme bloğu boyutu (örn. 128 * 1024)           */
    uint32_t   progMin;    /**< Minimum yazma hizalaması (STM32F4 için 4)      */
} FlashNewStmGeom;

/**
 * @brief DATA bölgesi için kayıt tipi tanımlayıcıları.
 *
 * Uygulama kodu 0x0100 ve üzeri değerleri tanımlayabilir.
 */
typedef enum
{
    FSNEW_DATA_SENSOR  = 0x0001u,  /**< Sensör ölçümü            */
    FSNEW_DATA_METER   = 0x0002u,  /**< Sayaç ölçümü             */
    FSNEW_DATA_EVENT   = 0x0003u,  /**< Sistem olayı             */
} FsNewDataType;

/* =========================================================================
 * Başlatma / biçimlendirme
 * ====================================================================== */

/**
 * @brief Donanım ops'larını ve geometriyi yapılandır.
 *
 * fsInit() veya flashNewStmFormat() öncesinde çağrılmalıdır.
 *
 * @param geom  Doldurulmuş FlashNewStmGeom yapısına işaretçi.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmConfigure(const FlashNewStmGeom *geom);

/**
 * @brief Üç bölgeyi de silerek taze bölge başlıkları yaz.
 *
 * Yalnızca ilk kullanımda ya da fabrika sıfırlamada çağrılmalıdır.
 *
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmFormat(void);

/**
 * @brief Mutex'leri sil (sistem kapatılırken veya yeniden yapılandırmada).
 *
 * flashNewStmConfigure() ile oluşturulan üç mutex'i serbest bırakır.
 */
void flashNewStmDeleteMutexes(void);

/* =========================================================================
 * Standart fs_port API  (CycloneTCP fs_port ile aynı imzalar)
 *
 * NOT: Bu modül derlemeye dahil edildiğinde diğer FS implementasyonları
 *      (fs_port_flashFS.c, fs_port_littlefs.c, vb.) aynı anda
 *      derlenmemelidir — fonksiyon isimleri çakışır.
 * ====================================================================== */

typedef void FsFile;
typedef void FsDir;

error_t  fsInit(void);

bool_t   fsFileExists (const char_t *path);
error_t  fsGetFileSize(const char_t *path, uint32_t *size);
error_t  fsGetFileStat(const char_t *path, FsFileStat *fileStat);
error_t  fsRenameFile (const char_t *oldPath, const char_t *newPath);
error_t  fsDeleteFile (const char_t *path);

FsFile  *fsOpenFile   (const char_t *path, uint_t mode);
error_t  fsSeekFile   (FsFile *file, int_t offset, uint_t origin);
error_t  fsWriteFile  (FsFile *file, void *data, size_t length);
error_t  fsReadFile   (FsFile *file, void *data, size_t size, size_t *length);
void     fsCloseFile  (FsFile *file);

bool_t   fsDirExists  (const char_t *path);
error_t  fsCreateDir  (const char_t *path);
error_t  fsRemoveDir  (const char_t *path);

FsDir   *fsOpenDir    (const char_t *path);
error_t  fsReadDir    (FsDir *dir, FsDirEntry *dirEntry);
void     fsCloseDir   (FsDir *dir);

/* =========================================================================
 * Yüksek seviyeli CONFIG bölgesi yardımcıları
 * ====================================================================== */

/**
 * @brief Adlandırılmış bir config girişini yaz (veya güncelle).
 *
 * Giriş CONFIG bölgesine eklenir. Yer yoksa compact() otomatik tetiklenir.
 *
 * @param path  Anahtar olarak kullanılan null-sonlandırmalı yol.
 * @param data  Yük verisi (JSON, binary, vs.).
 * @param size  Yük boyutu (byte).
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmWriteConfig(const char *path,
                                const void *data, uint32_t size);

/**
 * @brief Adlandırılmış bir config girişinin son sürümünü oku.
 *
 * @param path     Aranacak anahtar.
 * @param buf      Çıktı tamponu.
 * @param maxSize  Tampon kapasitesi (byte).
 * @param outSize  Okunan gerçek byte sayısı.
 * @return NO_ERROR, ERROR_FILE_NOT_FOUND veya hata kodu.
 */
error_t flashNewStmReadConfig(const char *path,
                               void *buf, uint32_t maxSize,
                               uint32_t *outSize);

/**
 * @brief Bir config girişini geçersiz olarak işaretle (flash silme gerektirmez).
 *
 * Mevcut kaydın bayrak alanına 0x00000000 yazılarak giriş silinmiş sayılır.
 *
 * @param path  Silinecek kayıt anahtarı.
 * @return NO_ERROR, ERROR_FILE_NOT_FOUND veya hata kodu.
 */
error_t flashNewStmDeleteConfig(const char *path);

/**
 * @brief CONFIG bölgesini sıkıştır.
 *
 * Son geçerli kayıtlar RAM'e okunur, bölge silinir, kayıtlar geri yazılır.
 * Düşük boş alan algılandığında veya elle çağrılabilir.
 *
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmCompactConfig(void);

/**
 * @brief CONFIG bölgesindeki boş alan miktarını döndür (byte).
 *
 * @param freeBytes  Kalan boş alan (byte) çıktısı.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmCfgFreeSpace(uint32_t *freeBytes);

/* =========================================================================
 * Yüksek seviyeli LOG bölgesi yardımcıları
 * ====================================================================== */

/**
 * @brief LOG bölgesine bir metin girdisi ekle.
 *
 * Bölge doluysa tüm sector silinip sıfırdan yazılır (en eski kayıtlar
 * kaybolur). Kesme güvenli değildir; çağrı öncesi mutex kullanın.
 *
 * @param text  Yazılacak metin (null-sonlandırıcı gerekli değil).
 * @param len   Yazılacak byte sayısı.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmWriteLog(const char *text, uint32_t len);

/**
 * @brief LOG bölgesindeki tüm geçerli girişleri en eskiden yeniye dolaş.
 *
 * @param startSeq  Bu değerden küçük seq'li girişleri atla (0 = hepsi).
 * @param cb        Her geçerli giriş için çağrılan callback.
 * @param ctx       Callback'e geçirilen kullanıcı bağlamı.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmIterateLogs(
        uint32_t startSeq,
        void (*cb)(uint32_t seq, const char *text,
                   uint32_t len, void *ctx),
        void *ctx);

/**
 * @brief LOG bölgesini tamamen temizle (sector sil + başlık yeniden yaz).
 *
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmClearLogs(void);

/* =========================================================================
 * Yüksek seviyeli DATA bölgesi yardımcıları
 * ====================================================================== */

/**
 * @brief DATA bölgesine tiplendirilmiş bir kayıt ekle.
 *
 * @param type       Uygulama tanımlı tip ID'si (bkz. FsNewDataType).
 * @param data       İkili yük verisi.
 * @param size       Yük boyutu (byte).
 * @param timestamp  Unix zaman damgası (saniye); bilinmiyorsa 0.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmWriteData(uint32_t type,
                              const void *data, uint32_t size,
                              uint32_t timestamp);

/**
 * @brief DATA bölgesindeki kayıtları dolaş.
 *
 * @param type      Filtre tipi (0 = tüm tipler).
 * @param startSeq  Bu değerden küçük seq'li kayıtları atla (0 = hepsi).
 * @param cb        Her eşleşen kayıt için çağrılan callback.
 * @param ctx       Kullanıcı bağlamı.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmIterateData(
        uint32_t type, uint32_t startSeq,
        void (*cb)(uint32_t seq, uint32_t ts, uint32_t type,
                   const void *data, uint32_t size, void *ctx),
        void *ctx);

/**
 * @brief DATA bölgesini tamamen temizle (sector sil + başlık yeniden yaz).
 *
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmClearData(void);

/* =========================================================================
 * Tanılama
 * ====================================================================== */

/**
 * @brief Her bölgedeki boş alan miktarını döndür.
 *
 * İhtiyaç duyulmayan değerler için NULL geçilebilir.
 *
 * @param cfgFree   CONFIG bölgesindeki boş byte sayısı.
 * @param logFree   LOG bölgesindeki boş byte sayısı.
 * @param dataFree  DATA bölgesindeki boş byte sayısı.
 * @return NO_ERROR veya hata kodu.
 */
error_t flashNewStmGetFreeSpace(uint32_t *cfgFree,
                                 uint32_t *logFree,
                                 uint32_t *dataFree);

#ifdef __cplusplus
}
#endif

#endif /* FS_FLASH_NEW_STM_H */
