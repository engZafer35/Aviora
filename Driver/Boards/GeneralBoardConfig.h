/******************************************************************************
* #Author       : Zafer Satılmış
* #Revision     : 1.0
* #Date         : Jan 27, 2021 - 9:05:66 AM
* #File Name    : GeneralBoardConfig.h
* #File Path    : /Driver/Board/GeneralBoardConfig.h
*******************************************************************************/

/******************************************************************************
* 
******************************************************************************/
/******************************IFNDEF & DEFINE********************************/
#ifndef __GENERAL_BOARD_CONFIG_H__
#define __GENERAL_BOARD_CONFIG_H__
/*********************************INCLUDES*************************************/
#define BOARD_LINUX_PC      (1)
#define BOARD_STM_010101    (2)
#define BOARD_STM_LP_010101 (3)
#define BOARD_WINDOWS_PC    (4)
#define BOARD_RA4M4_010101  (5)


/* add new board, dont change queue*/

#ifdef __linux
    #define CURRENT_BOARD   (BOARD_LINUX_PC)
#elif _WIN64
    #define CURRENT_BOARD   (BOARD_WINDOWS_PC)
#elif
    #define CURRENT_BOARD   (BOARD_RA4M4_010101) //select current embedded board here
#endif

#if (CURRENT_BOARD == BOARD_LINUX_PC)
    #include "BoardConfig_LinuxPC.h"
#elif (CURRENT_BOARD == BOARD_WINDOWS_PC)
    #include "BoardConfig_WindowsPC.h"
#elif (CURRENT_BOARD == BOARD_STM_010101)
    #include "BoardConfig_STM_010101.h"
#elif (CURRENT_BOARD == BOARD_STM_LP_010101)
    #include "BoardConfig_STM_LP_010101.h"
#else
    #error "!!! Current board is undefined. Check GeneralBoardConfig.h file !!!"
#endif

/******************************MACRO DEFINITIONS*******************************/
#define MCU_BIT_SIZE            (_MCU_BIT_SIZE)

typedef _DRV_RET_TYPE           DRV_RET_TYPE;
#define DRV_RET_SUCCESS         _DRV_RET_OK
#define DRV_RET_FAILURE         _DRV_RET_ERROR
#define DRV_RET_BUSY            _DRV_RET_BUSY
#define DRV_RET_TIMEOUT         _DRV_RET_TIMEOUT

/** MC all interrupt list */
typedef _EN_INTERRUPT_LIST EN_INTERRUPT_LIST;

#define OS_SUPPORT_STATIC_ALLOCATION (_OS_SUPPORT_STATIC_ALLOCATION)
/****************** BOARD MCU CLOCK CONTROL *******************/

#define MCU_CORE_INIT()         _MCU_CORE_INIT()
#define CONF_MCU_CLOCK()        _CONF_MCU_CLOCK()
#define CONF_PRIP_CLOCK()       _CONF_PRIP_CLOCK()

#define DELAY_MS(x)             _DELAY_MS(x)

/****************** BOARD GPI0 CONTROL *******************/
#define GPIO_INIT()                     _GPIO_INIT() //init all gpio

#define WRITE_GPIO(port, pin, status)   _WRITE_GPIO(port, pin, status)
#define GPIO_READ(port, pin)            _GPIO_READ(port, pin)
#define GPIO_TOGGLE(port, pin)          _GPIO_TOGGLE(port, pin)
//
#define POWER_STATUS_LED(x)             _PWR_STAT_LED(x)
#define JOB_STATUS_LED(x)               _JOB_STAT_LED(x)

#define GSM_LEVEL_LED_1(x)             _GSM_LEVEL_LED_1(x)
#define GSM_LEVEL_LED_2(x)             _GSM_LEVEL_LED_2(x)
#define GSM_LEVEL_LED_3(x)             _GSM_LEVEL_LED_3(x)
#define GSM_LEVEL_LED_4(x)             _GSM_LEVEL_LED_4(x)
#define GSM_LEVEL_LED_5(x)             _GSM_LEVEL_LED_5(x)

#define GSM_CONN_LED(x)                _GSM_CONN_LED(x)
#define GSM_INTERNET_LED(x)            _GSM_INTERNET_LED(x)

//
#define TOGGLE_POWER_STATUS_LED()      _TOGGLE_POWER_STATUS_LED()
#define TOGGLE_JOB_STATUS_LED()        _TOGGLE_JOB_STATUS_LED()

#define TOGGLE_GSM_LEVEL_LED_1()      _TOGGLE_GSM_LEVEL_LED_1()
#define TOGGLE_GSM_LEVEL_LED_2()      _TOGGLE_GSM_LEVEL_LED_2()
#define TOGGLE_GSM_LEVEL_LED_3()      _TOGGLE_GSM_LEVEL_LED_3()
#define TOGGLE_GSM_LEVEL_LED_4()      _TOGGLE_GSM_LEVEL_LED_4()
#define TOGGLE_GSM_LEVEL_LED_5()      _TOGGLE_GSM_LEVEL_LED_5()

