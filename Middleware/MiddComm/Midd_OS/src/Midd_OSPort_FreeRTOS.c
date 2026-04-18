/**
 * @file os_port_freertos.c
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


#if 1

//Dependencies
#include <stdio.h>
#include <stdlib.h>
#include "Midd_OSPort.h"
#include "Midd_OSPort_FreeRTOS.h"

//Default task parameters
const ZOsTaskParameters ZOS_TASK_DEFAULT_PARAMS =
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   NULL,                     //Task control block
   NULL,                     //Stack
#endif
   configMINIMAL_STACK_SIZE, //Size of the stack
   tskIDLE_PRIORITY + 1      //Task priority
};


/**
 * @brief Kernel initialization
 **/

void zosInitKernel(void)
{
}


/**
 * @brief Start kernel
 **/

void zosStartKernel(void)
{
   //Start the scheduler
   vTaskStartScheduler();
}

/**
 * @brief Create a task
 * @param[in] name NULL-terminated string identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] arg Argument passed to the task function
 * @param[in] params Task parameters
 * @return Task identifier referencing the newly created task
 **/

OsTaskId zosCreateTask(const char_t *name, OsTaskCode taskCode, void *arg,
   const ZOsTaskParameters *params)
{
   portBASE_TYPE status;
   TaskHandle_t handle;

#ifdef IDF_VER
   //The stack size is specified in bytes
   stackSize *= sizeof(uint32_t);
#endif

#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Static allocation?
   if(params->tcb != NULL && params->stack != NULL)
   {
      //Create a new task
      handle = xTaskCreateStatic((TaskFunction_t) taskCode, name,
         params->stackSize, arg, params->priority,
         (StackType_t *) params->stack, params->tcb);
   }
   else
#endif
   //Dynamic allocation?
   {
      //Create a new task
      status = xTaskCreate((TaskFunction_t) taskCode, name, params->stackSize,
         arg, params->priority, &handle);

      //Failed to create task?
      if(status != pdPASS)
      {
         handle = OS_INVALID_TASK_ID;
      }
   }

   //Return the handle referencing the newly created task
   return (OsTaskId) handle;
}

void zosGetTaskInfo(OsTaskId task, OsTaskInfo *info)
{
    if (task == NULL || info == NULL)
    {
        return;
    }

    vTaskGetInfo(task, info, pdTRUE, eInvalid);
}

