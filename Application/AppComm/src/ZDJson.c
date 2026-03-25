/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
* #Date         : 25 Mar 2026 - 23:46:00
* #File Name    : ZDJson.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/

/********************************* INCLUDES ***********************************/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "ZDJson.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/
static bool append(ZDJson_Builder_t *jb, const char *str)
{
    size_t len = strlen(str);

    if (jb->pos + len >= jb->size)
    {
        return false;
    }

    memcpy(&jb->buffer[jb->pos], str, len);
    jb->pos += len;
    jb->buffer[jb->pos] = '\0';

    return true;
}

static bool appendFormat(ZDJson_Builder_t *jb, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(&jb->buffer[jb->pos], jb->size - jb->pos, fmt, args);
    va_end(args);

    if (written <= 0 || (jb->pos + written >= jb->size))
        return false;

    jb->pos += written;
    return true;
}

static bool addKey(ZDJson_Builder_t *jb, const char *key)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }
    jb->needComma[jb->depth] = true;

    if (key != NULL)
    {
        return appendFormat(jb, "\"%s\":", key);
    }
    return true;
}

/***************************** PUBLIC FUNCTIONS  ******************************/

void ZDJson_Init(ZDJson_Builder_t *jb, char *buffer, size_t size)
{
    jb->buffer = buffer;
    jb->size = size;
    jb->pos = 0;
    jb->depth = 0;
    memset(jb->needComma, 0, sizeof(jb->needComma));
    jb->buffer[0] = '\0';
}

bool ZDJson_OpenObject(ZDJson_Builder_t *jb, const char *key)
{
    if (!addKey(jb, key)) return false;
    if (!append(jb, "{")) return false;

    jb->depth++;
    if (jb->depth >= 10) return false;

    jb->needComma[jb->depth] = false;

    return true;
}

bool ZDJson_CloseObject(ZDJson_Builder_t *jb)
{
    if (jb->depth <= 0) return false;

    if (!append(jb, "}")) return false;

    jb->depth--;
    return true;
}

bool ZDJson_AddString(ZDJson_Builder_t *jb, const char *key, const char *value)
{
    if (!addKey(jb, key)) return false;

    return appendFormat(jb, "\"%s\"", value);
}

bool ZDJson_AddNumber(ZDJson_Builder_t *jb, const char *key, int value)
{
    if (!addKey(jb, key)) return false;

    return appendFormat(jb, "%d", value);
}

bool ZDJson_AddBool(ZDJson_Builder_t *jb, const char *key, bool value)
{
    if (!addKey(jb, key)) return false;

    return append(jb, value ? "true" : "false");
}

bool ZDJson_AddNull(ZDJson_Builder_t *jb, const char *key)
{
    if (!addKey(jb, key)) return false;

    return append(jb, "null");
}

bool ZDJson_AddObject(ZDJson_Builder_t *jb, const char *rawJsonObject)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }

    jb->needComma[jb->depth] = true;
    return append(jb, rawJsonObject);
}

bool ZDJson_OpenArray(ZDJson_Builder_t *jb, const char *key)
{
    if (!addKey(jb, key)) return false;

    if (!append(jb, "[")) return false;

    jb->depth++;
    if (jb->depth >= 10) return false;

    jb->needComma[jb->depth] = false;

    return true;
}

bool ZDJson_CloseArray(ZDJson_Builder_t *jb)
{
    if (jb->depth <= 0) return false;

    if (!append(jb, "]")) return false;

    jb->depth--;
    return true;
}

bool ZDJson_AddArrayString(ZDJson_Builder_t *jb, const char *value)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }

    jb->needComma[jb->depth] = true;

    return appendFormat(jb, "\"%s\"", value);
}

bool ZDJson_AddArrayNumber(ZDJson_Builder_t *jb, int value)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }

    jb->needComma[jb->depth] = true;

    return appendFormat(jb, "%d", value);
}

