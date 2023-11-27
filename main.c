// *** main.c *********************************************************
// this is the main program that is launched by startup.c; it just
// initializes all of the modules of the os and then runs the main
// application program loop.

#include "main.h"

#if ! STICK_GUEST
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)


// PIC32MK0512GPK064 Configuration Bit Settings

// 'C' source line config statements

// DEVCFG3
#pragma config USERID = 0xFFFF          // Enter Hexadecimal value (Enter Hexadecimal value)
#pragma config FUSBIDIO2 = OFF          // USB2 USBID Selection (USBID pin is controlled by the port function)
#pragma config FVBUSIO2 = OFF           // USB2 VBUSON Selection bit (VBUSON pin is controlled by the port function)
#pragma config PGL1WAY = ON             // Permission Group Lock One Way Configuration bit (Allow only one reconfiguration)
#pragma config PMDL1WAY = ON            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO1 = OFF          // USB1 USBID Selection (USBID pin is controlled by the port function)
#pragma config FVBUSIO1 = OFF           // USB2 VBUSON Selection bit (VBUSON pin is controlled by the port function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_1         // System PLL Input Divider (1x Divider)
#pragma config FPLLRNG = RANGE_8_16_MHZ // System PLL Input Range (8-16 MHz Input)
#pragma config FPLLICLK = PLL_POSC      // System PLL Input Clock Selection (POSC is input to the System PLL)
#pragma config FPLLMULT = MUL_40        // System PLL Multiplier (PLL Multiply by 40)
#pragma config FPLLODIV = DIV_4         // System PLL Output Clock Divider (4x Divider)
#pragma config BORSEL = HIGH            // Brown-out trip voltage (BOR trip voltage 2.1v (Non-OPAMP deviced operation))
#pragma config UPLLEN = ON              // USB PLL Enable (USB PLL Enabled)

