/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE 1:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the comprehensive test and demo version.
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * full demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware, are defined in main.c.
 *
 ******************************************************************************
 *
 * main_full() creates all the demo application tasks and software timers, then
 * starts the scheduler.  The web documentation provides more details of the
 * standard demo application tasks, which provide no particular functionality,
 * but do provide a good example of how to use the FreeRTOS API.
 *
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Reg test" tasks - These fill the core registers with known values, then
 * check that each register maintains its expected value for the lifetime of the
 * task.  Each task uses a different set of values.  The reg test tasks execute
 * with a very low priority, so get preempted very frequently.  A register
 * containing an unexpected value is indicative of an error in the context
 * switching mechanism.
 *
 * "Check" task - The check task period is initially set to three seconds.  The
 * task checks that all the standard demo tasks, and the register check tasks,
 * are not only still executing, but are executing without reporting any errors.
 * If the check task discovers that a task has either stalled, or reported an
 * error, then it changes its own execution period from the initial three
 * seconds, to just 200ms.  The check task also toggles an LED each time it is
 * called.  This provides a visual indication of the system status:  If the LED
 * toggles every three seconds, then no issues have been discovered.  If the LED
 * toggles every 200ms, then an issue has been discovered with at least one
 * task.
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Standard demo application includes. */
#include "flop.h"
#include "semtest.h"
#include "dynamic.h"
#include "BlockQ.h"
#include "blocktim.h"
#include "countsem.h"
#include "GenQTest.h"
#include "recmutex.h"
#include "death.h"
#include "partest.h"
#include "comtest2.h"
#include "serial.h"
#include "TimerDemo.h"
#include "QueueOverwrite.h"
#include "IntQueue.h"
#include "EventGroupsDemo.h"
#include "flash.h"
#include "TaskNotify.h"
#include "IntSemTest.h"
#include "AbortDelay.h"
#include "MessageBufferDemo.h"
#include "StreamBufferDemo.h"
#include "StreamBufferInterrupt.h"
#include "MessageBufferAMP.h"
#include "PollQ.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "QueueSet.h"
#include "QueueSetPolling.h"
#include "integer.h"

/* Priorities for the demo application tasks. */
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1UL )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2UL )
#define mainCREATOR_TASK_PRIORITY			( tskIDLE_PRIORITY + 3UL )
#define mainFLOP_TASK_PRIORITY				( tskIDLE_PRIORITY )
#define mainCOM_TEST_TASK_PRIORITY			( tskIDLE_PRIORITY + 2 )
#define mainCHECK_TASK_PRIORITY				( configMAX_PRIORITIES - 1 )
#define mainQUEUE_OVERWRITE_PRIORITY		( tskIDLE_PRIORITY )
#define mainFLASH_PRIORITY					( tskIDLE_PRIORITY )
#define mainINTEGER_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define mainQUEUE_POLL_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainMESSAGE_BUFFER_STACK_SIZE		( configMINIMAL_STACK_SIZE *2UL )

/* The priority used by the UART command console task. */
#define mainUART_COMMAND_CONSOLE_TASK_PRIORITY	( configMAX_PRIORITIES - 2 )

/* The LED used by the check task. */
#define mainCHECK_LED						( 1 )

/* A block time of zero simply means "don't block". */
#define mainDONT_BLOCK						( 0UL )

/* The period of the check task, in ms, provided no errors have been reported by
any of the standard demo tasks.  ms are converted to the equivalent in ticks
using the pdMS_TO_TICKS() macro constant. */
#define mainNO_ERROR_CHECK_TASK_PERIOD		pdMS_TO_TICKS( 3000UL )

/* The period of the check task, in ms, if an error has been reported in one of
the standard demo tasks.  ms are converted to the equivalent in ticks using the
pdMS_TO_TICKS() macro. */
#define mainERROR_CHECK_TASK_PERIOD 		pdMS_TO_TICKS( 200UL )

/* Parameters that are passed into the register check tasks solely for the
purpose of ensuring parameters are passed into tasks correctly. */
#define mainREG_TEST_TASK_1_PARAMETER		( ( void * ) 0x12345678 )
#define mainREG_TEST_TASK_2_PARAMETER		( ( void * ) 0x87654321 )

/* The base period used by the timer test tasks. */
#define mainTIMER_TEST_PERIOD				( 50 )

/*-----------------------------------------------------------*/

/*
 * Called by main() to run the full demo (as opposed to the blinky demo) when
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY is set to 0.
 */
void main_full( void );

