/**
 * @file os_port_freertos.h
 * @brief RTOS abstraction layer (FreeRTOS)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2024 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.4.0
 **/

#ifndef _ZOS_PORT_FREERTOS_H
#define _ZOS_PORT_FREERTOS_H

//Dependencies
#ifdef IDF_VER
   #include "freertos/FreeRTOS.h"
   #include "freertos/task.h"
   #include "freertos/semphr.h"
#else
   #include "FreeRTOS.h"
   #include "task.h"
   #include "semphr.h"
#endif

//Types
typedef char char_t;
typedef signed int int_t;
typedef unsigned int uint_t;
typedef int bool_t;

//Invalid task identifier
#define OS_INVALID_TASK_ID NULL
//Self task identifier
#define OS_SELF_TASK_ID NULL

#define OS_TASK_PRIORITY_HIGH          (configMAX_PRIORITIES - 1) //4
#define OS_TASK_PRIORITY_UP_MEDIUM     (configMAX_PRIORITIES - 2) //3
#define OS_TASK_PRIORITY_MEDIUM        (configMAX_PRIORITIES - 3) //2
#define ZOS_TASK_PRIORITY_LOW           (configMAX_PRIORITIES - 4) //1
#define OS_TASK_PRIORITY_LOWEST        (tskIDLE_PRIORITY)         //0

#define OS_TASK_PRIORITY_NORMAL        OS_TASK_PRIORITY_MEDIUM
#define ZOS_MIN_STACK_SIZE               (configMINIMAL_STACK_SIZE)

//Milliseconds to system ticks
#ifndef OS_MS_TO_SYSTICKS
   #define OS_MS_TO_SYSTICKS(n) (n)
#endif

//System ticks to milliseconds
#ifndef OS_SYSTICKS_TO_MS
   #define OS_SYSTICKS_TO_MS(n) (n)
#endif

//Retrieve 64-bit system time (not implemented)
#ifndef osGetSystemTime64
   #define osGetSystemTime64() osGetSystemTime()
#endif

//Task prologue
#ifndef osEnterTask
   #define osEnterTask()
#endif

//Task epilogue
#ifndef osExitTask
   #define osExitTask()
#endif

//Interrupt service routine prologue
#ifndef osEnterIsr
   #if defined(portENTER_SWITCHING_ISR)
      #define osEnterIsr() portENTER_SWITCHING_ISR()
   #else
      #define osEnterIsr()
   #endif
#endif

//Interrupt service routine epilogue
#ifndef osExitIsr
   #if defined(__XTENSA__)
      #define osExitIsr(flag) if(flag) portYIELD_FROM_ISR()
   #elif defined(portEXIT_SWITCHING_ISR)
      #define osExitIsr(flag) portEXIT_SWITCHING_ISR()
   #elif defined(portEND_SWITCHING_ISR)
      #define osExitIsr(flag) portEND_SWITCHING_ISR(flag)
   #elif defined(portYIELD_FROM_ISR)
      #define osExitIsr(flag) portYIELD_FROM_ISR(flag)
   #else
      #define osExitIsr(flag)
   #endif
#endif

//Static object allocation
#ifndef configSUPPORT_STATIC_ALLOCATION
   #define configSUPPORT_STATIC_ALLOCATION 1
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief System time
 **/

typedef TickType_t systime_t;


/**
 * @brief Task identifier
 **/

typedef TaskHandle_t OsTaskId;


/**
 * @brief Task parameters
 **/

typedef struct
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticTask_t *tcb;
#ifdef IDF_VER
   uint32_t *stack;
#else
   StackType_t *stack;
#endif
#endif
   size_t stackSize;
   uint_t priority;
} ZOsTaskParameters;


/**
 * @brief Event object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} ZOsEvent;


/**
 * @brief Semaphore object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} ZOsSemaphore;


/**
 * @brief Mutex object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} ZOsMutex;


/**
 * @brief Task routine
 **/

