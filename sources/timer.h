// *** timer.h ********************************************************

enum { ticks_per_msec = 4 }; // tunable

extern volatile int32 ticks;  // incremented by pit0 isr every tick
extern volatile int32 msecs;  // incremented by pit0 isr every millisecond
extern volatile int32 seconds;  // incremented by pit0 isr every second

extern bool volatile timer_in_isr;

#if STICK_GUEST
void
timer_ticks(bool align);
#endif

void
timer_initialize(void);

