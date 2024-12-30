/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Dec 30, 2024 - 9:38:46 AM
* #File Name    : DrvLinuxUartGeneric.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>

#include "DrvLinuxUartGeneric.h"
/****************************** MACRO DEFINITIONS *****************************/

/******************************* TYPE DEFINITIONS *****************************/

/********************************** VARIABLES *********************************/

/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/
/*
 * Start the UART device.
 *
 * @param dev points to the UART device to be started, must have filename and rate populated
 * @param canonical whether to define some compatibility flags for a canonical interface
 *
 * @return - 0 if the starting procedure succeeded
 *         - negative if the starting procedure failed
 */
int uart_start(struct UartDevice* dev, unsigned int canonical)
{
    struct termios *tty;
    int fd;
    int rc;

    fd = open(dev->filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        printf("%s: failed to open UART device\r\n", __func__);
        return fd;
    }

    tty = malloc(sizeof(*tty));
    if (!tty) {
        printf("%s: failed to allocate UART TTY instance\r\n", __func__);
        return -ENOMEM;
    }

    memset(tty, 0, sizeof(*tty));

    /*
     * Set baud-rate.
     */

    tty->c_cflag |= dev->rate;

    /* Ignore framing and parity errors in input. */
    tty->c_iflag |=  IGNPAR;

    /* Use 8-bit characters. This too may affect standard streams,
     * but any sane C library can deal with 8-bit characters. */

    tty->c_cflag |=  CS7;//CS8;

    tty->c_cflag |= PARENB;     // Pariteyi etkinleştir
    tty->c_cflag &= ~PARODD;    // Even pariteyi seç

    /* Enable receiver. */
    tty->c_cflag |=  CREAD;

    if (canonical) {
        /* Enable canonical mode.
         * This is the most important bit, as it enables line buffering etc. */
        tty->c_lflag |= ICANON;
    } else {
        /* To maintain best compatibility with normal behaviour of terminals,
         * we set TIME=0 and MAX=1 in noncanonical mode. This means that
         * read() will block until at least one byte is available. */
        tty->c_cc[VTIME] = 0;
        tty->c_cc[VMIN] = 1;
    }

    /*
     * Flush port.
     */
    tcflush(fd, TCIFLUSH);

    /*
     * Apply attributes.
     */
    rc = tcsetattr(fd, TCSANOW, tty);
    if (rc) {
        return rc;
    }

    dev->fd = fd;
    dev->tty = tty;

    return 0;
}

/*
 * Read a string from the UART device.
 *
 * @param dev points to the UART device to be read from
 * @param buf points to the start of buffer to be read into
 * @param buf_len length of the buffer to be read
 *
 * @return - number of bytes read if the read procedure succeeded
 *         - negative if the read procedure failed
 */
int uart_reads(struct UartDevice* dev, char *buf, size_t buf_len)
{
    int rc;

    rc = read(dev->fd, buf, buf_len - 1);
    if (rc < 0) {
        return rc;
    }

    buf[rc] = '\0';
    return rc;
}

/*
 * Write data to the UART device.
 *
 * @param dev points to the UART device to be written to
 * @param buf points to the start of buffer to be written from
 * @param buf_len length of the buffer to be written
 *
 * @return - number of bytes written if the write procedure succeeded
 *         - negative if the write procedure failed
 */
int uart_writen(struct UartDevice* dev, char *buf, size_t buf_len)
{
    return write(dev->fd, buf, buf_len);
}

/*
 * Write a string to the UART device.
 *
 * @param dev points to the UART device to be written to
 * @param string points to the start of buffer to be written from
 *
 * @return - number of bytes written if the write procedure succeeded
 *         - negative if the write procedure failed
 */
int uart_writes(struct UartDevice* dev, char *string)
{
    size_t len = strlen(string);
    return uart_writen(dev, string, len);
}

int set_baudrate(struct UartDevice* dev, unsigned int speed)
{
    struct termios tty;
    int rc1, rc2;

    if (tcgetattr(dev->fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }
    rc1 = cfsetospeed(&tty, speed);
    rc2 = cfsetispeed(&tty, speed);
    if ((rc1 | rc2) != 0 ) {
        printf("Error from cfsetxspeed: %s\n", strerror(errno));
        return -1;
    }
    if (tcsetattr(dev->fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    tcflush(dev->fd, TCIOFLUSH);  /* discard buffers */

    return 0;
}


/*
 * Stop the UART device.
 *
 * @param dev points to the UART device to be stopped
 */
void uart_stop(struct UartDevice* dev)
{
    free(dev->tty);
    close(dev->fd);
}
/******************************** End Of File *********************************/
