/**
 * @file os_port_posix.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL TRACE_LEVEL_OFF

//Dependencies
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "Midd_OSPort.h"
#include "Midd_OSPort_Posix.h"

//Pthread start routine
typedef void *(*PthreadTaskCode) (void *param);

//Default task parameters
const ZOsTaskParameters OS_TASK_DEFAULT_PARAMS =
{
   0, //Size of the stack
   0  //Task priority
};

volatile struct
{
    OsTaskId id;
    volatile int suspend;
}taskList[10] = {{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,{-1,0} ,};
/**
 * @brief Kernel initialization
 **/

void zosInitKernel(void)
{
   //Not implemented
}


/**
 * @brief Start kernel
 **/

void zosStartKernel(void)
{
   //Not implemented
}


/**
 * @brief Create a task
 * @param[in] name NULL-terminated string identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] arg Argument passed to the task function
 * @param[in] params Task parameters
 * @return Task identifier referencing the newly created task
 **/

OsTaskId zosCreateTask(const char *name, OsTaskCode taskCode, void *arg, const ZOsTaskParameters *params)
{
   int ret;
   pthread_t thread;

   //Create a new thread
   ret = pthread_create(&thread, NULL, (PthreadTaskCode) taskCode, arg);

   //Return a pointer to the newly created thread
   if(ret == 0)
   {
       for (int i = 0; i < 10; i++)
       {
           if (OS_INVALID_TASK_ID == taskList[i].id)
           {
               taskList[i].id = thread;
               taskList[i].suspend = 0;
               break;
           }
       }


      return (OsTaskId) thread;
   }
   else
   {
      return OS_INVALID_TASK_ID;
   }
}

void zosGetTaskInfo(OsTaskId tid, OsTaskInfo *info)
{
    *info = tid; //just return task id as a task info
}

void zosSuspendTask(OsTaskId task)
{
    for (int i = 0; i < 10; i++)
    {
        if (taskList[i].id == task)
        {
            taskList[i].suspend = 1;
            break; //delete it after the below lines are activated.
//            do
//            {
//                sleep(1);
//            }while (taskList[i].suspend);
        }
    }
}
void zosResumeTask(OsTaskId task)
{
    for (int i = 0; i < 10; i++)
    {
        if (taskList[i].id == task)
        {
            taskList[i].suspend = 0;
            break;
        }
    }
}

/**
 * @brief Delete a task
 * @param[in] taskId Task identifier referencing the task to be deleted
 **/

void zosDeleteTask(OsTaskId taskId)
{
   //Delete the calling thread?
   if(taskId == OS_SELF_TASK_ID)
   {
      //Kill ourselves
      pthread_exit(NULL);
   }
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void zosDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   usleep(delay * 1000);
}


/**
 * @brief Yield control to the next task
 **/

void zosSwitchTask(void)
{
   //Not implemented
}


/**
 * @brief Suspend scheduler activity
 **/

void zosSuspendAllTasks(void)
{
   //Not implemented
}


/**
 * @brief Resume scheduler activity
 **/

