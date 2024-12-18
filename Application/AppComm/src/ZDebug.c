/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 16, 2024 - 8:43:17 AM
* #File Name    : ZDebug.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "ZDebug.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
void debugArray(const char *prepend, const void *data, unsigned int length)
{
   unsigned int i;

   fprintf(stdout, "%s\n", prepend);

   for(i = 0; i < length; i++)
   {
      //Display current data byte
      fprintf(stdout, "%02X " , *((const unsigned char *) data + i));

      //End of current line?
      if((i % 16) == 15 || i == (length - 1))
      {
          fprintf(stdout, "\n");
      }
   }
}
/******************************** End Of File *********************************/