/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void zosDeleteTask(OsTaskId taskId)
{
   //Delete the specified task
   vTaskDelete((TaskHandle_t) taskId);
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void zosDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   vTaskDelay(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void zosSwitchTask(void)
{
   //Force a context switch
   taskYIELD();
}


/**
 * @brief Suspend scheduler activity
 **/

void zosSuspendAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
   {
      //Suspend all tasks
      vTaskSuspendAll();
   }
}


/**
 * @brief Resume scheduler activity
 **/

void zosResumeAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
   {
      //Resume all tasks
      xTaskResumeAll();
   }
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t zosCreateEvent(ZOsEvent *event)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a binary semaphore
   event->handle = xSemaphoreCreateBinaryStatic(&event->buffer);
#else
   //Create a binary semaphore
   event->handle = xSemaphoreCreateBinary();
#endif

   //Check whether the returned handle is valid
   if(event->handle != NULL)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void zosDeleteEvent(ZOsEvent *event)
{
   //Make sure the handle is valid
   if(event->handle != NULL)
   {
      //Properly dispose the event object
      vSemaphoreDelete(event->handle);
   }
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void zosSetEvent(ZOsEvent *event)
{
   //Set the specified event to the signaled state
   xSemaphoreGive(event->handle);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void zosResetEvent(ZOsEvent *event)
{
   //Force the specified event to the nonsignaled state
   xSemaphoreTake(event->handle, 0);
}


/**
 * @brief Wait until the specified event is in the signaled state
 * @param[in] event Pointer to the event object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the state of the specified object is
 *   signaled. FALSE is returned if the timeout interval elapsed
 **/

bool_t zosWaitForEvent(ZOsEvent *event, systime_t timeout)
{
   portBASE_TYPE ret;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = xSemaphoreTake(event->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      ret = xSemaphoreTake(event->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the event is set
   return ret;
}


/**
 * @brief Set an event object to the signaled state from an interrupt service routine
 * @param[in] event Pointer to the event object
 * @return TRUE if setting the event to signaled state caused a task to unblock
 *   and the unblocked task has a priority higher than the currently running task
 **/

bool_t zosSetEventFromIsr(ZOsEvent *event)
{
   portBASE_TYPE flag = FALSE;

   //Set the specified event to the signaled state
   xSemaphoreGiveFromISR(event->handle, &flag);

   //A higher priority task has been woken?
   return flag;
}


/**
 * @brief Create a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] count The maximum count for the semaphore object. This value
 *   must be greater than zero
 * @return The function returns TRUE if the semaphore was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t zosCreateSemaphore(ZOsSemaphore *semaphore, uint_t count)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a semaphore
   semaphore->handle = xSemaphoreCreateCountingStatic(count, count,
      &semaphore->buffer);
#else
   //Create a semaphore
   semaphore->handle = xSemaphoreCreateCounting(count, count);
#endif

   //Check whether the returned handle is valid
   if(semaphore->handle != NULL)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void zosDeleteSemaphore(ZOsSemaphore *semaphore)
{
   //Make sure the handle is valid
   if(semaphore->handle != NULL)
   {
      //Properly dispose the specified semaphore
      vSemaphoreDelete(semaphore->handle);
   }
}


/**
 * @brief Wait for the specified semaphore to be available
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the semaphore is available. FALSE is
 *   returned if the timeout interval elapsed
 **/

bool_t zosWaitForSemaphore(ZOsSemaphore *semaphore, systime_t timeout)
{
   portBASE_TYPE ret;

   //Wait until the specified semaphore becomes available
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = xSemaphoreTake(semaphore->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      ret = xSemaphoreTake(semaphore->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the semaphore is available
   return ret;
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void zosReleaseSemaphore(ZOsSemaphore *semaphore)
{
   //Release the semaphore
   xSemaphoreGive(semaphore->handle);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t zosCreateMutex(ZOsMutex *mutex)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a mutex object
   mutex->handle = xSemaphoreCreateMutexStatic(&mutex->buffer);
#else
   //Create a mutex object
   mutex->handle = xSemaphoreCreateMutex();
#endif

   //Check whether the returned handle is valid
   if(mutex->handle != NULL)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void zosDeleteMutex(ZOsMutex *mutex)
{
   //Make sure the handle is valid
   if(mutex->handle != NULL)
   {
      //Properly dispose the specified mutex
      vSemaphoreDelete(mutex->handle);
   }
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void zosAcquireMutex(ZOsMutex *mutex)
{
   //Obtain ownership of the mutex object
   xSemaphoreTake(mutex->handle, portMAX_DELAY);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void zosReleaseMutex(ZOsMutex *mutex)
{
   //Release ownership of the mutex object
   xSemaphoreGive(mutex->handle);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t zosGetSystemTime(void)
{
   systime_t time;

   //Get current tick count
   time = xTaskGetTickCount();

   //Convert system ticks to milliseconds
   return OS_SYSTICKS_TO_MS(time);
}


/**
 * @brief Allocate a memory block
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

void *zosAllocMem(size_t size)
{
   void *p;

   //Allocate a memory block
   p = pvPortMalloc(size);

   //Return a pointer to the newly allocated memory block
   return p;
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

void zosFreeMem(void *p)
{
   //Make sure the pointer is valid
   if(p != NULL)
   {
      //Free memory block
      vPortFree(p);
   }
}


OsQueue zosMsgQueueCreate(const char *name, unsigned int queLeng, unsigned int itemSize)
{
    (void)name; // FreeRTOS queue name kullanmaz

    QueueHandle_t queue = xQueueCreate(queLeng, itemSize);

    if (queue == NULL)
    {
        return OS_INVALID_QUEUE;
    }

    return queue;
}

int zosMsgQueueSend(OsQueue queue, const char * const msg, size_t msgLeng, unsigned int timeOut)
{
    (void)msgLeng;

    if (queue == NULL || msg == NULL)
    {
        return QUEUE_FAILURE;
    }

    BaseType_t ret = xQueueSend(
        queue,
        msg,
        pdMS_TO_TICKS(timeOut)   // timeout ms kabul ettim
    );

    return (ret == pdPASS) ? QUEUE_SUCCESS : QUEUE_FAILURE;
}

int zosMsgQueueReceive(OsQueue queue, char *msg, size_t msgLeng, unsigned int timeOutSec)
{
    (void)msgLeng;

    if (queue == NULL || msg == NULL)
    {
        return QUEUE_FAILURE;
    }

    BaseType_t ret = xQueueReceive(
        queue,
        msg,
        pdMS_TO_TICKS(timeOutSec)
    );

    return (ret == pdPASS) ? TRUE : QUEUE_FAILURE;
}

int zosMsgQueueClose(OsQueue queue)
{
    if (queue == NULL)
    {
        return QUEUE_FAILURE;
    }

    vQueueDelete(queue);
    return QUEUE_SUCCESS;
}

void zosSuspendTask(OsTaskId task)
{
    vTaskSuspend(task);
}

void zosResumeTask(OsTaskId task)
{
    if (task != NULL)
    {
        vTaskResume(task);
    }
}

/***********************************************************************
 * Event Group handling
 **********************************************************************/
#include <stdint.h>
#include "FreeRTOS.h"
#include "FreeRTOS.h"
#include "event_groups.h"

ZOsEventGroup zosEventGroupCreate(void)
{
    return (ZOsEventGroup)xEventGroupCreate();
}

void zosEventGroupDelete(ZOsEventGroup ev)
{
    if (!ev) return;

    vEventGroupDelete((EventGroupHandle_t)ev);
}

int zosEventGroupSet(ZOsEventGroup ev, uint32_t flags)
{
    if (!ev) return -1;

    xEventGroupSetBits((EventGroupHandle_t)ev, flags);
    return 0;
}

int zosEventGroupClear(ZOsEventGroup ev, uint32_t flags)
{
    if (!ev) return -1;

    xEventGroupClearBits((EventGroupHandle_t)ev, flags);
    return 0;
}

int zosEventGroupWait(ZOsEventGroup ev, uint32_t flags, uint32_t timeoutMs, uint32_t options)
{
    if (!ev) return -1;

    BaseType_t waitAll = (options & ZOS_EVENT_WAIT_ALL);
    BaseType_t clear   = (options & ZOS_EVENT_CLEAR);

    EventBits_t ret = xEventGroupWaitBits(
        (EventGroupHandle_t)ev,
        flags,
        clear,
        waitAll,
        pdMS_TO_TICKS(timeoutMs)
    );

    if (waitAll)
    {
        return ((ret & flags) == flags) ? 0 : -1;
    }
    else
    {
        return (ret & flags) ? 0 : -1;
    }
}







#if (configSUPPORT_STATIC_ALLOCATION == 1 && !defined(IDF_VER))

/**
 * @brief Provide the memory that is used by the idle task
 **/

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
   StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
   static StaticTask_t xIdleTaskTCB;
   static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

   *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
   *ppxIdleTaskStackBuffer = uxIdleTaskStack;
   *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#endif


#if 0

/**
 * @brief FreeRTOS stack overflow hook
 **/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
   (void) pcTaskName;
   (void) pxTask;

   taskDISABLE_INTERRUPTS();
   while(1);
}


/**
 * @brief Trap FreeRTOS errors
 **/

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
   volatile unsigned long ul = 0;

   (void) pcFile;
   (void) ulLine;

   taskENTER_CRITICAL();

   //Set ul to a non-zero value using the debugger to step out of this function
   while(ul == 0)
   {
      portNOP();
   }

   taskEXIT_CRITICAL();
}

#endif
#endif