bool ZDJson_AddArrayBool(ZDJson_Builder_t *jb, bool value)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }

    jb->needComma[jb->depth] = true;

    return append(jb, value ? "true" : "false");
}

bool ZDJson_AddArrayNull(ZDJson_Builder_t *jb)
{
    if (jb->needComma[jb->depth])
    {
        if (!append(jb, ",")) return false;
    }

    jb->needComma[jb->depth] = true;

    return append(jb, "null");
}

const char* ZDJson_GetString(const ZDJson_Builder_t *jb)
{
    return jb->buffer;
}


/************************************************************************/

bool ZDJson_GetStringValue(const char *jsonStr, const char *key, char *outValue, size_t outSize)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);  // Anahtar ile birlikte arama yapıyoruz.

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
    {
        return false;  // Anahtar bulunamazsa
    }

    keyPos += strlen(searchKey);  // Anahtardan sonrasına geç
    if (*keyPos != '\"')
    {
        return false;  // Eğer beklenen formatta değilse yani string değilse
    }

    keyPos++;  // " karakterinden sonrasına geç
    const char *endPos = strchr(keyPos, '\"');

    if (endPos == NULL)
    {
        return false;  // Stringin sonu bulunamazsa
    }

    size_t valueLength = (size_t)(endPos - keyPos);
    if (valueLength >= outSize)
    {
        return false;  // Eğer çıktı için yeterli yer yoksa
    }

    strncpy(outValue, keyPos, valueLength);
    outValue[valueLength] = '\0';

    return true;
}

bool ZDJson_GetNumberValue(const char *jsonStr, const char *key, int *outValue)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
    {
        return false;  // Anahtar bulunamazsa
    }

    keyPos += strlen(searchKey);  // Anahtardan sonrasına geç
    while (*keyPos == ' ' || *keyPos == '\"')
    {
        keyPos++;  // Gereksiz boşlukları ve çift tırnakları geç
    }

    if (sscanf(keyPos, "%d", outValue) != 1)
    {
        return false;  // Sayı okuma başarısız olduysa
    }

    return true;
}

bool ZDJson_GetBoolValue(const char *jsonStr, const char *key, bool *outValue)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
    {
        return false;  // Anahtar bulunamazsa
    }

    keyPos += strlen(searchKey);  // Anahtardan sonrası
    while (*keyPos == ' ' || *keyPos == '\"')
    {
        keyPos++;  // Gereksiz boşlukları ve çift tırnakları geç
    }

    if (strncmp(keyPos, "true", 4) == 0)
    {
        *outValue = true;
        return true;
    }
    else if (strncmp(keyPos, "false", 5) == 0)
    {
        *outValue = false;
        return true;
    }

    return false;
}

bool ZDJson_GetNullValue(const char *jsonStr, const char *key)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
    {
        return false;  // Anahtar bulunamazsa
    }

    keyPos += strlen(searchKey);  // Anahtardan sonrası
    while (*keyPos == ' ' || *keyPos == '\"')
    {
        keyPos++;  // Gereksiz boşluklar ve çift tırnaklar
    }

    if (strncmp(keyPos, "null", 4) == 0)
    {
        return true;
    }

    return false;  // Null değilse
}

bool ZDJson_GetArrayvalue(const char *jsonStr, const char *key, int index, char *outValue, size_t outSize)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
    {
        return false;  // Anahtar bulunamazsa
    }

    keyPos += strlen(searchKey);  // Anahtardan sonrası
    if (*keyPos != '[')
    {
        return false;  // Eğer array değilse
    }

    keyPos++;  // Array başlatıcıyı geç
    for (int i = 0; i < index; i++)
    {
        keyPos = strchr(keyPos, ',');
        if (keyPos == NULL)
        {
            return false;  // Eğer dizinin sonu erken gelirse
        }
        keyPos++;  // Virgülü geç
    }

    // Şu anki öğeyi al
    const char *endPos = strchr(keyPos, ',');
    if (endPos == NULL)
    {
        endPos = strchr(keyPos, ']');  // Dizinin sonuna kadar
    }

    size_t valueLength = (size_t)(endPos - keyPos);
    if ((0 == valueLength) || (valueLength >= outSize))
    {
        return false;  // Eğer çıktı için yeterli yer yoksa yada çıktı boşsa
    }

    strncpy(outValue, keyPos, valueLength);
    outValue[valueLength] = '\0';
    return true;
}

