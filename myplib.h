#include <xc.h>
#include <sys/kmem.h>
#include <sys/attribs.h>

unsigned int __attribute__((nomips16))  INTEnableInterrupts(void);
void __attribute__ ((nomips16)) INTEnableSystemMultiVectoredInt(void);

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#undef __ISR
#define __ISR(v,ipl) __attribute__((vector(v), interrupt(ipl), micromips))
#define EXCEPTION_HANDLER  __attribute__((micromips))
#else
#error
#endif

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define ReadADC10(bufIndex) ((uint16_t) (*((&ADCDATA12) + (bufIndex << 2))))
#else
#error
#endif
