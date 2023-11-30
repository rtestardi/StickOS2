// *** main.h *********************************************************

#ifndef MAIN_INCLUDED

#define VERSION  "2.27c"

//#define SODEBUG  1
#define DEBUGGING  1  // enable to use mplab x debugger

#define ZIGFLEA  1

#define UPGRADE  1

// Enable in-memory trace buffer for debugging with the trace() macro.
#define IN_MEMORY_TRACE 0

#if STICK_GUEST
#define __32MK0512GPK064__  1
#else
#define NULL ((void*)0)
#endif

#if ! STICK_GUEST

#include <myplib.h>
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;

#define MCU_CORE_BITS 32
typedef int intptr;
typedef unsigned int uintptr;

#define asm_halt()  asm("SDBBP 0");

#else  // STICK_GUEST

#if WIN32

// _DEBUG/NDEBUG win
#if _DEBUG
#define SODEBUG  1
#else
#if NDEBUG
#define SODEBUG  0
#else
#error _DEBUG/NDEBUG?
#endif
#endif  // _DEBUG

#else  // WIN32

// SODEBUG wins
#if SODEBUG
#define _DEBUG
#undef NDEBUG
#else
#define NDEBUG
#undef _DEBUG
#endif  // SODEBUG

#endif  // WIN32

#if GCC

#include <inttypes.h>
#include <bits/wordsize.h>
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uintptr_t uintptr;
typedef intptr_t intptr;
typedef uint32 size_t;

#else // GCC

#define _WIN32_WINNT 0x0601
#include <windows.h>
extern int isatty(int);
#if ! NO_UINT_TYPEDEFS
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef  int int32;
typedef unsigned int uint32;
typedef int intptr;
typedef unsigned int uintptr;
#endif
#define NO_UINT_TYPEDEFS  1

#endif // ! GCC

#include <assert.h>
#define ASSERT(x)  assert(x)
#define assert_ram(x)  assert(x)

extern void write(int, const void *, size_t);
extern char *gets(char *);

#define inline
#undef MAX_PATH

#endif  // ! STICK_GUEST

typedef unsigned char bool;
typedef unsigned char byte;
typedef unsigned int uint;

enum {
    false,
    true
};

#define IN
#define OUT
#define OPTIONAL
#define VARIABLE  1
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define ROUNDUP(n, s)  (((n)+(s)-1)&~((s)-1))  // N.B. s must be power of 2!
#define ROUNDDOWN(n, s)  ((n)&~((s)-1))  // N.B. s must be power of 2!
#define LENGTHOF(a)  (sizeof(a)/sizeof(a[0]))
#define OFFSETOF(t, f)  ((int)(intptr)(&((t *)0)->f))
#define IS_POWER_OF_2(x) ((((x)-1)&(x))==0)

#define BASIC_OUTPUT_LINE_SIZE  79
#define BASIC_INPUT_LINE_SIZE  72

#define vsnprintf  myvsnprintf
#define snprintf  mysnprintf
#define sprintf  mysprintf
#define vsprintf  myvsprintf
#define printf  myprintf
#define sprintf_s mysprintf_s
#define snprintf_s mysnprintf_s

#include <stdarg.h>

#include "flash.h"
#include "pin.h"
#include "printf.h"
#include "qspi.h"
#include "i2c.h"
#include "zigflea.h"
#include "terminal.h"
#include "text.h"
#include "timer.h"
#include "util.h"
#include "adc.h"
#include "led.h"
#include "serial.h"

#include "config.h"

#if ! STICK_GUEST

#include "cdcacm.h"
#include "usb.h"

#endif  // ! STICK_GUEST

#include "cpustick.h"
#include "basic.h"
#include "code.h"
#include "parse.h"
#include "run.h"
#include "vars.h"
#include "basic0.h"
#include "parse2.h"
#include "run2.h"
#include "scope.h"

#define os_yield()  // NULL

int
vprintf(const char *format, va_list ap);

#if ! STICK_GUEST

#define CASSERT(predicate) _impl_CASSERT_LINE(predicate,__LINE__)
#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line) \
    typedef char _impl_PASTE(cassert_failed_line##_,line)[2*!!(predicate)-1];

// usable in function body
#define cassert(predicate)  do { CASSERT(predicate); } while (0)

#if SODEBUG
#define assert(x)  do { if (! (x)) { led_line(__LINE__); } } while (0)
#else
#define assert(x)
#endif
#define assert_ram(x)  do { if (! (x)) { asm_halt(); } } while (0)
#define ASSERT(x)  do { if (! (x)) { led_line(__LINE__); } } while (0)
#define ASSERT_RAM(x)  do { if (! (x)) { asm_halt(); } } while (0)

#else  // STICK_GUEST

extern byte big_buffer[768];

#endif  // ! STICK_GUEST

extern bool disable_autorun;
extern uint16 flash_checksum;

extern byte *end_of_static;

extern uint32 cpu_frequency;
extern uint32 bus_frequency;
extern uint32 oscillator_frequency;

extern bool debugger_attached;
extern byte big_buffer[768];

typedef void (*flash_upgrade_ram_begin_f)(void);

void
#if ! _WIN32
__longramfunc__
__attribute__((nomips16))
#endif
flash_upgrade_ram_begin(void);

void
flash_upgrade_ram_end(void);

#define MAIN_INCLUDED  1
#endif  // MAIN_INCLUDED