bool ZDJson_GetArrayObject(const char *jsonStr, const char *key, int index, char *outValue, size_t outSize)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
        return false;  // Anahtar bulunamadı

    const char *arrayStart = strchr(keyPos, '[');
    if (arrayStart == NULL)
        return false;  // Array yok

    arrayStart++;  // '[' sonrası
    const char *arrayEnd = strchr(arrayStart, ']');

    if (arrayEnd == NULL) return false;  // Array düzgün kapanmamış

    const char *current = arrayStart;

    for (int i = 0; i <= index; i++)
    {
        // Objeye kadar ilerle
        while (current < arrayEnd && *current != '{') current++;
        if (current >= arrayEnd || *current != '{') return false; // İstenen index yok

        if (i == index)
        {
            // Burada istediğimiz objedeyiz
            const char *objStart = current;
            int braceCount = 0;
            const char *p = objStart;

            do {
                if (*p == '{') braceCount++;
                else if (*p == '}') braceCount--;
                p++;

                if (p >= arrayEnd && braceCount > 0)
                    return false; // Obje array bitmeden kapanmadı
            } while (braceCount > 0);

            size_t objLen = (size_t)(p - objStart);
            if (objLen >= outSize) return false;

            strncpy(outValue, objStart, objLen);
            outValue[objLen] = '\0';
            return true;
        }

        // Bu objeyi geçiyoruz
        int braceCount = 0;
        do {
            if (*current == '{') braceCount++;
            else if (*current == '}') braceCount--;
            current++;
        } while (current < arrayEnd && braceCount > 0);

        // Virgül ve boşlukları geç
        while (current < arrayEnd && (*current == ',' || *current == ' ' || *current == '\n' || *current == '\r'))
            current++;
    }

    return false; // Bu satıra normalde gelinmez ama güvenlik için
}

bool ZDJson_GetObjectValueAddress(const char *jsonStr, const char *key, const char **objStartPointer, size_t *objSize)
{
    if (!jsonStr || !key || !objStartPointer || !objSize)
        return false;

    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
        return false;

    keyPos += strlen(searchKey);

    while (*keyPos == ' ' || *keyPos == '\n' || *keyPos == '\t') keyPos++;

    const char *startContent = keyPos;
    const char *endPos = startContent;
    char openChar = *keyPos;
    char closeChar = 0;

    if (openChar == '{') closeChar = '}';
    else if (openChar == '[') closeChar = ']';
    else if (openChar == '"')
    {

        startContent++;
        endPos++;
        while (*endPos) {
            if (*endPos == '"' && *(endPos - 1) != '\\')
                break;
            endPos++;
        }
        if (*endPos != '"')
            return false;
        *objStartPointer = startContent;
        *objSize = (size_t)(endPos - startContent);
        return true;
    }
    else
    {
        // number, boolean or null
        while (*endPos && *endPos != ',' && *endPos != '}' && *endPos != ']')
            endPos++;
        *objStartPointer = startContent;
        *objSize = (size_t)(endPos - startContent);
        return true;
    }

    // Obje or array
    int depth = 0;
    bool inString = false;

    while (*endPos)
    {
        if (*endPos == '"' && *(endPos - 1) != '\\') {
            inString = !inString;
        } else if (!inString) {
            if (*endPos == openChar) depth++;
            else if (*endPos == closeChar) {
                depth--;
                if (depth == 0) {
                    endPos++; // include closing char
                    break;
                }
            }
        }
        endPos++;
    }

    if (depth != 0)
        return false;

    *objStartPointer = startContent;
    *objSize = (size_t)(endPos - startContent);

    return true;
}

