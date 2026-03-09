/******************************************************************************
* #Author       : Zafer Satilmis
* #Revision     : 1.0
* #Date         : Jun 8, 2024 - 6:12:19 PM
* #File Name    : uart_driver.c
*******************************************************************************/
/******************************************************************************
*
*
*******************************************************************************/
#define SHOW_PAGE_DBG_MSG  (DISABLE)
/********************************* INCLUDES ***********************************/
#include "net_config.h"
#if USE_CYCLONE_LIB == 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "core/net.h"
#include "ppp/ppp_hdlc.h"
#include "uart_driver.h"
#include "debug.h"
/****************************** MACRO DEFINITIONS *****************************/

#define UART_FAILURE -1
#define UART_SUCCESS 0

/******************************* TYPE DEFINITIONS *****************************/
struct UartDevice {
    char* name;
    int rate;

    int fd;
    struct termios *tty;
};
/********************************** VARIABLES *********************************/
static struct UartDevice uartDev;
static pthread_t gs_uartRx, gs_uartTx;
volatile unsigned int rxReady;

static NetInterface *gs_interface;

// Declaration of thread condition variable
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
// declaring mutex
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/***************************** STATIC FUNCTIONS  ******************************/

/***************************** PUBLIC FUNCTIONS  ******************************/

//UART baudrate
#define UART_DEV_NAME "/dev/ttyUSB0"
#define APP_UART_BAUDRATE 115200
//Hardware flow control
#define APP_UART_HW_FLOW_CTRL DISABLED


/**
 * @brief UART driver
 **/

const UartDriver uartDriver =
{
   uartInit,
   uartEnableIrq,
   uartDisableIrq,
   uartStartTx
};

static int uart_writen(struct UartDevice* dev, uint8_t *buf, size_t buf_len)
{
    int ret;
    ret = write(dev->fd, buf, buf_len);
    return ret;
}

int uart_reads(struct UartDevice* dev, uint8_t *buf, size_t bufLen)
{
    return read(dev->fd, buf, bufLen);
}

/**
 * @brief FLEXCOM0 interrupt handler
 **/

void uartTxFunc(void *p)
{
    int_t  c=0;

    pthread_mutex_lock(&lock); //mutex lock
    while(1)
    {
        pthread_cond_wait(&cond, &lock); //wait for the condition
        while (c != EOF)
        {
            //Get next character
            pppHdlcDriverReadTxQueue(gs_interface, &c);
            //Valid character read?
            if(c != EOF)
            {
                //Send data byte
                uart_writen(&uartDev, (uint8_t*)(&c), sizeof(char));
            }
        }
        c=0;

        pthread_mutex_unlock(&lock);
    }
}

static void uartRxFunc(void *p)
{
    fd_set read_fd;
    int    ret;

    while (1)
    {
        /* Initialize the file descriptor set. */
        FD_ZERO( &read_fd );
        FD_SET(uartDev.fd, &read_fd);

        ret = select(FD_SETSIZE, &read_fd, NULL, NULL, NULL);

        if( ret < 0 )
        {
            printf("select error !!");
            continue;
        }

        if( FD_ISSET(uartDev.fd, &read_fd))
        {
            uint8_t rxBuff[PPP_RX_BUFFER_SIZE] = {0,};
            int rc;
            rc = uart_reads(&uartDev, rxBuff, sizeof(rxBuff));

            for (int i= 0; i < rc; i++)
            {
                pppHdlcDriverWriteRxQueue(gs_interface, rxBuff[i]);
            }
        }
    }
}

/**
 * @brief UART configuration
 * @return Error code
 **/

error_t uartInit(void)
{
    //Debug message
    TRACE_INFO("Initializing UART...\r\n");
    struct UartDevice* dev = &uartDev;
    unsigned char canonical = 0;

    struct termios *tty;
    int fd;
    int rc;

    gs_interface = &netInterface[0];

    dev->name = UART_DEV_NAME;
    dev->rate = APP_UART_BAUDRATE;

    fd = open(UART_DEV_NAME, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        printf("%s: failed to open UART device\r\n", __func__);
        return ERROR_FAILURE;
    }

    tty = malloc(sizeof(*tty));
    if (!tty)
    {
        printf("%s: failed to allocate UART TTY instance\r\n", __func__);
        return ERROR_FAILURE;
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
    tty->c_cflag |=  CS8;

    /* Enable receiver. */
    tty->c_cflag |=  CREAD;

    if (canonical)
    {
        /* Enable canonical mode.
         * This is the most important bit, as it enables line buffering etc. */
        tty->c_lflag |= ICANON;
    }
    else
    {
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
    if (rc)
    {
       printf("%s: failed to set attributes\r\n", __func__);
       return ERROR_FAILURE;
    }

    dev->fd = fd;
    dev->tty = tty;

    uart_writen(&uartDev, "ATE0\r", sizeof("ATE0")+1);
    uart_writen(&uartDev, "AT+IPR=921600\r", sizeof("AT+IPR=921600")+1);
    sleep(1);
    //Create a new thread
    if(0 != pthread_create(&gs_uartRx, NULL, uartRxFunc, NULL))
    {
        gs_uartRx = OS_INVALID_TASK_ID;
        return ERROR_FAILURE;
    }

    //Create a new thread
    if(0 != pthread_create(&gs_uartTx, NULL, uartTxFunc, NULL))
    {
        gs_uartTx = OS_INVALID_TASK_ID;
        return ERROR_FAILURE;
    }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Enable UART interrupts
 **/

void uartEnableIrq(void)
{
   //Enable FLEXCOM0 interrupts
//   NVIC_EnableIRQ(FLEXCOM0_IRQn);
}


/**
 * @brief Disable UART interrupts
 **/

void uartDisableIrq(void)
{
   //Disable FLEXCOM0 interrupt
//   NVIC_DisableIRQ(FLEXCOM0_IRQn);
}


/**
 * @brief Start transmission
 **/
void uartStartTx(void)
{
    pthread_cond_signal(&cond);
}
#endif

void uartDrvSendDma(int line, void *buff, int leng)
{
//    uart_writen(NULL, buff, leng);

    uartTxDmaHalfCompleted();
}
/******************************** End Of File *********************************/
