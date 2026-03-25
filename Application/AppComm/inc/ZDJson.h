/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.3
* #Date         : 25 Mar 2026 - 23:46:00
* #File Name    : ZDJson.h
*******************************************************************************/
/******************************************************************************
* This file is a part of the ZDJson library. You can use this library to create 
* and parse JSON data.Some data types are not supported by this library
*  like float, double, etc.  ZDJson_GetObjectValueAddress and
* ZDJson_GetArrayObjectAddress returns the address of the value and the 
* length of the value.
* Example usage:
* ZDJson_Builder_t jb;
* ZDJson_Init(&jb, buffer, sizeof(buffer));
* ZDJson_OpenObject(&jb, "root");
* ZDJson_AddString(&jb, "key", "value");
* ZDJson_CloseObject(&jb);
* printf("%s\n", jb.buffer);

* Getter example:
* char buffer[] = "{\"key\":\"value\",\"key2\":10,\"key3\":true,\"key4\":null}";
*
* Getter functions:
* char value[100];
* ZDJson_GetStringValue(buffer, "key", value, sizeof(value));
* printf("%s\n", value);
* 
* int numberValue;
* ZDJson_GetNumberValue(buffer, "key2", &numberValue);
* printf("%d\n", numberValue);
* 
* bool boolValue;
* ZDJson_GetBoolValue(buffer, "key3", &boolValue);
* printf("%d\n", boolValue);
* 
* bool nullValue;
* ZDJson_GetNullValue(buffer, "key4");
* printf("%d\n", nullValue);
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __ZDJSON_H__
#define __ZDJSON_H__

/*********************************INCLUDES*************************************/
#include <stddef.h>
#include <stdbool.h>

/******************************MACRO DEFINITIONS*******************************/
#define MAX_KEY_LENGTH  (64)
#define MAX_VALUE_LENGT (256)
#define MAX_DEPTH       (10)

/*******************************TYPE DEFINITIONS ******************************/
typedef struct {
    char *buffer;
    size_t size;
    size_t pos;
    int depth;
    bool needComma[MAX_DEPTH + 1];    // +1 for root object
} ZDJson_Builder_t;

/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/
void ZDJson_Init(ZDJson_Builder_t *jb, char *buffer, size_t size);

bool ZDJson_OpenObject(ZDJson_Builder_t *jb, const char *key);   // key == NULL: root objesi veya array içinde obje
bool ZDJson_CloseObject(ZDJson_Builder_t *jb);

bool ZDJson_AddString(ZDJson_Builder_t *jb, const char *key, const char *value);
bool ZDJson_AddNumber(ZDJson_Builder_t *jb, const char *key, int value);
bool ZDJson_AddBool(ZDJson_Builder_t *jb, const char *key, bool value);
bool ZDJson_AddNull(ZDJson_Builder_t *jb, const char *key);
bool ZDJson_AddObject(ZDJson_Builder_t *jb, const char *rawJsonObject);

bool ZDJson_OpenArray(ZDJson_Builder_t *jb, const char *key);    // key == NULL: array içinde array olabilir
bool ZDJson_CloseArray(ZDJson_Builder_t *jb);

bool ZDJson_AddArrayString(ZDJson_Builder_t *jb, const char *value);
bool ZDJson_AddArrayNumber(ZDJson_Builder_t *jb, int value);
bool ZDJson_AddArrayBool(ZDJson_Builder_t *jb, bool value);
bool ZDJson_AddArrayNull(ZDJson_Builder_t *jb);

const char* ZDJson_GetString(const ZDJson_Builder_t *jb);

bool ZDJson_IsValid(const char *json);
bool ZDJson_IsKeyAvailable(const char *jsonStr, const char *key);
/*************************************************************************/
bool ZDJson_GetStringValue(const char *jsonStr, const char *key, char *outValue, size_t out_size);
bool ZDJson_GetNumberValue(const char *jsonStr, const char *key, int *outValue);
bool ZDJson_GetBoolValue(const char *jsonStr, const char *key, bool *outValue);
bool ZDJson_GetNullValue(const char *jsonStr, const char *key);
bool ZDJson_GetArrayvalue(const char *jsonStr, const char *key, int index, char *outValue, size_t out_size);
bool ZDJson_GetArrayObject(const char *jsonStr, const char *key, int index, char *outValue, size_t outSize);
bool ZDJson_GetObjectValue(const char *jsonStr, const char *key, char *outValue, size_t out_size);

bool ZDJson_GetObjectValueAddress(const char *jsonStr, const char *key, const char **objStartPointer, size_t *objSize);
bool ZDJson_GetArrayObjectAddress(const char *jsonStr, const char *key, int index, const char **outStart, size_t *outLength);

/*************************************************************************/

#endif // __ZDJSON_H__

/********************************* End Of File ********************************/
