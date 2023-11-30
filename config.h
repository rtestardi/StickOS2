// *** config.h *******************************************************

#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED  1

#define FLASH_START  0x9D000000

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define FLASH_BYTES  (512*1024)  // the smallest part we support
#define FLASH_PAGE_SIZE  4096
#define BASIC_RAM_PAGE_SIZE  4096
#define BASIC_VARS  200
#define BASIC_STORES  10
#define SCOPE  1

#else
#error

#endif

#if ! STICK_GUEST
#if __PIC32_SRS_SET_COUNT == 1
#define PIC32IPL4  IPL4SOFT
#elif __PIC32_SRS_SET_COUNT == 2
#define PIC32IPL4  IPL4AUTO
#elif __PIC32_SRS_SET_COUNT == 8
#define PIC32IPL4  IPL4SRS
#else
#error
#endif
#endif

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define FLASH2_START  0x9FC00000  // boot flash, for flash upgrade
#define FLASH2_BYTES  (20*1024)
#else
#error
#endif

#if SODEBUG && ! STICK_GUEST
#define BASIC_LARGE_PAGE_SIZE  (12*1024)
#else
#define BASIC_LARGE_PAGE_SIZE  (24*1024)
#endif

#define BASIC_SMALL_PAGE_SIZE  FLASH_PAGE_SIZE

#endif
