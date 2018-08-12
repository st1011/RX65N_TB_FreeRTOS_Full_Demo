/*
 * This file contains the non-portable and therefore RX65N specific parts of
 * the IntQueue standard demo task - namely the configuration of the timers
 * that generate the interrupts and the interrupt entry points.
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo includes. */
#include "IntQueueTimer.h"
#include "IntQueue.h"

/* Renesas includes. */
#include "r_smc_entry.h"

#define tmrTPU4_FREQUENCY	( 2000UL )
#define tmrTPU5_FREQUENCY	( 2001UL )

void vInitialiseTimerForIntQueueTest( void )
{
	/* Ensure interrupts do not start until full configuration is complete. */
	portENTER_CRITICAL();
	{
	    /* Enable writing to registers related to operating modes, LPC, CGC and software reset */
	    SYSTEM.PRCR.WORD = 0xA50BU;

	    /* Enable writing to MPC pin function control registers */
	    MPC.PWPR.BIT.B0WI = 0U;
	    MPC.PWPR.BIT.PFSWE = 1U;

	    /* Set peripheral settings */
	    // TPU4
	    {
	        /* Release TPU channel 4 from stop state */
	        MSTP(TPU4) = 0U;

	        /* Stop TPU channel 4 counter */
	        TPUA.TSTR.BIT.CST4 = 0U;

	        /* Set TGI4A interrupt priority level */
	        ICU.SLIBR160.BYTE = 0x21U;
	        IPR(PERIB, INTB160) = configMAX_SYSCALL_INTERRUPT_PRIORITY - 1;

	        /* TPU channel 4 is used as normal mode */
	        TPUA.TSYR.BIT.SYNC4 = 0U;
	        TPU4.TCR.BYTE = 0x01U | 0x08U | 0x20U;
	        TPU4.TIER.BYTE |= 0x01 | 0x40;
	        TPU4.TIOR.BYTE = 0x00;
	        TPU4.TGRA = ( unsigned short ) ( ( ( configPERIPHERAL_CLOCK_HZ / tmrTPU4_FREQUENCY ) -1 ) / 4 );
	    }

	    // TPU5
	    {
	    	/* Release TPU channel 5 from stop state */
	        MSTP(TPU5) = 0U;

	        /* Stop TPU channel 5 counter */
	        TPUA.TSTR.BIT.CST5 = 0U;

	        /* Set TGI5A interrupt priority level */
	        ICU.SLIBR164.BYTE = 0x25U;
	        IPR(PERIB, INTB164) = configMAX_SYSCALL_INTERRUPT_PRIORITY - 2;

	        /* TPU channel 5 is used as normal mode */
	        TPUA.TSYR.BIT.SYNC5 = 0U;
	        TPU5.TCR.BYTE = 0x01U | 0x08U | 0x20U;
	        TPU5.TIER.BYTE |= 0x01 | 0x40;
	        TPU5.TIOR.BYTE = 0x00;
	        TPU5.TGRA = ( unsigned short ) ( ( ( configPERIPHERAL_CLOCK_HZ / tmrTPU5_FREQUENCY ) -1 ) / 4 );
	    }

	    /* Disable writing to MPC pin function control registers */
	    MPC.PWPR.BIT.PFSWE = 0U;
	    MPC.PWPR.BIT.B0WI = 1U;

	    /* Enable protection */
	    SYSTEM.PRCR.WORD = 0xA500U;

	    /* Enable TGI4A interrupt in ICU */
	    IEN(PERIB, INTB160) = 1U;

	    /* Start TPU channel 4 counter */
	    TPUA.TSTR.BIT.CST4 = 1U;

	    /* Enable TGI5A interrupt in ICU */
	    IEN(PERIB, INTB164) = 1U;

	    /* Start TPU channel 5 counter */
	    TPUA.TSTR.BIT.CST5 = 1U;
	}
	portEXIT_CRITICAL();
}
/*-----------------------------------------------------------*/

#if FAST_INTERRUPT_VECTOR == VECT_PERIB_INTB160
#pragma interrupt r_Config_TPU4_tgi4a_interrupt(vect=VECT(PERIB,INTB160),fint)
#else
#pragma interrupt r_Config_TPU4_tgi4a_interrupt(vect=VECT(PERIB,INTB160))
#endif
static void r_Config_TPU4_tgi4a_interrupt(void)
{
	portYIELD_FROM_ISR( xFirstTimerHandler() );
}
/*-----------------------------------------------------------*/


#if FAST_INTERRUPT_VECTOR == VECT_PERIB_INTB164
#pragma interrupt r_Config_TPU5_tgi5a_interrupt(vect=VECT(PERIB,INTB164),fint)
#else
#pragma interrupt r_Config_TPU5_tgi5a_interrupt(vect=VECT(PERIB,INTB164))
#endif
static void r_Config_TPU5_tgi5a_interrupt(void)
{
	portYIELD_FROM_ISR( xSecondTimerHandler() );
}
/*-----------------------------------------------------------*/