bool ZDJson_GetArrayObjectAddress(const char *jsonStr, const char *key, int index, const char **outStart, size_t *outLength)
{
    if (!jsonStr || !key || !outStart || !outLength || index < 0)
        return false;

    // 1. "key" aramasını yap
    char searchKey[128];
    snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (!keyPos) return false;

    // 2. ':' sonrası '[' karakterini bul
    const char *p = strchr(keyPos, '[');
    if (!p) return false;

    p++; // '[' sonrası ilk karaktere ilerle

    int currentIndex = 0;
    while (*p && *p != ']') {

        if (*p != '{')
            return false; // JSON format hatası

        // 3. Objeyi başlat
        const char *objStart = p;
        int braceCount = 0;
        bool inString = false;

        // 4. Objeyi sonuna kadar tara
        while (*p) {
            if (*p == '"' && *(p - 1) != '\\') {
                inString = !inString;
            } else if (!inString) {
                if (*p == '{') braceCount++;
                else if (*p == '}') braceCount--;

                if (braceCount == 0) {
                    p++; // objenin sonundan sonrasına geç
                    break;
                }
            }
            p++;
        }

        if (braceCount != 0)
            return false;

        if (currentIndex == index) {
            *outStart = objStart;
            *outLength = (size_t)(p - objStart);
            return true;
        }

        currentIndex++;

        // Objeden sonra virgül varsa atla
        if (*p == ',')
            p++;
    }

    return false; // index bulunamadı
}

bool ZDJson_GetObjectValue(const char *jsonStr, const char *key, char *outValue, size_t outSize)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (keyPos == NULL)
        return false;

    keyPos += strlen(searchKey);

    while (*keyPos == ' ' || *keyPos == '\n' || *keyPos == '\t') keyPos++;

    char openChar = *keyPos;
    char closeChar;

    if (openChar == '{') closeChar = '}';
    else if (openChar == '[') closeChar = ']';
    else return false;

    int depth = 0;
    const char *startContent = keyPos + 1;
    const char *endPos = startContent;

    while (*endPos)
    {
        if (*endPos == openChar) depth++;
        else if (*endPos == closeChar)
        {
            if (depth == 0) break;
            depth--;
        }
        endPos++;
    }

    if (*endPos != closeChar)
        return false;

    size_t valueLength = (size_t)(endPos - startContent);
    if (valueLength >= outSize)
        return false;

    strncpy(outValue, startContent, valueLength);
    outValue[valueLength] = '\0';

    return true;
}

bool ZDJson_IsKeyAvailable(const char *jsonStr, const char *key)
{
    char searchKey[MAX_KEY_LENGTH];
    snprintf(searchKey, MAX_KEY_LENGTH, "\"%s\":", key);

    const char *keyPos = strstr(jsonStr, searchKey);
    if (NULL != keyPos)
    {
        return true;
    }

    return false;  // Anahtar bulunamazsa
}
bool ZDJson_IsValid(const char *json)
{
    int braces = 0;
    int brackets = 0;
    bool in_string = false;
    bool escape = false;

    while (*json)
    {
        char c = *json++;

        if (escape)
        {
            escape = false;
            continue;
        }

        if (c == '\\')
        {
            escape = true;
            continue;
        }

        if (c == '"')
        {
            in_string = !in_string;
            continue;
        }

        if (!in_string)
        {
            if (c == '{') braces++;
            else if (c == '}') braces--;
            else if (c == '[') brackets++;
            else if (c == ']') brackets--;

            if (braces < 0 || brackets < 0) {
                return false;  // Fazladan kapama var
            }
        }
    }

    return (braces == 0 && brackets == 0 && !in_string);
}

/******************************** End Of File *********************************/

