/**
 * @file os_port_posix.h
 * @brief RTOS abstraction layer (POSIX Threads)
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

#ifndef _ZOS_PORT_POSIX_H
#define _ZOS_PORT_POSIX_H

//Dependencies
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>

#include "Midd_OSPort.h"

//Invalid task identifier
#define OS_INVALID_TASK_ID -1
//Self task identifier
#define OS_SELF_TASK_ID -1

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
#define osEnterTask()
//Task epilogue
#define osExitTask()
//Interrupt service routine prologue
#define osEnterIsr()
//Interrupt service routine epilogue
#define osExitIsr(flag)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief System time
 **/

typedef unsigned int systime_t;


/**
 * @brief Task identifier
 **/

typedef pthread_t OsTaskId;


/**
 * @brief Task parameters
 **/

typedef struct
{
   size_t stackSize;
   unsigned int priority;
} ZOsTaskParameters;


/**
 * @brief Event object
 **/

typedef sem_t OsEvent;


/**
 * @brief Semaphore object
 **/

typedef sem_t OsSemaphore;


/**
 * @brief Mutex object
 **/

typedef pthread_mutex_t OsMutex;

/**
 * Task info
 */
typedef OsTaskId OsTaskInfo;
/**
 * @brief Task routine
 **/

typedef void (*OsTaskCode)(void *arg);

//Default task parameters
extern const ZOsTaskParameters ZOS_TASK_DEFAULT_PARAMS;

//Kernel management
void zosInitKernel(void);
void zosStartKernel(void);

//Task management
#define ZOS_TASK_PRIORITY_HIGH          (4)
#define ZOS_TASK_PRIORITY_MEDIUM_UP     (3)
#define ZOS_TASK_PRIORITY_MEDIUM        (2)
#define ZOS_TASK_PRIORITY_LOW           (1)
#define ZOS_TASK_PRIORITY_LOWEST        (0)

#define ZOS_MIN_STACK_SIZE              (128)
OsTaskId zosCreateTask(const char *name, OsTaskCode taskCode, void *arg, const ZOsTaskParameters *params);
void zosGetTaskInfo(OsTaskId tid, OsTaskInfo *info);

void zosSuspendTask(OsTaskId task);
void zosResumeTask(OsTaskId task);

void zosDeleteTask(OsTaskId taskId);
void zosDelayTask(systime_t delay);
void zosSwitchTask(void);
void zosSuspendAllTasks(void);
void zosResumeAllTasks(void);

//Event management
int zosCreateEvent(OsEvent *event);
void zosDeleteEvent(OsEvent *event);
void zosSetEvent(OsEvent *event);
void zosResetEvent(OsEvent *event);
int zosWaitForEvent(OsEvent *event, systime_t timeout);
int zosSetEventFromIsr(OsEvent *event);

//Semaphore management
int zosCreateSemaphore(OsSemaphore *semaphore, unsigned int count);
void zosDeleteSemaphore(OsSemaphore *semaphore);
int zosWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout);
void zosReleaseSemaphore(OsSemaphore *semaphore);

//Mutex management
int zosCreateMutex(OsMutex *mutex);
void zosDeleteMutex(OsMutex *mutex);
void zosAcquireMutex(OsMutex *mutex);
void zosReleaseMutex(OsMutex *mutex);

//System time
systime_t osGetSystemTime(void);

//Memory management
void *zosAllocMem(size_t size);
void zosFreeMem(void *p);


/***********************************************************************
 **********************************************************************/
typedef mqd_t OsQueue;
#define OS_INVALID_QUEUE   (-1)
#define QUEUE_SUCCESS   (0)
#define QUEUE_FAILURE   (-1)

#define QUEUE_NAME(x)     x
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
