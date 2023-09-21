// *** util.c *********************************************************
// this file implements generic utility functions.

#include "main.h"

void
write32(byte *addr, uint32 data)
{
    *(byte *)(addr+0) = data & 0xff;
    *(byte *)(addr+1) = (data>>8) & 0xff;
    *(byte *)(addr+2) = (data>>16) & 0xff;
    *(byte *)(addr+3) = (data>>24) & 0xff;
}

uint32
read32(const byte *addr)
{
    return *(const byte *)(addr+3) << 24 |
           *(const byte *)(addr+2) << 16 |
           *(const byte *)(addr+1) << 8 |
           *(const byte *)(addr+0);
}

void
write16(byte *addr, uint16 data)
{
    *(byte *)(addr+0) = data & 0xff;
    *(byte *)(addr+1) = (data>>8) & 0xff;
}

uint16
read16(const byte *addr)
{
    return *(const byte *)(addr+1) << 8 |
           *(const byte *)(addr+0);
}

uint32
read_n_bytes(int size, const volatile void *addr)
{
    switch (size) {
        case 4:
            return *(volatile uint32 *)addr;
        case 2:
            return *(volatile uint16 *)addr;
        case 1:
            return *(volatile uint8 *)addr;
        default:
            assert(0);
            return 0;
    }
}

void
write_n_bytes(int size, volatile void *addr, uint32 value)
{
    switch (size) {
        case 4:
            *(volatile uint32 *)addr = value;
            break;
        case 2:
            *(volatile uint16 *)addr = value;
            break;
        case 1:
            *(volatile uint8 *)addr = value;
            break;
        default:
            assert(0);
            break;
    }
}

uint32
byteswap(uint32 x, uint32 size)
{
    // byteswap all bytes of x within size
    switch (size) {
        case 4:
            return ((x)&0xff)<<24|((x)&0xff00)<<8|((x)&0xff0000)>>8|((x)&0xff000000)>>24;
        case 2:
            return ((x)&0xff)<<8|((x)&0xff00)>>8;
        case 1:
            return (x)&0xff;
        default:
            assert(0);
            break;
    }
    return x;
}

// return the current interrupt mask level
int
gpl(void)
{
#if ! _WIN32
    int oldlevel;

    oldlevel = (_CP0_GET_STATUS() >> 10) & 7;
#else
    short csr;
    int oldlevel;

    // get the sr
    csr = get_sr();

    oldlevel = (csr >> 8) & 7;
#endif
    return oldlevel;
}

// set the current interrupt mask level and return the old one
int
splx(int level)
{
#if ! _WIN32
    int csr;
    int oldlevel;

    // get the sr
    csr = _CP0_GET_STATUS();


    oldlevel = (csr >> 10) & 7;
    if (level <= 0) {
        // we're going down
        level = -level;
    } else {
        // we're going up
        level = MAX(level, oldlevel);
    }
    assert(level >= 0 && level <= 7);
    csr = (csr & 0xffffe3ff) | (level << 10);

    // update the sr
    _CP0_SET_STATUS(csr);

    assert(oldlevel >= 0 && oldlevel <= 7);
    return -oldlevel;
#else
    short csr;
    int oldlevel;

    // get the sr
    csr = get_sr();

    oldlevel = (csr >> 8) & 7;
    if (level <= 0) {
        // we're going down
        level = -level;
    } else {
        // we're going up
        level = MAX(level, oldlevel);
    }
    assert(level >= 0 && level <= 7);
    csr = (csr & 0xf8ff) | (level << 8);

    // update the sr
    set_sr(csr);

    assert(oldlevel >= 0 && oldlevel <= 7);
    return -oldlevel;
#endif
}

static volatile int g;

int blips_per_ms;

void
blip(void)
{
    int x;
    for (x = 0; x < 500; x++) {
        g++;
    }
}

// delay for the specified number of milliseconds
void
delay(int32 ms)
{
#if STICK_GUEST
    // we sleep quicker for unit tests
    ms = isatty(0)?(ms):(ms)/10;
#if _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
#else // ! STICK_GUEST
    int x;
    int32 m;
    int blips;

    // if interrupts are initialized...
    if (gpl() < SPL_PIT0) {
        // wait for the pit0 to count off the ticks
        m = msecs;
        blips = 0;
        while (msecs-m < ms+1) {
            blip();
            blips++;
        }
        if (! blips_per_ms) {
            blips_per_ms = blips/ms+1;
        }
    // otherwise; make a good guess with a busywait
    } else {
        assert(blips_per_ms);
        while (ms--) {
            for (x = 0; x < blips_per_ms; x++) {
                blip();
            }
        }
    }
#endif // ! STICK_GUEST
}

int
gethex(char *p)
{
    char c;

    c = *p;
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else {
        return -1;
    }
}

int
get2hex(char **p)
{
    int v1, v0;

    v1 = gethex(*p);
    if (v1 == -1) {
        return -1;
    }
    v0 = gethex(*p+1);
    if (v0 == -1) {
        return -1;
    }

    (*p) += 2;
    return v1*16+v0;
}

void
tailtrim(char *text)
{
    char *p;

    p = strchr(text, '\0');
    while (p > text && isspace(*(p-1))) {
        p--;
    }
    *p = '\0';
}

