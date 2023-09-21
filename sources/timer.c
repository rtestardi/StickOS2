// *** timer.c ********************************************************
// this file implements the core interval timer used internally.

#include "main.h"
#if STICK_GUEST && __unix__
#include <signal.h>
#include <sys/time.h>
#endif

#if SODEBUG
volatile bool timer_in_isr;
volatile int32 timer_in_ticks;
volatile int32 timer_out_ticks;
volatile int32 timer_max_ticks;
#endif

volatile int32 ticks;  // incremented by pit0 isr every tick
volatile int32 msecs;  // incremented by pit0 isr every millisecond
volatile int32 seconds;  // incremented by pit0 isr every second

volatile static byte msecs_in_second;  // incremented by pit0 isr every millisecond
volatile static byte eighths_in_second;  // incremented by pit0 isr 8 times per second

enum { msecs_per_debounce = 4 }; // tunable

// called by pit0 every tick
void
#if ! _WIN32
__ISR(_TIMER_1_VECTOR, PIC32IPL4)
#endif
timer_isr(void)
{
    bool debouncing;

    assert(! timer_in_isr);
    assert((timer_in_isr = true) ? true : true);
    assert((timer_in_ticks = ticks) ? true : true);

    // clear the interrupt flag
#if ! STICK_GUEST
    IFS0CLR = _IFS0_T1IF_MASK;
#endif // ! STICK_GUEST

    ticks++;

    // If a msec elapsed...
    if ((ticks & (ticks_per_msec - 1)) == 0) {
        msecs++;
        msecs_in_second++;

        debouncing = (msecs & (msecs_per_debounce - 1)) == 0;

        if (debouncing) {
            // poll debounced digital inputs.
            pin_timer_poll();
        }

        // if an an eighth of a second has elapsed...
        if (msecs_in_second == 125) {
            msecs_in_second = 0;

            eighths_in_second++;

            // if a second has elapsed...
            if ((eighths_in_second&7) == 0) {
                seconds++;
            }

            // poll the LEDs 8 times a second
            led_timer_poll();
        }

        // profile every msec while running
        if (running) {
            code_timer_poll();
        }

    } else {
        debouncing = false;
    }

    // poll the adc to occasionally record debouncing data and to record adc data every tick.
    adc_timer_poll(debouncing);

    assert(timer_in_isr);
    assert((timer_in_isr = false) ? true : true);
    assert((timer_out_ticks = ticks) ? true : true);
    assert((timer_max_ticks = MAX(timer_max_ticks, timer_out_ticks-timer_in_ticks)) ? true : true);
}

#if STICK_GUEST
static int32 oms;  // msecs already accounted for
static int32 tbd;  // ticks to be delivered

static
int32
timer_ms(void)
{
    int32 ms;

#if _WIN32
    ms = GetTickCount();
#else
    ms = times(NULL)*10;
#endif
    ms *= isatty(0)?1:10;

    return ms;
}

// deliver timer ticks for STICK_GUEST builds
void
timer_ticks(bool align)
{
    int x;
    int32 ms;

    ms = timer_ms();

    // if we are starting a run...
    if (align) {
        // wait for a time change
        while (timer_ms() == ms) {
            // NULL
        }
        ms = timer_ms();
    }

    // if the time has changed...
    if (ms != oms) {
        // schedule the ticks for delivery
        tbd += (ms-oms)*ticks_per_msec;
        oms = ms;
    }

    // deliver the ticks
    while (tbd) {
        x = splx(5);
        timer_isr();
        splx(x);
        tbd--;

        // N.B. if we're runnning (vs. starting a run) we only deliver one tick per call
        if (! align) {
            break;
        }
    }
}
#endif

// this function initializes the timer module.
void
timer_initialize(void)
{
    assert(IS_POWER_OF_2(ticks_per_msec));
    assert(IS_POWER_OF_2(msecs_per_debounce));

#if ! STICK_GUEST
    // configure t1 to interrupt every ticks times per msec.
    T1CONCLR = _T1CON_ON_MASK;
    T1CON = (1 << _T1CON_TCKPS_POSITION);  // 1:8 prescale
    TMR1 = 0;
    PR1 = bus_frequency/8/1000/ticks_per_msec - 1;
    T1CONSET = _T1CON_ON_MASK;

    // set up the timer interrupt with a priority of 4
    IEC0bits.T1IE = 1;
    IPC1bits.T1IP = 4;
    IPC1bits.T1IS = 0;
#else  // ! STICK_GUEST
    oms = timer_ms();
#endif // ! STICK_GUEST
}

