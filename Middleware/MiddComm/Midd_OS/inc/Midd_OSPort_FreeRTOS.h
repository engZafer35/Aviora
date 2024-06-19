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

#ifndef _OS_PORT_FREERTOS_H
#define _OS_PORT_FREERTOS_H

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

//Invalid task identifier
#define OS_INVALID_TASK_ID NULL
//Self task identifier
#define OS_SELF_TASK_ID NULL

#define OS_TASK_PRIORITY_HIGH          (configMAX_PRIORITIES - 1) //4
#define OS_TASK_PRIORITY_UP_MEDIUM     (configMAX_PRIORITIES - 2) //3
#define OS_TASK_PRIORITY_MEDIUM        (configMAX_PRIORITIES - 3) //2
#define OS_TASK_PRIORITY_LOW           (configMAX_PRIORITIES - 4) //1
#define OS_TASK_PRIORITY_LOWEST        (tskIDLE_PRIORITY)         //0

#define OS_MIN_STACK_SIZE               (configMINIMAL_STACK_SIZE)

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
   #define configSUPPORT_STATIC_ALLOCATION 0
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
} OsTaskParameters;


/**
 * @brief Event object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsEvent;


/**
 * @brief Semaphore object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsSemaphore;


/**
 * @brief Mutex object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsMutex;


/**
 * @brief Task routine
 **/

typedef void (*OsTaskCode)(void *arg);

/**
 * Task info
 */
typedef TaskStatus_t OsTaskInfo;

//Default task parameters
extern const OsTaskParameters OS_TASK_DEFAULT_PARAMS;

//Kernel management
void zosInitKernel(void);
void zosStartKernel(void);

//Task management
OsTaskId zosCreateTask(const char_t *name, OsTaskCode taskCode, void *arg, const OsTaskParameters *params);
void zosGetTaskInfo(OsTaskId task, OsTaskInfo *info);

void zosDeleteTask(OsTaskId taskId);
void zosDelayTask(systime_t delay);
void zosSwitchTask(void);
void zosSuspendAllTasks(void);
void zosResumeAllTasks(void);

//Event management
bool_t zosCreateEvent(OsEvent *event);
void zosDeleteEvent(OsEvent *event);
void zosSetEvent(OsEvent *event);
void zosResetEvent(OsEvent *event);
bool_t zosWaitForEvent(OsEvent *event, systime_t timeout);
bool_t zosSetEventFromIsr(OsEvent *event);

//Semaphore management
bool_t zosCreateSemaphore(OsSemaphore *semaphore, uint_t count);
void zosDeleteSemaphore(OsSemaphore *semaphore);
bool_t zosWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout);
void zosReleaseSemaphore(OsSemaphore *semaphore);

//Mutex management
bool_t zosCreateMutex(OsMutex *mutex);
void zosDeleteMutex(OsMutex *mutex);
void zosAcquireMutex(OsMutex *mutex);
void zosReleaseMutex(OsMutex *mutex);

//System time
systime_t zosGetSystemTime(void);

//Memory management
void *zosAllocMem(size_t size);
void zosFreeMem(void *p);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