void *
memcpy(void *d,  const void *s, size_t n)
{
#if MCU_CORE_BITS >= 32
    if (((uintptr)d&3)+((uintptr)s&3)+(n&3) == 0) {
        uint32 *dtemp = d;
        const uint32 *stemp = s;

        n >>= 2;
        while (n--) {
            *(dtemp++) = *(stemp++);
        }
    } else {
#endif
        uint8 *dtemp = d;
        const uint8 *stemp = s;

        while (n--) {
            *(dtemp++) = *(stemp++);
        }
#if MCU_CORE_BITS >= 32
    }
#endif
    return d;
}

void *
memmove(void *d,  const void *s, size_t n)
{
    void *dd;

    if ((char *)d > (char *)s && (char *)d < (char *)s+n) {
        dd = d;
        while (n--) {
            *((char *)d+n) = *((char *)s+n);
        }
        return dd;
    } else {
        return memcpy(d, s, n);
    }
}

void *
memset(void *p,  int d, size_t n)
{
#if MCU_CORE_BITS >= 32
    int dd;

    if (((uintptr)p&3)+(n&3) == 0) {
        uint32 *ptemp = p;

        n >>= 2;
        d = d & 0xff;
        dd = d<<24|d<<16|d<<8|d;
        while (n--) {
            *(ptemp++) = dd;
        }
    } else {
#endif
        uint8 *ptemp = p;

        while (n--) {
            *(ptemp++) = d;
        }
#if MCU_CORE_BITS >= 32
    }
#endif
    return p;
}

int
memcmp(const void *d,  const void *s, size_t n)
{
    char c;
    const uint8 *dtemp = d;
    const uint8 *stemp = s;

    while (n--) {
        c = *(dtemp++) - *(stemp++);
        if (c) {
            return c;
        }
    }
    return 0;
}

size_t
strlen(const char *s)
{
    const char *stemp = s;

    while (*stemp) {
        stemp++;
    }
    return stemp - s;
}

char *
strcat(char *dest, const char *src)
{
    char *orig_dest = dest;

    while (*dest) {
        dest++;
    }
    do {
        *(dest++) = *src;
    } while (*(src++));

    return orig_dest;
}

char *
strncat(char *dest, const char *src, size_t n)
{
    char *orig_dest = dest;

    while (*dest) {
        dest++;
    }
    while (n-- && *src) {
        *(dest++) = *(src++);
    }
    *dest = '\0';

    return orig_dest;
}

char *
strcpy(char *dest, const char *src)
{
    char *orig_dest = dest;

    do {
        *(dest++) = *src;
    } while (*(src++));

    return orig_dest;
}

char *
strncpy(char *dest, const char *src, size_t n)
{
    char *orig_dest = dest;

    while (n--) {
        *(dest++) = *src;
        if (! *(src++)) {
            break;
        }
    }

    return orig_dest;
}

int
strcmp(const char *s1, const char *s2)
{
    for (; *s1 == *s2; s1++, s2++) {
        if (! *s1) {
            return 0;
        }
    }

    if (*s1 < *s2) {
        return -1;
    } else {
        return 1;
    }
}

int
strncmp(const char *s1, const char *s2, size_t n)
{
    for (; n && (*s1 == *s2); n--, s1++, s2++) {
        if (! *s1) {
            return 0;
        }
    }

    if (n <= 0) {
        return 0;
    }

    if (*s1 < *s2) {
        return -1;
    } else {
        return 1;
    }
}

char *
strstr(const char *s1, const char *s2)
{
    int len;

    len = strlen(s2);
    while (*s1) {
        if (! strncmp(s1, s2, len)) {
            return (char *)s1;
        }
        s1++;
    }
    return NULL;
}

char *
strchr(const char *s, int c)
{
    for (; *s; s++) {
        if (*s == c) {
            return (char *)s;
        }
    }
    if (*s == c) {
        return (char *)s;
    }
    return NULL;
}

int
isdigit(int c)
{
    return (c >= '0') && (c <= '9');
}

int
isspace(int c)
{
    return (c == ' ') || (c == '\f') || (c == '\n') || (c == '\r') || (c == '\t') || (c == '\v');
}

int
isupper(int c)
{
    return (c >= 'A') && (c <= 'Z');
}

int
islower(int c)
{
    return (c >= 'a') && (c <= 'z');
}

int
isalpha(int c)
{
    return isupper(c) || islower(c);
}

int
isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}

int
isprint(int c)
{
    return (c >= ' ') && (c <= '~');
}

#if STICK_GUEST
static uint16 sr = 0x2000;
#endif

#if _WIN32
uint16
get_sr(void)
{
    uint16 csr;
#if STICK_GUEST
    csr = sr;
#endif
    assert(csr & 0x2000);
    return csr;
}

void
set_sr(uint16 csr)
{
    assert(csr & 0x2000);
#if STICK_GUEST
    sr = csr;
#endif
}
#endif

static uint32 m_w = 1;    /* must not be zero */
static uint32 m_z = 2;    /* must not be zero */

uint32
random_32(
    void
    )
{
    uint32 z;
    uint32 w;

    z = m_z;
    w = m_w;
    assert(z);
    assert(w);
    m_z = z = 36969 * (z & 65535) + (z >> 16);
    m_w = w = 18000 * (w & 65535) + (w >> 16);
    return (z << 16) + w;  /* 32-bit result */
}