typedef void (*OsTaskCode)(void *arg);

/**
 * Task info
 */
typedef TaskStatus_t OsTaskInfo;

//Default task parameters
extern const ZOsTaskParameters ZOS_TASK_DEFAULT_PARAMS;

//Kernel management
void zosInitKernel(void);
void zosStartKernel(void);

//Task management
OsTaskId zosCreateTask(const char_t *name, OsTaskCode taskCode, void *arg, const ZOsTaskParameters *params);
void zosGetTaskInfo(OsTaskId task, OsTaskInfo *info);

void zosDeleteTask(OsTaskId taskId);
void zosDelayTask(systime_t delay);
void zosSwitchTask(void);

void zosSuspendTask(OsTaskId task);
void zosResumeTask(OsTaskId task);

void zosSuspendAllTasks(void);
void zosResumeAllTasks(void);

//Event management
bool_t zosCreateEvent(ZOsEvent *event);
void zosDeleteEvent(ZOsEvent *event);
void zosSetEvent(ZOsEvent *event);
void zosResetEvent(ZOsEvent *event);
bool_t zosWaitForEvent(ZOsEvent *event, systime_t timeout);
bool_t zosSetEventFromIsr(ZOsEvent *event);

//Semaphore management
bool_t zosCreateSemaphore(ZOsSemaphore *semaphore, uint_t count);
void zosDeleteSemaphore(ZOsSemaphore *semaphore);
bool_t zosWaitForSemaphore(ZOsSemaphore *semaphore, systime_t timeout);
void zosReleaseSemaphore(ZOsSemaphore *semaphore);

//Mutex management
bool_t zosCreateMutex(ZOsMutex *mutex);
void zosDeleteMutex(ZOsMutex *mutex);
void zosAcquireMutex(ZOsMutex *mutex);
void zosReleaseMutex(ZOsMutex *mutex);

//System time
systime_t zosGetSystemTime(void);

//Memory management
void *zosAllocMem(size_t size);
void zosFreeMem(void *p);

/***********************************************************************
 **********************************************************************/
typedef QueueHandle_t OsQueue;
#define OS_INVALID_QUEUE   NULL
#define QUEUE_SUCCESS      (0)
#define QUEUE_FAILURE      (-1)

#define QUEUE_NAME(x)      x
/*
 * @brief  Open posix queque
 * @param  Queue name,
 * @param  Total gueue leng, total item count
 * @param  Item size
 * @return Returns a message queue descriptor on success, or –1 (INVALID_QUEUE) on error
 */
OsQueue zosMsgQueueCreate(const char *name, unsigned int queLeng, unsigned int itemSize);

/**
 * @brief Send message
 * @param Queue ID
 * @param message
 * @param message length
 * @param timeout, in posix queue it isn't not used. it could be set any value.
 * @return Returns QUEUE_SUCCESS on success, QUEUE_FAILURE on error
 */
int zosMsgQueueSend(OsQueue queue, const char * const msg, size_t msgLeng, unsigned int timeOut);

/**
 * @brief Receive message
 * @param Queue ID
 * @param message buff
 * @param message buff length
 * @param timeout, in posix queue, it isn't not used. it could be set any value.
 * @return Returns QUEUE_SUCCESS on success, QUEUE_FAILURE on error
 */
int zosMsgQueueReceive(OsQueue queue, char * msg, size_t msgLeng, unsigned int timeOutSec);

/**
 * @brief Close the queue
 * @param Queue ID
 * @return Returns QUEUE_SUCCESS on success, QUEUE_FAILURE on error
 */
int zosMsgQueueClose(OsQueue queue);

/**
 * @brief Clear queue
 * @param Queue ID
 * @return Returns QUEUE_SUCCESS on success, QUEUE_FAILURE on error
 */
int zosMsgQueueClear(OsQueue queue);


//C++ guard
#ifdef __cplusplus
}
#endif

#endif
