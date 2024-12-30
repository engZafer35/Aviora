/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Dec 30, 2024 - 9:38:38 AM
* #File Name    : DrvLinuxUartGeneric.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __DRV_LINUX_UART_GENERIC_H__
#define __DRV_LINUX_UART_GENERIC_H__
/*********************************INCLUDES*************************************/
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
/******************************MACRO DEFINITIONS*******************************/

/*******************************TYPE DEFINITIONS ******************************/
struct UartDevice
{
    char* filename;
    int rate;

    int fd;
    struct termios *tty;
};
/************************* GLOBAL VARIBALE REFERENCES *************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

int uart_start(struct UartDevice* dev, unsigned int canonic);
int uart_writen(struct UartDevice* dev, char *buf, size_t buf_len);
int uart_writes(struct UartDevice* dev, char *string);
int uart_reads(struct UartDevice* dev, char *buf, size_t buf_len);
int set_baudrate(struct UartDevice* dev, unsigned int speed);
void uart_stop(struct UartDevice* dev);

#endif /* __DRV_LINUX_UART_GENERIC_H__ */

/********************************* End Of File ********************************/