// DEVCFG1
#pragma config FNOSC = SPLL             // Oscillator Selection Bits (System PLL)
#pragma config DMTINTV = WIN_127_128    // DMT Count Window Interval (Window/Interval value is 127/128 counter value)
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disable Secondary Oscillator)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (HS osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disabled, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WDTSPGM = STOP           // Watchdog Timer Stop During Flash Programming (WDT stops during Flash programming)
#pragma config WINDIS = NORMAL          // Watchdog Timer Window Mode (Watchdog Timer is in non-Window mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled)
#pragma config FWDTWINSZ = WINSZ_25     // Watchdog Timer Window Size (Window size is 25%)
#pragma config DMTCNT = DMT31           // Deadman Timer Count Selection (2^31 (2147483648))
#pragma config FDMTEN = OFF             // Deadman Timer Enable (Deadman Timer is disabled)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config TRCEN = OFF              // Trace Enable (Trace features in the CPU are disabled)
#pragma config BOOTISA = MIPS32         // Boot ISA Selection (Boot code and Exception code is MIPS32)
#pragma config FECCCON = ECC_DECC_DISABLE_ECCON_WRITABLE// Dynamic Flash ECC Configuration Bits (ECC and Dynamic ECC are disabled (ECCCON<1:0> bits are writable))
#pragma config FSLEEP = OFF             // Flash Sleep Mode (Flash is powered down when the device is in Sleep mode)
#pragma config DBGPER = PG_ALL          // Debug Mode CPU Access Permission (Allow CPU access to all permission regions)
#pragma config SMCLR = MCLR_NORM        // Soft Master Clear Enable (MCLR pin generates a normal system Reset)
#pragma config SOSCGAIN = G3            // Secondary Oscillator Gain Control bits (Gain is G3)
#pragma config SOSCBOOST = ON           // Secondary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCGAIN = G3            // Primary Oscillator Coarse Gain Control bits (Gain Level 3 (highest))
#pragma config POSCBOOST = ON           // Primary Oscillator Boost Kick Start Enable bit (Boost the kick start of the oscillator)
#pragma config POSCFGAIN = G3           // Primary Oscillator Fine Gain Control bits (Gain is G3)
#pragma config POSCAGCDLY = AGCRNG_x_25ms// AGC Gain Search Step Settling Time Control (Settling time = 25ms x AGCRNG)
#pragma config POSCAGCRNG = ONE_X       // AGC Lock Range bit (Range 1x)
#pragma config POSCAGC = Automatic      // Primary Oscillator Gain Control bit (Automatic Gain Control for Oscillator)
#pragma config EJTAGBEN = NORMAL        // EJTAG Boot Enable (Normal EJTAG functionality)

// DEVCP
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// SEQ
#pragma config TSEQ = 0x0               // Boot Flash True Sequence Number (Enter Hexadecimal value)
#pragma config CSEQ = 0xFFFF            // Boot Flash Complement Sequence Number (Enter Hexadecimal value)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#else
    #pragma config UPLLEN   = ON            // USB PLL Enabled
#if defined(__32MX250F128B__)
    #pragma config FPLLMUL  = MUL_24        // PLL Multiplier
#else
    #pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#endif
    #pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
    #pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#if defined(__32MX250F128B__)
    #pragma config FPLLODIV = DIV_2         // PLL Output Divider
#else
    #pragma config FPLLODIV = DIV_1         // PLL Output Divider
#endif
#if defined(__32MX250F128B__)
    #pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#else
    #pragma config FPBDIV   = DIV_2         // Peripheral Clock divisor
#endif
    #pragma config FWDTEN   = OFF           // Watchdog Timer
    #pragma config WDTPS    = PS1           // Watchdog Timer Postscale
    #pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
    #pragma config OSCIOFNC = OFF           // CLKO Enable
    #pragma config POSCMOD  = XT            // Primary Oscillator
    #pragma config IESO     = OFF           // Internal/External Switch-over
    #pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable
    #pragma config FNOSC    = PRIPLL        // Oscillator Selection
    #pragma config CP       = OFF           // Code Protect
    #pragma config BWP      = OFF           // Boot Flash Write Protect
    #pragma config PWP      = OFF           // Program Flash Write Protect
#if defined(__32MX250F128B__)
    #pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#else
    #pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select
#endif
    #pragma config DEBUG    = OFF           // Debugger Disabled for Starter Kit
#endif

#if defined(__32MX250F128B__)
// XXX -- delete me?
//#pragma config PMDL1WAY = OFF
//#pragma config IOL1WAY = OFF
#pragma config JTAGEN = OFF
#pragma config FVBUSONIO = OFF  // give back vbuson and usbid pins
#pragma config FUSBIDIO = OFF  // give back vbuson and usbid pins
#endif
#endif

#if ! STICK_GUEST
EXCEPTION_HANDLER
void _general_exception_context(void)
{
    led_hex(_CP0_GET_CAUSE());
}

EXCEPTION_HANDLER
void _bootstrap_exception_handler(void)
{
    led_hex(_CP0_GET_CAUSE());
}
#endif

bool disable_autorun;
uint16 flash_checksum;

byte *end_of_static;

uint32 cpu_frequency;
uint32 oscillator_frequency;
uint32 bus_frequency;

bool debugger_attached;
// N.B. big_buffer may not be aligned; it is up to the caller to align it if needed.
byte big_buffer[768];

extern int (_ebase_address)();
extern int (_on_bootstrap)();

int
main()  // we're called directly by startup.c
{
    int x;
    byte *p;

    TRISACLR = 0x410;
    LATASET = 0x410;

#if defined(__32MX250F128B__)
    SYSTEMConfigPerformance(48000000L);
#elif defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    /* unlock system for clock configuration */
    SYSKEY = 0x00000000;
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;

    /* Configure UPLL */
    /* UPOSCEN = UPLL */
    /* PLLODIV = DIV_8 */
    /* PLLMULT = MUL_32 */
    /* PLLIDIV = DIV_1 */
    /* PLLRANGE = RANGE_8_16_MHZ */
    UPLLCON = 0x31f0002;

    /* Lock system since done with clock configuration */
    SYSKEY = 0x33333333;

//#ifdef SODEBUG
    // set clock testpoints
    RPD5R = 0x12;  // REFCLKO4/RD5
    REFO4CON = 0x2009000;  // SYSCLK/2/512
    RPC8R = 0x12;  // REFCLKO3/RC8
    REFO3CON = 0x2009006;  // UPLL/2/512
//#endif

    /* Configure CP0.K0 for optimal performance (cached instruction pre-fetch) */
    __builtin_mtc0(16, 0,(__builtin_mfc0(16, 0) | 0x3));

    //CHECONbits.PERCHEEN = 1;
    //CHECONbits.DCHEEN = 1;
    //CHECONbits.ICHEEN = 1;

    /* Configure Wait States and Prefetch */
    CHECONbits.PFMWS = 2;
    CHECONbits.PREFEN = 1;

    // purge cache on flash program
    CHECONbits.DCHECOH = 1;

    PRISS = 0x76543210;

    {
    // turn on ISAONEXC so we take micromips interrupts and exceptions!
	register unsigned long tmp;
	asm("mfc0 %0,$16,3" :  "=r"(tmp));
	tmp |= 1<<16;
	asm("mtc0 %0,$16,3" :: "r" (tmp));
    }
#else
    SYSTEMConfigPerformance(80000000L);
#endif
    INTEnableSystemMultiVectoredInt();
    (void)splx(7);
#if ! DEBUGGING
    DDPCON = 0;  // disable JTAG
#else
    debugger_attached = true;
#endif

    // slow us down
    // N.B. we can't rely on config bits since the bootloader sets them differently
    SYSKEY = 0xAA996655; // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA; // Write Key2 to SYSKEY
#if defined(__32MX250F128B__)
    OSCCONbits.PBDIV = 0;
#elif defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    /* Peripheral Bus 1 is by default enabled, set its divisor */
    PB1DIVbits.PBDIV = 0;
    /* Peripheral Bus 2 is by default enabled, set its divisor */
    PB2DIVbits.PBDIV = 0;
    /* Peripheral Bus 3 is by default enabled, set its divisor */
    PB3DIVbits.PBDIV = 0;
    /* Peripheral Bus 4 is by default enabled, set its divisor */
    PB4DIVbits.PBDIV = 0;
    //PB5DIV = 1;
    //PB6DIV = 3;
#else
    OSCCONbits.PBDIV = 1;
#endif
    SYSKEY = 0;

#if defined(__32MX250F128B__)
    cpu_frequency = 48000000;
    oscillator_frequency = 8000000;
    bus_frequency = 48000000;
#elif defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    cpu_frequency = 120000000;
    oscillator_frequency = 12000000;
    bus_frequency = 120000000;
#else
    cpu_frequency = 80000000;
    oscillator_frequency = 8000000;
    bus_frequency = 40000000;
#endif

    end_of_static = (byte *)FLASH_START + (_on_bootstrap-_ebase_address);

    // if pin_assignment_safemode is asserted on boot, skip autorun
    pin_initialize();

    // compute flash checksum
    for (p = (byte *)FLASH_START; p < end_of_static; p++) {
        flash_checksum += *p;
    }

    assert(sizeof(byte) == 1);
    assert(sizeof(uint16) == 2);
    assert(sizeof(uint32) == 4);

    // initialize timer
    timer_initialize();

    // calibrate our busywait
    x = splx(0);
    delay(20);
    splx(x);

    // initialize adc
    adc_initialize();

#if SCOPE
    // initialize scope
    scope_initialize();
#endif

    // configure leds
    led_initialize();

    // initialize flash
    flash_initialize();

    serial_initialize();

    // initialize usb
    usb_initialize();

    // initialize the application
    main_initialize();

    // enable interrupts
    splx(0);

    // initialize zigflea
    zb_initialize();

    // initialize the terminal interface
    terminal_initialize();

    // run the main application program loop
    main_run();

    ASSERT(0);  // stop!
    return 0;
}