#define TOGGLE_GSM_CONN()             _TOGGLE_GSM_CONN()
#define TOGGLE_GSM_INTERNET()         _TOGGLE_GSM_INTERNET()

#define TOGGLE_GSM_PWR_LED()          _TOGGLE_GSM_PWR_LED()
#define TOGGLE_GSM_SCOM_PWR_LED()     _TOGGLE_GSM_SCOM_PWR_LED()
//
#define READ_AC_INPUT()               _READ_AC_INPUT()
#define READ_SCAP_INPUT()             _READ_SCAP_INPUT()

//
#define INPUT_AC_PIN                  _INPUT_AC_PIN
#define INPUT_SCAP_PIN                _INPUT_SCAP_PIN

#define GPIO_AC_INT_CHNL              _GPIO_AC_INT_CHNL
#define GPIO_SCAP_INT_CHNL            _GPIO_SCAP_INT_CHNL

#define IS_INPUT_AC_PIN(x)            _IS_INPUT_AC_PIN(x)
#define IS_INPUT_SCAP_PIN(x)          _IS_INPUT_SCAP_PIN(x)

#define CORE_EXT_IT_FUNCTION          _CORE_EXT_IT_FUNCTION

#define GPIO_USER_INPUT_IRQ_CHNL      _GPIO_USER_INPUT_IRQ_CHNL
#define GPIO_USER2_INPUT_IRQ_CHNL     _GPIO_USER2_INPUT_IRQ_CHNL

/****************** BOARD I2C CONTROL *******************/
#define I2C1_INIT()                                 _I2C1_INIT()
#define I2C2_INIT()                                 _I2C2_INIT()

#define I2C1_LINE                                   _I2C1_LINE
#define I2C2_LINE                                   _I2C2_LINE

#define TCA9555_I2C_ADDR                            _TCA9555_I2C_ADDR
#define LM75B_I2C_ADDR                              _LM75B_I2C_ADDR

#define I2C_SEND(line, devAddr, buff, leng)         _I2C_SEND(line, devAddr, buff, leng)
#define I2C_RCV(line, devAddr, buff, leng)          _I2C_RCV(line, devAddr, buff, leng)

#define I2C_WRITE(line, devAddr, addr, buff, leng)  _I2C_WRITE(line, devAddr, addr, buff, leng)
#define I2C_READ(line, devAddr, addr, buff, leng)   _I2C_READ(line, devAddr, addr, buff, leng)

/****************** BOARD UART CONTROL *******************/
#define UART_DEBUG_INIT()                           _UART_DEBUG_INIT()
#define UART_COMM_INIT()                            _UART_COMM_INIT()

#define UART_LINE_OBJ_TYPE                          _UART_LINE_OBJ_TYPE
#define UART_LINE_1                                 _UART_COMM_LINE_1
#define UART_LINE_2                                 _UART_COMM_LINE_2
#define UART_LINE_3                                 _UART_DEBUG_LINE
#define UART_LINE_6                                 _UART_DEBUG_LINE

#define LINE_DBG_RX_IT_ID                           _LINE_DBG_RX_IT_ID
#define LINE_COMM_1_RX_IT_ID                        _LINE_COMM_1_RX_IT_ID
#define LINE_COMM_2_RX_IT_ID                        _LINE_COMM_2_RX_IT_ID

#define UART_RCV_IT_FUNCTION                        _UART_RCV_IT_FUNCTION

#define IS_DBG_UART_IT()                            _IS_DBG_UART_IT()
#define IS_COMM_1_UART_IT()                         _IS_COMM_1_UART_IT()
#define IS_COMM_2_UART_IT()                         _IS_COMM_2_UART_IT()

#define UART_RX_IT_DISABLE(line)                    _UART_RX_IT_DISABLE(line)
#define UART_RX_IT_ENABLE(line)                     _UART_RX_IT_ENABLE(line)
#define UART_RCV_IT(line, pBuff, leng)              _UART_RCV_IT(line, pBuff, leng)
#define UART_CLR_IT_FLAG(line, flag)                _UART_CLR_IT_FLAG(line, flag)
#define UART_SEND(line, buff, leng, timeout)        _UART_SEND(line, buff, leng, timeout)
#define UART_CLR_RX_INT_FLAG(line)                  _UART_CLR_IT_FLAG(line, UART_IT_RXNE)

#define IS_LINE1_UART_IT()                          IS_COMM_1_UART_IT()
#define IS_LINE2_UART_IT()                          IS_COMM_2_UART_IT()
#define IS_LINE6_UART_IT()                          IS_DBG_UART_IT()

#define LINE_UART1_RX_IT_ID                         LINE_COMM_1_RX_IT_ID
#define LINE_UART2_RX_IT_ID                         LINE_COMM_2_RX_IT_ID
#define LINE_UART3_RX_IT_ID                         ******
#define LINE_UART4_RX_IT_ID                         ******
#define LINE_UART5_RX_IT_ID                         ******
#define LINE_UART6_RX_IT_ID                         LINE_DBG_RX_IT_ID
#define LINE_UART7_RX_IT_ID                         ******
#define LINE_UART8_RX_IT_ID                         ******

