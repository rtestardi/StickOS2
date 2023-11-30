#include <xc.h>
#include <sys/kmem.h>
#include <sys/attribs.h>

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#undef __ISR
#define __ISR(v,ipl) __attribute__((vector(v), interrupt(ipl), micromips))
#define EXCEPTION_HANDLER  __attribute__((micromips))
#else
#define EXCEPTION_HANDLER
#endif

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define ReadADC10(bufIndex) ((uint16_t) (*((&ADCDATA12) + (bufIndex << 2))))
#else
#error
#endif

/*********************************************************************
 * Function:        void cheKseg0CacheOn(void)
 *
 * Overview:        This routine is used to enable cacheability of KSEG0.
 * Description:     Sets the K0 field in Config register 16 select 0,
 * 					of Co-processor 0 to the value "011"b
 * PreCondition:    None
 * Input:           None
 * Output:          none
 * 
 * Note:			There is some errata warning about Cache and Prefetch enabled together.
 *
 ********************************************************************/
#if (defined __PIC32MX__) || (defined __PIC32MK__)
#ifndef _DMA_H_
static inline 
void __attribute__ ((nomips16)) CheKseg0CacheOn()
{
	register unsigned long tmp;
	asm("mfc0 %0,$16,0" :  "=r"(tmp));
	tmp = (tmp & ~7) | 3;
	asm("mtc0 %0,$16,0" :: "r" (tmp));
}
#endif
#endif

/*******************************************************************************
 * Function:        int DmaSuspend(void)
 *
 * PreCondition:    None
 * Input:			None
 * Output:          true if the DMA was previously suspended, false otherwise
 *
 * Side Effects:    None
 *
 * Overview:        The function suspends the DMA controller.
 *
 * Note:            After the execution of this function the DMA operation is supposed to be suspended.
 *                  I.e. the function has to wait for the suspension to take place!
 *
 * Example:         int susp = DmaSuspend();
 ******************************************************************************/
#ifndef _DMA_H_
static __inline__ 
int __attribute__((always_inline)) DmaSuspend(void)
{
  #ifdef _DMACON_SUSPEND_MASK
	int suspSt;
	suspSt = (DMACON & _DMACON_SUSPEND_MASK) >> _DMACON_SUSPEND_POSITION;
	if (!suspSt )	// Previous state.
	{
		DMACONSET = _DMACON_SUSPEND_MASK;	// suspend
	  #ifdef _DMACON_DMABUSY_MASK
		while(DMACON & _DMACON_DMABUSY_MASK);	// wait to be actually suspended.
	  #endif
	}
	return suspSt;
  #else									/* Some devices have no DMA. */
	return 0;
  #endif
}
#endif	// _DMA_H_

/*********************************************************************
 * Function:        void DmaResume(int susp)
 *
 * PreCondition:    None
 *
 * Input:			the desired DMA suspended state.
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:		The function restores the DMA controller activity to the old suspended mode.
 *
 * Note:            Some devices do Not have DMA.
 *
 * Example:			int susp = DmaSuspend();
 *					{....};
 *					DmaResume(susp);
 ******************************************************************************/
#ifndef _DMA_H_
static __inline__ 
void __attribute__((always_inline)) DmaResume(int susp)
{
  #ifdef _DMACON_SUSPEND_MASK
	if (susp)
	{
		DmaSuspend();
	}
	else
	{
		DMACONCLR = _DMACON_SUSPEND_MASK;	// resume DMA activity
	}
  #endif
}
#endif	// _DMA_H_

#define _DMA_H_

unsigned int SYSTEMConfigPerformance(unsigned int sys_clock);

void __attribute__ ((nomips16)) INTEnableSystemMultiVectoredInt(void);