/*
 * The check task, as described at the top of this file.
 */
static void prvCheckTask( void *pvParameters );

/*
 * Register check tasks, and the tasks used to write over and check the contents
 * of the FPU registers, as described at the top of this file.  The nature of
 * these files necessitates that they are written in an assembly file, but the
 * entry points are kept in the C file for the convenience of checking the task
 * parameter.
 */
static void prvRegTestTaskEntry1( void *pvParameters );
extern void vRegTest1Implementation( void );
static void prvRegTestTaskEntry2( void *pvParameters );
extern void vRegTest2Implementation( void );

/*
 * A high priority task that does nothing other than execute at a pseudo random
 * time to ensure the other test tasks don't just execute in a repeating
 * pattern.
 */
static void prvPseudoRandomiser( void *pvParameters );

/*
 * A task to demonstrate the use of the xQueueSpacesAvailable() function.
 */
static void prvDemoQueueSpaceFunctions( void *pvParameters );

/*
 * Tasks that ensure indefinite delays are truly indefinite.
 */
static void prvPermanentlyBlockingSemaphoreTask( void *pvParameters );
static void prvPermanentlyBlockingNotificationTask( void *pvParameters );


/*-----------------------------------------------------------*/

/* The following two variables are used to communicate the status of the
register check tasks to the check task.  If the variables keep incrementing,
then the register check tasks have not discovered any errors.  If a variable
stops incrementing, then an error has been found. */
volatile uint32_t ulRegTest1LoopCounter = 0UL, ulRegTest2LoopCounter = 0UL;

/* This semaphore is created purely to test using the vSemaphoreDelete() and
semaphore tracing API functions.  It has no other purpose. */
static SemaphoreHandle_t xMutexToDelete = NULL;

/* The variable into which error messages are latched. */
static char *pcStatusMessage = "All tasks running without error";

/*-----------------------------------------------------------*/

void main_full( void )
{
	/* Start all the other standard demo/test tasks.  They have no particular
	functionality, but do demonstrate how to use the FreeRTOS API and test the
	kernel port. */
	vStartTaskNotifyTask();
	vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );
	vStartPolledQueueTasks( mainQUEUE_POLL_PRIORITY );
	vStartIntegerMathTasks( mainINTEGER_TASK_PRIORITY );
	vStartQueuePeekTasks();
	vStartQueueSetTasks();
	vStartInterruptSemaphoreTasks();
	vStartQueueSetPollingTask();
#if (INCLUDE_xTaskAbortDelay == 1)
	vCreateAbortDelayTasks();