#define UART_HW_LINE_1                              UART_LINE_1
#define UART_HW_LINE_2                              UART_LINE_2
#define UART_HW_LINE_3                              UART_LINE_3
#define UART_HW_LINE_4                              ******
#define UART_HW_LINE_5                              ******
#define UART_HW_LINE_6                              UART_LINE_6
#define UART_HW_LINE_7                              ******
#define UART_HW_LINE_8                              ******

#define UART_RX_IT_DISABLE(line)                    _UART_RX_IT_DISABLE(line)
#define UART_RX_IT_ENABLE(line)                     _UART_RX_IT_ENABLE(line)
#define UART_RCV_IT(line, pBuff, leng)              _UART_RCV_IT(line, pBuff, leng)
#define UART_CLR_IT_FLAG(line, flag)                _UART_CLR_IT_FLAG(line, flag)
#define UART_SEND(line, buff, leng, timeout)        _UART_SEND(line, buff, leng, timeout)
#define UART_CLR_RX_INT_FLAG(line)                  _UART_CLR_IT_FLAG(line, UART_IT_RXNE)

/****************** BOARD SPI CONTROL *******************/
#define SPI_INIT()      _SPI_INIT()

/********************* BOARD I2S CONTROL ******************/
#define I2S2_INIT()                                                _I2S2_INIT()
#define I2S3_INIT()                                                _I2S3_INIT()

#define VOICE_REC_I2S_LINE_ID                                      _VOICE_REC_I2S_LINE_ID
#define IS_VOICE_REC_I2S_LINE()                                    _IS_VOICE_REC_I2S_LINE()

#define I2S_RECIVE_DMA(line, buff, leng)                           _I2S_RECIVE_DMA(line, buff, leng)
#define I2S_REC_DMA_STOP(line)                                     _I2S_REC_DMA_STOP(line)
#define I2S_REC_DMA_PAUSE(line)                                    _I2S_REC_DMA_PAUSE(line)

#define I2S_REC_DMA_PAUSE(line)                                    _I2S_REC_DMA_PAUSE(line)
#define I2S_REC_DMA_RESUME(line)                                   _I2S_REC_DMA_RESUME(line)

#define I2S_REC_HALF_DMA_IT_FUNCTION                               _I2S_REC_HALF_DMA_IT_FUNCTION
#define I2S_REC_CPLT_DMA_IT_FUNCTION                               _I2S_REC_CPLT_DMA_IT_FUNCTION

/****************** BOARD DMA CONTROL *******************/
#define DMA_INIT()      _DMA_INIT()

/****************** BOARD TIMER CONTROL *******************/
#define TIMER_EVENT_IT_ID       _TIMER_EVENT_IT_ID
#define TIMER_ALERT_INIT()      _TIMER_ALERT_INIT()
#define TIMER_EVENT_INIT()      _TIMER_EVENT_INIT()


#define TIMER_EVENT_START()     _TIMER_EVENT_START()
#define TIMER_ALERT_START()     _TIMER_ALERT_START()

#define CORE_TIMER_IT_FUNCTION  _CORE_TIMER_IT_FUNCTION
/****************** BOARD CAN CONTROL *******************/
#define CAN_1_START()           _CAN_1_START()
#define CAN_2_START()           _CAN_2_START()
/****************** BOARD WDT CONTROL *******************/
#define WDT_INIT(x)             _WDT_INIT(x)
#define WDT_START()             _WDT_START()
#define WDT_FEED()              _WDT_FEED()

/****************** BOARD ADC CONTROL *******************/
#define ADC_TEMP_INIT()         _ADC_TEMP_INIT()


/****************** BOARD CAN CONTROL *******************/
#define CAN_INIT()                                     _CAN_INIT()

//typedef _HAL_CAN_TX_HEADER                              DRV_CAN_TX_HEADER;
//typedef _HAL_CAN_RX_HEADER                              DRV_CAN_RX_HEADER;
#define CAN_LINE1                                       _CAN_LINE1
#define CAN_LINE2                                       _CAN_LINE2

#define COM_CAN_RX_IT_FUNC                              _CORE_CAN_2_IT_FUNC

#define HAL_CAN_1_START()                               _HAL_CAN_1_START()
#define HAL_CAN_2_START()                               _HAL_CAN_2_START()

#define CAN_ADD_TX_MESSAGE(line, txHeader, data, mbox)  _HAL_CAN_ADD_TX_MESSAGE(line, txHeader, data, mbox)
#define HAL_CAN_GET_RX_MESSAGE(v1, v2, v3, v4)          _HAL_CAN_GET_RX_MESSAGE(v1, v2, v3, v4)
/******************************* TYPE DEFINITIONS ****************************/

/************************* GLOBAL VARIBALE REFERENCES ************************/

/************************* GLOBAL FUNCTION DEFINITIONS ************************/

#endif /* __GENERAL_BOARD_CONFIG_H__ */

/********************************* End Of File ********************************/