void zosResumeAllTasks(void)
{
   //Not implemented
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

int zosCreateEvent(OsEvent *event)
{
   int ret;

   //Create a semaphore object
   ret = sem_init(event, 0, 0);

   //Check whether the semaphore was successfully created
   if(ret == 0)
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

void zosDeleteEvent(OsEvent *event)
{
   //Properly dispose the event object
   sem_destroy(event);
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void zosSetEvent(OsEvent *event)
{
   int ret;
   int value;

   //Get the current value of the semaphore
   ret = sem_getvalue(event, &value);

   //Nonsignaled state?
   if(ret == 0 && value == 0)
   {
      //Set the specified event to the signaled state
      sem_post(event);
   }
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void zosResetEvent(OsEvent *event)
{
   int ret;

   //Force the specified event to the nonsignaled state
   do
   {
      //Decrement the semaphore's count by one
      ret = sem_trywait(event);

      //Check status
   } while(ret == 0);
}


/**
 * @brief Wait until the specified event is in the signaled state
 * @param[in] event Pointer to the event object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the state of the specified object is
 *   signaled. FALSE is returned if the timeout interval elapsed
 **/

int zosWaitForEvent(OsEvent *event, systime_t timeout)
{
   int ret;
   struct timespec ts;

   //Wait until the specified event is in the signaled state or the timeout
   //interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      ret = sem_trywait(event);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = sem_wait(event);
   }
   else
   {
      //Get current time
      clock_gettime(CLOCK_REALTIME, &ts);

      //Set absolute timeout
      ts.tv_sec += timeout / 1000;
      ts.tv_nsec += (timeout % 1000) * 1000000;

      //Normalize time stamp value
      if(ts.tv_nsec >= 1000000000)
      {
         ts.tv_sec += 1;
         ts.tv_nsec -= 1000000000;
      }

      //Wait until the specified event becomes set
      ret = sem_timedwait(event, &ts);
   }

   //Check whether the specified event is set
   if(ret == 0)
   {
      //Force the event back to the nonsignaled state
      do
      {
         //Decrement the semaphore's count by one
         ret = sem_trywait(event);

         //Check status
      } while(ret == 0);

      //The specified event is in the signaled state
      return TRUE;
   }
   else
   {
      //The timeout interval elapsed
      return FALSE;
   }
}


/**
 * @brief Set an event object to the signaled state from an interrupt service routine
 * @param[in] event Pointer to the event object
 * @return TRUE if setting the event to signaled state caused a task to unblock
 *   and the unblocked task has a priority higher than the currently running task
 **/

int zosSetEventFromIsr(OsEvent *event)
{
   //Not implemented
   return FALSE;
}


/**
 * @brief Create a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] count The maximum count for the semaphore object. This value
 *   must be greater than zero
 * @return The function returns TRUE if the semaphore was successfully
 *   created. Otherwise, FALSE is returned
 **/

int zosCreateSemaphore(OsSemaphore *semaphore, unsigned int count)
{
   int ret;

   //Create a semaphore object
   ret = sem_init(semaphore, 0, count);

   //Check whether the semaphore was successfully created
   if(ret == 0)
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

void zosDeleteSemaphore(OsSemaphore *semaphore)
{
   //Properly dispose the semaphore object
   sem_destroy(semaphore);
}


/**
 * @brief Wait for the specified semaphore to be available
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the semaphore is available. FALSE is
 *   returned if the timeout interval elapsed
 **/

int zosWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout)
{
   int ret;
   struct timespec ts;

   //Wait until the semaphore is available or the timeout interval elapses
   if(timeout == 0)
   {
      //Non-blocking call
      ret = sem_trywait(semaphore);
   }
   else if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = sem_wait(semaphore);
   }
   else
   {
      //Get current time
      clock_gettime(CLOCK_REALTIME, &ts);

      //Set absolute timeout
      ts.tv_sec += timeout / 1000;
      ts.tv_nsec += (timeout % 1000) * 1000000;

      //Normalize time stamp value
      if(ts.tv_nsec >= 1000000000)
      {
         ts.tv_sec += 1;
         ts.tv_nsec -= 1000000000;
      }

      //Wait until the specified semaphore becomes available
      ret = sem_timedwait(semaphore, &ts);
   }

   //Check whether the specified semaphore is available
   if(ret == 0)
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void zosReleaseSemaphore(OsSemaphore *semaphore)
{
   //Release the semaphore
   sem_post(semaphore);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

int zosCreateMutex(OsMutex *mutex)
{
   int ret;

   //Create a mutex object
   ret = pthread_mutex_init(mutex, NULL);

   //Check whether the mutex was successfully created
   if(ret == 0)
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

void zosDeleteMutex(OsMutex *mutex)
{
   //Properly dispose the mutex object
   pthread_mutex_destroy(mutex);
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void zosAcquireMutex(OsMutex *mutex)
{
   //Obtain ownership of the mutex object
   pthread_mutex_lock(mutex);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void zosReleaseMutex(OsMutex *mutex)
{
   //Release ownership of the mutex object
   pthread_mutex_unlock(mutex);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t zosGetSystemTime(void)
{
   struct timeval tv;

   //Get current time
   gettimeofday(&tv, NULL);

   //Convert resulting value to milliseconds
   return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/**
 * @brief Allocate a memory block
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

void *zosAllocMem(size_t size)
{
   //Allocate a memory block
   return malloc(size);
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

void zosFreeMem(void *p)
{
   //Free memory block
   free(p);
}

/***********************************************************************
 **********************************************************************/
#include <mqueue.h>
OsQueue zosMsgQueueCreate(const char *name, unsigned int queLeng, unsigned int itemSize)
{
//#define QUEUE_PERMS ((int)(0777))
//#define QUEUE_MAXMSG  5 /* Maximum number of messages. */
//#define QUEUE_MSGSIZE 1024 /* Length of message. */
//#define QUEUE_ATTR_INITIALIZER ((struct mq_attr){0, QUEUE_MAXMSG, QUEUE_MSGSIZE, 0, {0}})

    static int nameUniq = 0;
    char nameTemp[32] = "";
    struct mq_attr attr;
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = itemSize;
    attr.mq_flags = O_RDWR | O_CREAT;

    sprintf(nameTemp, "/%s-%d", name, nameUniq++); //create unique name. So app level dont need to handle this case
    return (OsQueue) mq_open(nameTemp, O_RDWR | O_CREAT | O_NONBLOCK, 0777, &attr);
}

int zosMsgQueueSend(OsQueue queue, const char * msg, size_t msgLeng,  unsigned int timeOut)
{
    (void)timeOut;
    return mq_send(queue, msg, msgLeng, 0); //0 -> priority, doesn't matter
}

int zosMsgQueueReceive(OsQueue queue, char * msg, size_t msgLeng, unsigned int timeOutSec)
{
    unsigned int prio;

    return mq_receive(queue, msg, msgLeng, &prio);

    //struct timespec tm;
    //tm.tv_sec  = 0;
    //tm.tv_nsec = timeOutSec*1000000;

    //return mq_timedreceive(queue, msg, msgLeng, NULL, &tm);
}

int zosMsgQueueClose(OsQueue queue)
{
    return mq_close(queue);
}

int zosMsgQueueClear(OsQueue queue)
{
    struct mq_attr attr;
    char tempBuff[128];

    if(mq_getattr(queue, &attr) == 0)
    {
       while(attr.mq_curmsgs)
       {
           mq_receive(queue, tempBuff, sizeof(tempBuff), NULL);
           attr.mq_curmsgs--;
       }
    }

    return QUEUE_SUCCESS;
}