#endif

	vStartInterruptQueueTasks();
	vStartDynamicPriorityTasks();
	vStartBlockingQueueTasks( mainBLOCK_Q_PRIORITY );
	vCreateBlockTimeTasks();
	vStartCountingSemaphoreTasks();
	vStartGenericQueueTasks( tskIDLE_PRIORITY );
	vStartRecursiveMutexTasks();
	vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );

	vStartMessageBufferTasks( mainMESSAGE_BUFFER_STACK_SIZE );
	vStartStreamBufferTasks();
	vStartStreamBufferInterruptDemo();
	vStartMessageBufferAMPTasks( mainMESSAGE_BUFFER_STACK_SIZE );

	// flop.c と sp_flopがある
	// sp_flopはsingle precision: 単精度浮動小数点: floatをテストで使用する
	vStartMathTasks( mainFLOP_TASK_PRIORITY );
	vStartTimerDemoTask( mainTIMER_TEST_PERIOD );
	vStartQueueOverwriteTask( mainQUEUE_OVERWRITE_PRIORITY );
	vStartEventGroupTasks();
	vStartLEDFlashTasks( mainFLASH_PRIORITY );

	xTaskCreate( prvDemoQueueSpaceFunctions, "QSpace", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
	xTaskCreate( prvPermanentlyBlockingSemaphoreTask, "BlockSem", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );
	xTaskCreate( prvPermanentlyBlockingNotificationTask, "BlockNoti", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL );

	/* Create the register check tasks, as described at the top of this	file */
	xTaskCreate( prvRegTestTaskEntry1, "Reg1", configMINIMAL_STACK_SIZE, mainREG_TEST_TASK_1_PARAMETER, tskIDLE_PRIORITY, NULL );
	xTaskCreate( prvRegTestTaskEntry2, "Reg2", configMINIMAL_STACK_SIZE, mainREG_TEST_TASK_2_PARAMETER, tskIDLE_PRIORITY, NULL );

	/* Create the task that just adds a little random behaviour. */
	xTaskCreate( prvPseudoRandomiser, "Rnd", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL );

	/* Create the task that performs the 'check' functionality,	as described at
	the top of this file. */
	xTaskCreate( prvCheckTask, "Check", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, NULL );

	/* The set of tasks created by the following function call have to be
	created last as they keep account of the number of tasks they expect to see
	running. */
	vCreateSuicidalTasks( mainCREATOR_TASK_PRIORITY );

	/* Create the semaphore that will be deleted in the idle task hook.  This
	is done purely to test the use of vSemaphoreDelete(). */
	xMutexToDelete = xSemaphoreCreateMutex();

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvCheckTask( void *pvParameters )
{
TickType_t xDelayPeriod = mainNO_ERROR_CHECK_TASK_PERIOD;
TickType_t xLastExecutionTime;
static uint32_t ulLastRegTest1Value = 0, ulLastRegTest2Value = 0;

	/* Just to stop compiler warnings. */
	( void ) pvParameters;

	/* Initialise xLastExecutionTime so the first call to vTaskDelayUntil()
	works correctly. */
	xLastExecutionTime = xTaskGetTickCount();

	/* Cycle for ever, delaying then checking all the other tasks are still
	operating without error.  The onboard LED is toggled on each iteration.
	If an error is detected then the delay period is decreased from
	mainNO_ERROR_CHECK_TASK_PERIOD to mainERROR_CHECK_TASK_PERIOD.  This has the
	effect of increasing the rate at which the onboard LED toggles, and in so
	doing gives visual feedback of the system status. */
	for( ;; )
	{
		/* Delay until it is time to execute again. */
		vTaskDelayUntil( &xLastExecutionTime, xDelayPeriod );

		/* Check all the demo tasks (other than the flash tasks) to ensure
		that they are all still running, and that none have detected an error. */
		if( xAreIntQueueTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: IntQueue";
		}

		if( xAreMathsTaskStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: Flop";
		}

		if( xAreDynamicPriorityTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: Dynamic";
		}

		if( xAreBlockingQueuesStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: BlockQueue";
		}

		if ( xAreBlockTimeTestTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: Block time";
		}

		if ( xAreGenericQueueTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: GenQueue";
		}

		if ( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: RecMutex";
		}

		if( xIsCreateTaskStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: Death";
		}

		if( xAreSemaphoreTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: SemTest";
		}

		if( xAreTimerDemoTasksStillRunning( ( TickType_t ) mainNO_ERROR_CHECK_TASK_PERIOD ) != pdPASS )
		{
			pcStatusMessage = "Error: TimerDemo";
		}

		if( xAreCountingSemaphoreTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: CountSem";
		}

		if( xIsQueueOverwriteTaskStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Queue overwrite";
		}

		if( xAreEventGroupTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: EventGroup";
		}

		/* Check that the register test 1 task is still running. */
		if( ulLastRegTest1Value == ulRegTest1LoopCounter )
		{
			pcStatusMessage = "Error: Register 1";
		}
		ulLastRegTest1Value = ulRegTest1LoopCounter;

		/* Check that the register test 2 task is still running. */
		if( ulLastRegTest2Value == ulRegTest2LoopCounter )
		{
			pcStatusMessage = "Error: Register 2";
		}
		ulLastRegTest2Value = ulRegTest2LoopCounter;

		if( xAreStreamBufferTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error:  StreamBuffer";
		}

		if( xAreMessageBufferTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error:  MessageBuffer";
		}

		if( xAreTaskNotificationTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error:  Notification";
		}

		if( xAreInterruptSemaphoreTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: IntSem";
		}

		if( xAreIntegerMathsTaskStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: IntMath";
		}

		if( xAreQueuePeekTasksStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: QueuePeek";
		}

		if( xArePollingQueuesStillRunning() != pdTRUE )
		{
			pcStatusMessage = "Error: PollQueue";
		}

		if( xAreQueueSetTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Queue set";
		}

		if( xAreQueueSetPollTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Queue set polling";
		}

#if (INCLUDE_xTaskAbortDelay == 1)
		if( xAreAbortDelayTestTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Abort delay";
		}
#endif

		if( xIsInterruptStreamBufferDemoStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Stream buffer interrupt";
		}

		if( xAreMessageBufferAMPTasksStillRunning() != pdPASS )
		{
			pcStatusMessage = "Error: Message buffer AMP";
		}


		/* Toggle the check LED to give an indication of the system status.  If
		the LED toggles every mainNO_ERROR_CHECK_TASK_PERIOD milliseconds then
		everything is ok.  A faster toggle indicates an error. */
		vParTestToggleLED( mainCHECK_LED );

		if( pcStatusMessage[0] == 'E' )
		{
			/* An error has been detected in one of the tasks - flash the LED
			at a higher frequency to give visible feedback that something has
			gone wrong (it might just be that the loop back connector required
			by the comtest tasks has not been fitted). */
			xDelayPeriod = mainERROR_CHECK_TASK_PERIOD;
		}
	}
}
/*-----------------------------------------------------------*/

static void prvRegTestTaskEntry1( void *pvParameters )
{
	/* Although the regtest task is written in assembler, its entry point is
	written in C for convenience of checking the task parameter is being passed
	in correctly. */
	if( pvParameters == mainREG_TEST_TASK_1_PARAMETER )
	{
		/* Start the part of the test that is written in assembler. */
		vRegTest1Implementation();
	}

	/* The following line will only execute if the task parameter is found to
	be incorrect.  The check task will detect that the regtest loop counter is
	not being incremented and flag an error. */
	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

static void prvRegTestTaskEntry2( void *pvParameters )
{
	/* Although the regtest task is written in assembler, its entry point is
	written in C for convenience of checking the task parameter is being passed
	in correctly. */
	if( pvParameters == mainREG_TEST_TASK_2_PARAMETER )
	{
		/* Start the part of the test that is written in assembler. */
		vRegTest2Implementation();
	}

	/* The following line will only execute if the task parameter is found to
	be incorrect.  The check task will detect that the regtest loop counter is
	not being incremented and flag an error. */
	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

static void prvPseudoRandomiser( void *pvParameters )
{
const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL, ulMinDelay = ( 35 / portTICK_PERIOD_MS );
volatile uint32_t ulNextRand = ( uint32_t ) &pvParameters, ulValue;

	/* This task does nothing other than ensure there is a little bit of
	disruption in the scheduling pattern of the other tasks.  Normally this is
	done by generating interrupts at pseudo random times. */
	for( ;; )
	{
		ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
		ulValue = ( ulNextRand >> 16UL ) & 0xffUL;

		if( ulValue < ulMinDelay )
		{
			ulValue = ulMinDelay;
		}

		vTaskDelay( ulValue );

		while( ulValue > 0 )
		{
			nop();
			nop();
			nop();

			ulValue--;
		}
	}
}

static void prvDemoQueueSpaceFunctions( void *pvParameters )
{
QueueHandle_t xQueue = NULL;
const unsigned portBASE_TYPE uxQueueLength = 10;
unsigned portBASE_TYPE uxReturn, x;

	/* Remove compiler warnings. */
	( void ) pvParameters;

	/* Create the queue that will be used.  Nothing is actually going to be
	sent or received so the queue item size is set to 0. */
	xQueue = xQueueCreate( uxQueueLength, 0 );
	configASSERT( xQueue );

	for( ;; )
	{
		for( x = 0; x < uxQueueLength; x++ )
		{
			/* Ask how many messages are available... */
			uxReturn = uxQueueMessagesWaiting( xQueue );

			/* Check the number of messages being reported as being available
			is as expected, and force an assert if not. */
			if( uxReturn != x )
			{
				/* xQueue cannot be NULL so this is deliberately causing an
				assert to be triggered as there is an error. */
				configASSERT( xQueue == NULL );
			}

			/* Ask how many spaces remain in the queue... */
			uxReturn = uxQueueSpacesAvailable( xQueue );

			/* Check the number of spaces being reported as being available
			is as expected, and force an assert if not. */
			if( uxReturn != ( uxQueueLength - x ) )
			{
				/* xQueue cannot be NULL so this is deliberately causing an
				assert to be triggered as there is an error. */
				configASSERT( xQueue == NULL );
			}

			/* Fill one more space in the queue. */
			xQueueSendToBack( xQueue, NULL, 0 );
		}

		/* Perform the same check while the queue is full. */
		uxReturn = uxQueueMessagesWaiting( xQueue );
		if( uxReturn != uxQueueLength )
		{
			configASSERT( xQueue == NULL );
		}

		uxReturn = uxQueueSpacesAvailable( xQueue );

		if( uxReturn != 0 )
		{
			configASSERT( xQueue == NULL );
		}

		/* The queue is full, start again. */
		xQueueReset( xQueue );

		#if( configUSE_PREEMPTION == 0 )
			taskYIELD();
		#endif
	}
}
/*-----------------------------------------------------------*/

static void prvPermanentlyBlockingSemaphoreTask( void *pvParameters )
{
SemaphoreHandle_t xSemaphore;

	/* Prevent compiler warning about unused parameter in the case that
	configASSERT() is not defined. */
	( void ) pvParameters;

	/* This task should block on a semaphore, and never return. */
	xSemaphore = xSemaphoreCreateBinary();
	configASSERT( xSemaphore );

	xSemaphoreTake( xSemaphore, portMAX_DELAY );

	/* The above xSemaphoreTake() call should never return, force an assert if
	it does. */
	configASSERT( pvParameters != NULL );
	vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

static void prvPermanentlyBlockingNotificationTask( void *pvParameters )
{
	/* Prevent compiler warning about unused parameter in the case that
	configASSERT() is not defined. */
	( void ) pvParameters;

	/* This task should block on a task notification, and never return. */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	/* The above ulTaskNotifyTake() call should never return, force an assert
	if it does. */
	configASSERT( pvParameters != NULL );
	vTaskDelete( NULL );
}

/* Called from vApplicationIdleHook(), which is defined in main.c. */
void vFullDemoIdleFunction( void )
{
volatile size_t xFreeHeapSpace, xMinimumEverFreeHeapSpace;

	/* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task.  It must *NOT* attempt to block.  In this case the
	idle task just queries the amount of FreeRTOS heap that remains.  See the
	memory management section on the http://www.FreeRTOS.org web site for memory
	management options.  If there is a lot of heap memory free then the
	configTOTAL_HEAP_SIZE value in FreeRTOSConfig.h can be reduced to free up
	RAM. */
	xFreeHeapSpace = xPortGetFreeHeapSize();
	xMinimumEverFreeHeapSpace = xPortGetMinimumEverFreeHeapSize();

	/* Remove compiler warning about xFreeHeapSpace being set but never used. */
	( void ) xFreeHeapSpace;
	( void ) xMinimumEverFreeHeapSpace;

	/* If xMutexToDelete has not already been deleted, then delete it now.
	This is done purely to demonstrate the use of, and test, the
	vSemaphoreDelete() macro.  Care must be taken not to delete a semaphore
	that has tasks blocked on it. */
	if( xMutexToDelete != NULL )
	{
		/* For test purposes, add the mutex to the registry, then remove it
		again, before it is deleted - checking its name is as expected before
		and after the assertion into the registry and its removal from the
		registry. */
		configASSERT( pcQueueGetName( xMutexToDelete ) == NULL );
		vQueueAddToRegistry( xMutexToDelete, "Test_Mutex" );
		configASSERT( strcmp( pcQueueGetName( xMutexToDelete ), "Test_Mutex" ) == 0 );
		vQueueUnregisterQueue( xMutexToDelete );
		configASSERT( pcQueueGetName( xMutexToDelete ) == NULL );

		vSemaphoreDelete( xMutexToDelete );
		xMutexToDelete = NULL;
	}

}
/*-----------------------------------------------------------*/

/* Called by vApplicationTickHook(), which is defined in main.c. */
void vFullDemoTickHookFunction( void )
{
TaskHandle_t xTimerTask;

	/* Call the periodic timer test, which tests the timer API functions that
	can be called from an ISR. */
	#if( configUSE_PREEMPTION != 0 )
	{
		/* Only created when preemption is used. */
		vTimerPeriodicISRTests();
	}
	#endif

	/* Call the periodic queue overwrite from ISR demo. */
	vQueueOverwritePeriodicISRDemo();

	/* Write to a queue that is in use as part of the queue set demo to
	demonstrate using queue sets from an ISR. */
	vQueueSetAccessQueueSetFromISR();
	vQueueSetPollingInterruptAccess();

	/* Exercise event groups from interrupts. */
	vPeriodicEventGroupsProcessing();

	/* Exercise giving mutexes from an interrupt. */
	vInterruptSemaphorePeriodicTest();

	/* Exercise using task notifications from an interrupt. */
	xNotifyTaskFromISR();

	/* Writes to stream buffer byte by byte to test the stream buffer trigger
	level functionality. */
	vPeriodicStreamBufferProcessing();

	/* Writes a string to a string buffer four bytes at a time to demonstrate
	a stream being sent from an interrupt to a task. */
	vBasicStreamBufferSendFromISR();

	/* For code coverage purposes. */
	xTimerTask = xTimerGetTimerDaemonTaskHandle();
	configASSERT( uxTaskPriorityGetFromISR( xTimerTask ) == configTIMER_TASK_PRIORITY );
}
