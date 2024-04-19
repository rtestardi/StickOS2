#include "main.h"

#if SCOPE

#if ! defined(__32MK0512GPK064__) && ! defined(__32MK0512MCM064__)
#error  only __32MK__ interleaved adc dma is supported for now
#endif

// XXX -- is there any way to use dma counts to get accurate trigger position mid-capture???

// hardware configuration: ra0, ra1, rb0, rb1, ra11 interleaved analog input

// rc10 is wave out from CDAC2 or OC11

#define MSPS  18  // approximate target sample rate
#define SAMPLES  (2000*MSPS/20)  // approximate width of display canvas
#define EXTRA  200  // extra samples in case web page wants to fine-scale
#define INTERLEAVE  5  // we interleave results from 5 adcs

// command line:
// <sampleinterval> [+-=](<voltage>|0x<bits>) [<slew>|0x<mask>] [<delaysamples>]
//          -10    -> 18  Msps -> 3.6  MHz timer with 5x interleave and 10x magnification
//          -5     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 5x magnification
//          -2     -> 18  Msps -> 3.6  MHz timer with 5x interleave and 2x magnification
// number1 = 1     -> 18  Msps -> 3.6  MHz timer with 5x interleave
//           2     -> 9   Msps -> 1.8  MHz timer with 5x interleave
//           5     -> 3.6 Msps -> 0.72 MHz timer with 5x interleave
//           10    -> 1.8 Msps -> 0.36 MHz timer with 5x interleave
//           20    -> 0.9 Msps -> 0.18 MHz timer with 5x interleave
//           50    -> 360 ksps -> 72   kHz timer with 5x interleave
//           ...
//           10000 -> 1.8 ksps -> 0.36 kHz timer with 5x interleave
//           20000 -> 0.9 ksps -> 0.18 kHz timer with 5x interleave

short dmabuffer[INTERLEAVE][2][128];  // filled by adc dma hardware

short adcbuffer[INTERLEAVE*2*128*10];  // copied to from dmabuffer by adc dma isr

short bitsbuffer[INTERLEAVE][2*128*10];  // filled from PORTB by core dma hardware

// adcbuffer is an integral dmabuffer size so we don't need to checks for wraps mid-copy
CASSERT(LENGTHOF(adcbuffer)%LENGTHOF(dmabuffer[0][0]) == 0);

CASSERT(sizeof(bitsbuffer) == sizeof(adcbuffer));

// N.B. both of these can be larger than adcbuffer, as they wrap adcbuffer!
volatile int current;  // current location where dma isr is filling adcbuffer
volatile int trigger;  // location of trigger start in adc buffer, including delay

volatile int currento;  // current % LENGTHOF(adcbuffer) maintained for performance

int magnify;  // non-0 -> we allow each adc sample to be reported multiple times

volatile int g;

// accumulate dmabuffer into adcbuffer
void
__ISR(_ADC_DMA_VECTOR, IPL7SRS)
scope_isr()
{
    short *o;
    short *lasto;
    int adcdstat;
    short *i0, *i1, *i2, *i3, *i4;

    // read ADCDSTAT once
    adcdstat = ADCDSTAT;

    // dmabuffer A and B can't both be full!
    assert((adcdstat & (_ADCDSTAT_RAF4_MASK|_ADCDSTAT_RBF4_MASK)) != (_ADCDSTAT_RAF4_MASK|_ADCDSTAT_RBF4_MASK));

    // if either dmabuffer is full
    if (adcdstat & (_ADCDSTAT_RAF4_MASK|_ADCDSTAT_RBF4_MASK)) {
        // if dmabuffer A is full...
        if (adcdstat & _ADCDSTAT_RAF4_MASK) {
            // get dmabuffer A input addresses
            i0 = dmabuffer[0][0];
            i1 = dmabuffer[1][0];
            i2 = dmabuffer[2][0];
            i3 = dmabuffer[3][0];
            i4 = dmabuffer[4][0];
        } else {
            // get dmabuffer B input addresses
            i0 = dmabuffer[0][1];
            i1 = dmabuffer[1][1];
            i2 = dmabuffer[2][1];
            i3 = dmabuffer[3][1];
            i4 = dmabuffer[4][1];
        }

        // get adcbuffer output address
        // N.B. adding and maintaining currento is a bit faster than adding (current % LENGTHOF(adcbuffer));
        o = adcbuffer + currento;
        lasto = o + sizeof(dmabuffer)/2/sizeof(*o);

        // copy to dmabuffer to adcbuffer
        while (o < lasto) {
            *o++ = *i0++;
            *o++ = *i1++;
            *o++ = *i2++;
            *o++ = *i3++;
            *o++ = *i4++;
        }

        current += INTERLEAVE*LENGTHOF(dmabuffer[0][0]);
        currento += INTERLEAVE*LENGTHOF(dmabuffer[0][0]);
        if (currento == LENGTHOF(adcbuffer)) {
            currento = 0;
        }
    }

    IFS3bits.AD1FCBTIF = 0; // Clear the ADC DMA Interrupt
}

void
scope_initialize()
{
    // configure analog capture
    // N.B. this is called after adc_initialize()

    // enable OPA3
    PMD2bits.OPA3MD = 0;
    CM3CONbits.ENPGA = 1;  // unity gain
    CM3CONbits.OPAON = 1;

    ADCCON1bits.ON = 0;  // turn off adc
    g++;

    // calibrate dedicated adcs 0-4
    ADC0CFG = DEVADC0;
    ADC1CFG = DEVADC1;
    ADC2CFG = DEVADC2;
    ADC3CFG = DEVADC3;
    ADC4CFG = DEVADC4;

    // configure adc DMA
    ADCCON1bits.DMABL = 7;  // ADC Buffer length = 128 samples
    ADCDMAB = KVA_TO_PA(dmabuffer);

    // configure dedicated adcs
    ADC0TIMEbits.SELRES = 3;  // 12 bit conversion
    ADC0TIMEbits.BCHEN = 1;  // DMA enable
    ADC0TIMEbits.ADCDIV = 1;  // TAD = 2*TQ (16.67ns)
    ADC0TIMEbits.SAMC = 1;  // sample time = 3*TAD (since we have enough time at 3.6 Msps)
    ADC1TIME = ADC0TIME;
    ADC2TIME = ADC0TIME;
    ADC3TIME = ADC0TIME;
    ADC4TIME = ADC0TIME;

    ADCANCONbits.ANEN0 = 1;  // Enable ADC0 analog bias/logic
    ADCANCONbits.ANEN1 = 1;  // Enable ADC0 analog bias/logic
    ADCANCONbits.ANEN2 = 1;  // Enable ADC0 analog bias/logic
    ADCANCONbits.ANEN3 = 1;  // Enable ADC0 analog bias/logic
    ADCANCONbits.ANEN4 = 1;  // Enable ADC0 analog bias/logic

    ADCTRG1bits.TRGSRC0 = 0x10;  // OC1 triggers adc0 at 20% of T3
    ADCTRG1bits.TRGSRC1 = 0x11;  // OC2 triggers adc1 at 40% of T3
    ADCTRG1bits.TRGSRC2 = 0x12;  // OC3 triggers adc2 at 60% of T3
    ADCTRG1bits.TRGSRC3 = 0x13;  // OC4 triggers adc3 at 80% of T3
    ADCTRG2bits.TRGSRC4 = 0x06;  // TMR3 triggers adc4 at 100% of T3

    ADCCON1bits.ON = 1;  // turn on adc

    while(!ADCCON2bits.BGVRRDY);  // Wait until the reference voltage is ready
    while(ADCCON2bits.REFFLT);  // Wait if there is a fault with the reference voltage

    while(!ADCANCONbits.WKRDY0);  // Wait until ADC is ready
    while(!ADCANCONbits.WKRDY1);  // Wait until ADC is ready
    while(!ADCANCONbits.WKRDY2);  // Wait until ADC is ready
    while(!ADCANCONbits.WKRDY3);  // Wait until ADC is ready
    while(!ADCANCONbits.WKRDY4);  // Wait until ADC is ready

    // N.B. ports ABC setup in scope.c; ports EG setup in pin.c
    ADCTRGMODEbits.SH4ALT = 2;  // adc4 reads from AN9/RA11

    TRISA = 0x0803;             // adc0 reads from AN0/RA0
    ANSELA = 0x0803;            // adc1 reads from AN1/RA1
    CNPUA = ~ANSELA;            // keep inputs at low power

    TRISB = 0xffff;             // adc2 reads from AN2/RB0
    ANSELB = 0x0003;            // adc3 reads from AN3/RB1
    //CNPUB = ~ANSELB;          // do not enable pullups for logic analyzer inputs!

    TRISC = 0xffff;
    ANSELC = 0x0005;            // oa3in+ is RC2; oa3out is RC0
    CNPUC = ~ANSELC;            // keep inputs at low power

    ADCDSTATbits.RAFIEN4 = 1;  // enable buffer full int for final (adc4) interleave A buffer
    ADCDSTATbits.RBFIEN4 = 1;  // enable buffer full int for final (adc4) interleave B buffer
    IFS3bits.AD1FCBTIF = 0;  // Clear DMA Buffer full interrupt flag
    IPC26bits.AD1FCBTIP = 7;  // Set DMA Buffer full interrupt priority
    IPC26bits.AD1FCBTIS = 0;  // Set DMA Buffer full interrupt sub-priority
    IEC3bits.AD1FCBTIE = 1;  // Enable DMA Buffer full interrupt

    // configure digital capture

    DMACONbits.ON = 1;

    DCH0SSIZ = 2;  // source buffer size
    DCH0SSA = (uint32)KVA_TO_PA(&PORTB);  // source address
    DCH0DSIZ = sizeof(bitsbuffer[0]);  // dest buffer size
    DCH0DSA = (uint32)KVA_TO_PA(bitsbuffer[0]);  // dest address
    DCH0CSIZ = 2;  // 2 byte cell transfer on event
    DCH0ECONbits.CHSIRQ = 7;  // OC1 is event
    DCH0ECONbits.SIRQEN = 1;
    DCH0CONbits.CHPRI = 3;  // higher prio than wave
    DCH0CONbits.CHAEN = 1;  // run block after block!

    DCH1SSIZ = 2;  // source buffer size
    DCH1SSA = (uint32)KVA_TO_PA(&PORTB);  // source address
    DCH1DSIZ = sizeof(bitsbuffer[1]);  // dest buffer size
    DCH1DSA = (uint32)KVA_TO_PA(bitsbuffer[1]);  // dest address
    DCH1CSIZ = 2;  // 2 byte cell transfer on event
    DCH1ECONbits.CHSIRQ = 12;  // OC2 is event
    DCH1ECONbits.SIRQEN = 1;
    DCH1CONbits.CHPRI = 3;  // higher prio than wave
    DCH1CONbits.CHAEN = 1;  // run block after block!

    DCH2SSIZ = 2;  // source buffer size
    DCH2SSA = (uint32)KVA_TO_PA(&PORTB);  // source address
    DCH2DSIZ = sizeof(bitsbuffer[2]);  // dest buffer size
    DCH2DSA = (uint32)KVA_TO_PA(bitsbuffer[2]);  // dest address
    DCH2CSIZ = 2;  // 2 byte cell transfer on event
    DCH2ECONbits.CHSIRQ = 17;  // OC3 is event
    DCH2ECONbits.SIRQEN = 1;
    DCH2CONbits.CHPRI = 3;  // higher prio than wave
    DCH2CONbits.CHAEN = 1;  // run block after block!

    DCH3SSIZ = 2;  // source buffer size
    DCH3SSA = (uint32)KVA_TO_PA(&PORTB);  // source address
    DCH3DSIZ = sizeof(bitsbuffer[3]);  // dest buffer size
    DCH3DSA = (uint32)KVA_TO_PA(bitsbuffer[3]);  // dest address
    DCH3CSIZ = 2;  // 2 byte cell transfer on event
    DCH3ECONbits.CHSIRQ = 22;  // OC4 is event
    DCH3ECONbits.SIRQEN = 1;
    DCH3CONbits.CHPRI = 3;  // higher prio than wave
    DCH3CONbits.CHAEN = 1;  // run block after block!

    DCH4SSIZ = 2;  // source buffer size
    DCH4SSA = (uint32)KVA_TO_PA(&PORTB);  // source address
    DCH4DSIZ = sizeof(bitsbuffer[4]);  // dest buffer size
    DCH4DSA = (uint32)KVA_TO_PA(bitsbuffer[4]);  // dest address
    DCH4CSIZ = 2;  // 2 byte cell transfer on event
    DCH4ECONbits.CHSIRQ = 14;  // TMR3 is event
    DCH4ECONbits.SIRQEN = 1;
    DCH4CONbits.CHPRI = 3;  // higher prio than wave
    DCH4CONbits.CHAEN = 1;  // run block after block!
}

void
scope_begin(int number1, int number2)
{
    int t;
    int ps;
    static int once;

    if (! once) {
        once = 1;
        /* unlock system for clock configuration */
        SYSKEY = 0x00000000;
        SYSKEY = 0xAA996655;
        SYSKEY = 0x556699AA;

        CFGCONbits.PMDLOCK = 0;

        PMD1 = 0x00000310;  // EEMD CTMUMD DAC1
        PMD2 = 0x0013001a;  // OPA5MD OPA2MD OPA1MD, CMP5MD C4MPMD CMP2MD
        PMD3 = 0xfbc0ffff;  // all OC/IC except OC11MD OC6MD OC5MD OC4MD OC3MD OC2MD OC1MD
        PMD4 = 0x0fff01f8;  // all PWM/T except T3 T2 T1
        PMD5 = 0xf20f3f3f;  // all CAN/USB/I2C/SPI/U except USB1
        PMD6 = 0x0f0d0300;  // all QE/PMP/REFO except REFO3 REFO4

        CFGCONbits.PMDLOCK = 1;

        /* Lock system since done with clock configuration */
        SYSKEY = 0x33333333;
    }
    
    pin_declare(PIN_A0, pin_type_digital_output, 0);
    pin_declare(PIN_A1, pin_type_digital_output, 0);
    pin_declare(PIN_A2, pin_type_digital_output, 0);
    pin_declare(PIN_A3, pin_type_digital_output, 0);
    pin_declare(PIN_A4, pin_type_digital_output, 0);
    pin_declare(PIN_A5, pin_type_digital_output, 0);
    pin_declare(PIN_A6, pin_type_digital_output, 0);
    pin_declare(PIN_A7, pin_type_digital_output, 0);
    pin_declare(PIN_A8, pin_type_digital_output, 0);

    pin_declare(PIN_E2, pin_type_digital_output, 0);
    pin_declare(PIN_E3, pin_type_digital_output, 0);

    pin_set(PIN_E2, pin_type_digital_output, 0, 0);  // we are waiting for trigger
    pin_set(PIN_E3, pin_type_digital_output, 0, 1);  // we are waiting for trigger

    RPE14R = 0;  // undo OC5 -> RE14
    RPE15R = 0;  // undo OC6 -> RE15
    RPG9R = 0;  // undo OC3 -> RG9
    RPG8R = 0;  // undo OC4 -> RG8
    RPG7R = 0;  // undo OC2 -> RG7
    RPG6R = 0;  // undo OC1 -> RG6

    // configure analog comparator voltage reference

    // set dac3 to number2
    DAC3CONbits.REFSEL = 3;  // Positive reference voltage = AVDD
    DAC3CONbits.DACDAT = number2 << 2;  // 10 bit input to 12 bit register

    //DAC3CONbits.DACOE = 1;  // enable DAC3OUT on CDAC3/RA8
    DAC3CONbits.ON = 1;

    // configure adc trigger timer

    // set TMR3 to 3.6 MHz divided by horizontal scale
    if (number1 > 1000) {
        ps = 16;
    } else {
        ps = 1;
    }
    t = (bus_frequency/1000000*number1*INTERLEAVE/ps + MSPS/2)/MSPS;  // 1:ps prescale
    assert(t > 1 && t < 0x10000);  // 16 bit register

    // use 100% of T3 trigger for adc4
    T3CONCLR = _T3CON_ON_MASK;
    g++;
    TMR3 = 0;
    PR3 = t-1;
    assert(ps == 1 || ps == 16);
    T3CON = (ps==16?4:0) <<_T3CON_TCKPS_POSITION;  // 1:ps prescale

    // configure adc trigger output compares

    // use 20% of T3 trigger for adc0
    OC1CONbits.ON = 0;
    g++;
    OC1CONbits.OCTSEL = 1;  // timer 3
    OC1CONbits.OCM = 5;  // initial low, continuous high pulses
    OC1R = t/INTERLEAVE-1;
    OC1RS = t/INTERLEAVE+1;
    OC1CONbits.ON = 1;

    // use 40% of T3 trigger for adc1
    OC2CONbits.ON = 0;
    g++;
    OC2CONbits.OCTSEL = 1;  // timer 3
    OC2CONbits.OCM = 5;  // initial low, continuous high pulses
    OC2R = 2*t/INTERLEAVE-1;
    OC2RS = 2*t/INTERLEAVE+1;
    OC2CONbits.ON = 1;

    // use 60% of T3 trigger for adc2
    OC3CONbits.ON = 0;
    g++;
    OC3CONbits.OCTSEL = 1;  // timer 3
    OC3CONbits.OCM = 5;  // initial low, continuous high pulses
    OC3R = 3*t/INTERLEAVE-1;
    OC3RS = 3*t/INTERLEAVE+1;
    OC3CONbits.ON = 1;

    // use 80% of T3 trigger for adc3
    OC4CONbits.ON = 0;
    g++;
    OC4CONbits.OCTSEL = 1;  // timer 3
    OC4CONbits.OCM = 5;  // initial low, continuous high pulses
    OC4R = 4*t/INTERLEAVE-1;
    OC4RS = 4*t/INTERLEAVE+1;
    OC4CONbits.ON = 1;
}

// prepare for DMA to capture samples imminently -- THIS IS EVERYTHING EXCEPT "T3CONSET = _T3CON_ON_MASK;"
void
scope_capture_begin(int number1, bool rise, bool fall)
{
    current = 0;
    currento = 0;
    trigger = -1;

    TMR3 = 0;

    ADCDSTATbits.DMAEN = 1;
    ADCCON3bits.DIGEN0 = 1;      // Enable ADC
    ADCCON3bits.DIGEN1 = 1;      // Enable ADC
    ADCCON3bits.DIGEN2 = 1;      // Enable ADC
    ADCCON3bits.DIGEN3 = 1;      // Enable ADC
    ADCCON3bits.DIGEN4 = 1;      // Enable ADC

    DCH0CONbits.CHEN = 1;
    DCH1CONbits.CHEN = 1;
    DCH2CONbits.CHEN = 1;
    DCH3CONbits.CHEN = 1;
    DCH4CONbits.CHEN = 1;

    CM1CONbits.HYSPOL = fall?0:1;  // 0 -> rising edge; 1 -> falling edge
    CM1CONbits.CFSEL = 0;  // SYSCLK
    CM1CONbits.CFDIV = number1==1?1:4;  // digital filter divide by 16 except at highest sample rate, when we divide by 2
    CM1CONbits.CFLTREN = 1;  // digital filter divides by 3 again
    CM1CONbits.HYSSEL = 2;  // medium hysteresis
    CM1CONbits.CPOL = 1;  // 1 -> inverted (because CMP+ is DAC3)
    CM1CONbits.EVPOL = fall?2:1;  // 1 -> rising edge; 2 -> falling edge
    CM1CONbits.CREF = 1;  // CMP+ is DAC3
    CM1CONbits.CCH = 3;  // CMP- is RPB1
    //RPC10R = 7;  // C1OUT on RC10
    //CM1CONbits.COE = 1;  // enable C1OUT on RC10

    // enable the comparator
    delay(2);
    CM1CONbits.ON = 1;
    delay(2);
}

// virt:   b8,   b7,   b6,   b5,   b4,   b3,   b2,   b1,   b0
// phys: rb15, rb14, rb13, rb12, rb11, rb10,  rb6,  rb5,  rb4

int
ptovbits(int bits)
{
    return ((bits&0xfc00)>>7) | ((bits & 0x0070)>>4);
}

int
vtopbits(int bits)
{
    return ((bits&0x01f8)<<7) | ((bits&0x0007)<<4);
}

void
scope_trigger(bool level, bool automatic, bool rise, bool fall, int digital, int number2, int number3, int number4)
{
    int i;
    int r;
    int loop;
    int loops;

    loops = 1000000;  // XXX -- tune this!

    // temporarily turn off the timer interrupt
    IEC0bits.T1IE = 0;

    // delay a sub-ms random time to make a better demo with 1kHz test signal on auto/level trigger!
    r = random_32()%blips_per_ms;
    for (i = 0; i < r; i++) {
        blip();
    }

    // start looking for trigger now
    CM1CONbits.COUT = 0;
    CM1CONbits.CEVT = 0;

    loop = 0;

    // wait for trigger
    if (digital) {
        int number2x;
        int number3x;
        bool triggered;
        bool lasttriggered = -1;

        number2x = vtopbits(number2);
        number3x = vtopbits(number3);

        if (automatic) {
            for (;;) {
                if ((PORTB & number3x) == number2x || loop++ > loops) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        } else if (level) {
            for (;;) {
                if ((PORTB & number3x) == number2x) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        } else {
            assert(rise || fall);
            assert(rise != fall);

            for (;;) {
                triggered = (PORTB & number3x) == number2x;
                if (triggered == rise && lasttriggered == fall) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                lasttriggered = triggered;
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        }
    } else {
        if (automatic) {
            for (;;) {
                if (CM1CONbits.COUT || loop++ > loops) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        } else if (level) {
            for (;;) {
                if (CM1CONbits.COUT) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        } else {
            for (;;) {
                if (CM1CONbits.CEVT) {
                    // N.B. THIS STARTS EVERYTHING GOING!!!  ADCs, ADC DMA, DIGITAL DMA, ETC.!
                    T3CONSET = _T3CON_ON_MASK;
                    break;
                }
                if (! run_stop) {
                    goto XXX_RETURN_XXX;
                }
            }
        }
    }

    if (run_stop) {
        trigger = number4;  // mark trigger, including delay

        pin_set(PIN_E2, pin_type_digital_output, 0, 1);  // we have triggered
    }

XXX_RETURN_XXX:
    pin_set(PIN_E3, pin_type_digital_output, 0, 0);  // we are no longer waiting for trigger
}

void
scope_capture_end(void)
{
    int i;
    int once;

    once = 0;

    if (pin_get(PIN_S1, pin_type_digital_input, 0) == 1) {
        once = 1;
    }

    // wait for trigger and post samples; kill time and go easy on the bus
    while (run_stop && current < trigger+SAMPLES+EXTRA) {
        // XXX -- this causes possible power supply noise on 2.6 hacked to use unregulated +/-5V for op amp
        //__asm__("wait");
        if (! once) {
            // generate test pattern for loopback testing from a* to b*
            for (i = 0; i < 9; i++) {
                pin_set(PIN_A0+i, pin_type_digital_output, 0, 1);
                pin_set(PIN_A0+i, pin_type_digital_output, 0, 0);
            }
            once = 1;

            // resume timer interrupts
            // XXX -- should this be after sample complete???
            IEC0bits.T1IE = 1;
        }
    }

    // resume timer interrupts
    // XXX -- should this be after sample complete???
    IEC0bits.T1IE = 1;

    // N.B. THIS STOPS EVERYTHING!
    T3CONCLR = _T3CON_ON_MASK;
    g++;

    ADCCON3bits.DIGEN0 = 0;      // Disable ADC
    ADCCON3bits.DIGEN1 = 0;      // Disable ADC
    ADCCON3bits.DIGEN2 = 0;      // Disable ADC
    ADCCON3bits.DIGEN3 = 0;      // Disable ADC
    ADCCON3bits.DIGEN4 = 0;      // Disable ADC
    ADCDSTATbits.DMAEN = 0;

    CM1CONbits.ON = 0;
    g++;

    DAC3CONbits.ON = 0;
    g++;

    DCH0ECONbits.CABORT = 1;
    DCH1ECONbits.CABORT = 1;
    DCH2ECONbits.CABORT = 1;
    DCH3ECONbits.CABORT = 1;
    DCH4ECONbits.CABORT = 1;
}

void
scope_result(int number1)
{
    int i;
    int bits;
    int l, m;

    l = 0;
    for (i = trigger; i < trigger+SAMPLES+EXTRA; i++) {
        for (m = 0; m < magnify; m++) {
            bits = bitsbuffer[i%INTERLEAVE][(i/INTERLEAVE)%LENGTHOF(bitsbuffer[0])];
            printf("%d,0x%x\n", adcbuffer[i%LENGTHOF(adcbuffer)], ptovbits(bits));
            if (++l == SAMPLES+EXTRA) {
                return;
            }
        }
    }
}

void
scope_end()
{
    // N.B. turn OCs off before reassigning to pins!
    OC1CONbits.ON = 0;
    g++;
    OC2CONbits.ON = 0;
    g++;
    OC3CONbits.ON = 0;
    g++;
    OC4CONbits.ON = 0;
    g++;

    RPE14R = 6;  // OC5 -> RE14
    RPE15R = 6;  // OC6 -> RE15
    RPG9R = 5;  // OC3 -> RG9
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    RPG6R = 5;  // OC1 -> RG6

    pin_set(PIN_E2, pin_type_digital_output, 0, 0);  // we have no longer triggered
    pin_set(PIN_E3, pin_type_digital_output, 0, 0);  // we are no longer waiting for trigger
}

void scope(int number1, bool rise, bool fall, bool level, bool automatic, int digital, int number2, int number3, int number4)
{
    if (! number1) {
        return;
    }

    //memset(adcbuffer, 0, sizeof(adcbuffer));
    //memset(bitsbuffer, 0, sizeof(bitsbuffer));

    // handle negative number1 magnification
    magnify = 1;
    if (number1 < 0) {
        magnify = -number1;
        number1 = 1;
    }

    //run_clear(false);
    scope_begin(number1, number2);

    run_stop = 1;
    main_command = NULL;
    terminal_command_ack(false);
    terminal_command_discard(true);

    scope_capture_begin(number1, rise, fall);

    scope_trigger(level, automatic, rise, fall, digital, number2, number3, number4);

    scope_capture_end();

    if (run_stop) {
        scope_result(number1);
    }

    run_stop = false;
    terminal_command_discard(false);

    scope_end();
    //run_clear(false);
}

#if 0
// scraped from flea-scope measuring fg-100!
$q=@(80, 97, 170, 238, 295, 310, 322, 325, 332, 334, 332, 336, 339, 347, 350, 361, 372, 378, 402, 422, 445,
    463, 491, 514, 530, 535, 534, 524, 506, 472, 444, 408, 388, 362, 346, 339, 326, 318, 323, 321, 325,
    325, 329, 338, 343, 347, 347, 342, 340, 339, 335, 331, 331, 332, 328, 325, 325, 328, 320, 316, 314,
    313, 311, 311, 311, 307, 314, 310, 311, 310, 307, 305, 307, 311, 313, 324, 336, 337, 347, 338, 339,
    326, 317, 321, 313, 296, 279, 272, 270, 274, 273, 269, 255, 262, 354, 675, 938, 926, 635, 208)
$n=0; $a="";
foreach ($i in $q) {
    $a = $a + ($i*4) + ", "
    $n++;
    if (($n%20) -eq 0) { $a; $a="" };
}; $a
#endif

short ekg[] = {
    320, 388, 680, 952, 1180, 1240, 1288, 1300, 1328, 1336, 1328, 1344, 1356, 1388, 1400, 1444, 1488, 1512, 1608, 1688,
    1780, 1852, 1964, 2056, 2120, 2140, 2136, 2096, 2024, 1888, 1776, 1632, 1552, 1448, 1384, 1356, 1304, 1272, 1292, 1284,
    1300, 1300, 1316, 1352, 1372, 1388, 1388, 1368, 1360, 1356, 1340, 1324, 1324, 1328, 1312, 1300, 1300, 1312, 1280, 1264,
    1256, 1252, 1244, 1244, 1244, 1228, 1256, 1240, 1244, 1240, 1228, 1220, 1228, 1244, 1252, 1296, 1344, 1348, 1388, 1352,
    1356, 1304, 1268, 1284, 1252, 1184, 1116, 1088, 1080, 1096, 1092, 1076, 1020, 1048, 1416, 2700, 3752, 3704, 2540, 832,
};

#if 0
$a=""; $p=100; $pp=4095*.98; for ($i = 0; $i -lt $p; $i++) {
    $rad = 2*[Math]::pi/$p*$i;
    $s = [Math]::Sin($rad);
    $d=[Math]::Floor($s*($pp/2) + $pp/2 + (4095-$pp)/2);
    $a = $a + $d + ", "
    if ((($i+1)%20) -eq 0) { $a; $a="" };
}; $a
#endif

short sine[] = {
    2047, 2173, 2298, 2423, 2546, 2667, 2786, 2901, 3014, 3122, 3226, 3326, 3421, 3510, 3593, 3670, 3741, 3805, 3863, 3913,
    3955, 3991, 4018, 4038, 4050, 4054, 4050, 4038, 4018, 3991, 3955, 3913, 3863, 3805, 3741, 3670, 3593, 3510, 3421, 3326,
    3226, 3122, 3014, 2901, 2786, 2667, 2546, 2423, 2298, 2173, 2047, 1921, 1796, 1671, 1548, 1427, 1308, 1193, 1080, 972,
    868, 768, 673, 584, 501, 424, 353, 289, 231, 181, 139, 103, 76, 56, 44, 40, 44, 56, 76, 103,
    139, 181, 231, 289, 353, 424, 501, 584, 673, 768, 868, 972, 1080, 1193, 1308, 1427, 1548, 1671, 1796, 1921,
};

#if 0
$a=""; $p=100; $pp=4095*.98; for ($i = 0; $i -lt $p; $i++) {
    if ($i -lt $p/2) {
        $d=[Math]::Floor($i/($p/2-1)*$pp + (4095-$pp)/2);
    } else {
        $d=[Math]::Floor((($p-1)-$i)/($p/2-1)*$pp + (4095-$pp)/2);
    };
    $a = $a + $d + ", ";
    if ((($i+1)%20) -eq 0) { $a; $a="" };
}; $a
#endif

short triangle[] = {
    40, 122, 204, 286, 368, 450, 532, 614, 696, 778, 859, 941, 1023, 1105, 1187, 1269, 1351, 1433, 1515, 1597,
    1678, 1760, 1842, 1924, 2006, 2088, 2170, 2252, 2334, 2416, 2497, 2579, 2661, 2743, 2825, 2907, 2989, 3071, 3153, 3235,
    3316, 3398, 3480, 3562, 3644, 3726, 3808, 3890, 3972, 4054, 4054, 3972, 3890, 3808, 3726, 3644, 3562, 3480, 3398, 3316,
    3235, 3153, 3071, 2989, 2907, 2825, 2743, 2661, 2579, 2497, 2416, 2334, 2252, 2170, 2088, 2006, 1924, 1842, 1760, 1678,
    1597, 1515, 1433, 1351, 1269, 1187, 1105, 1023, 941, 859, 778, 696, 614, 532, 450, 368, 286, 204, 122, 40,
};

#define WAVES  100

void
wave(char *pattern, int hz)
{
    int t;
    int ps;
    int min;
    int max;
    bool square;
    short *dacdat;

    delay(500);  // this always takes a while!

    square = parse_word(&pattern, "square");

    T2CONCLR = _T2CON_ON_MASK;
    g++;
    if (square) {
        // square wave
        // prepare OC11
        DAC2CONbits.ON = 0;  // no CDAC2 on RC10
        g++;
        RPC10R = 9;  // OC11 on RC10
        min = 10;
        max = 4000000;  // 4 MHz

    } else {
        // arbitrary waveforms -- ekg, sine, triangle wave
        // prepare dac2
        RPC10R = 0;  // no OC11 on RC10
        DAC2CONbits.ON = 1;  // CDAC2 on RC10
        hz *= WAVES;  // WAVES samples per waveform
        min = 2*WAVES;
        max = 40000*WAVES;  // 40 kHz
    }

    if (! hz || hz < min || hz > max) {
        return;
    }

    if (hz > 2000) {
        ps = 1;
    } else if (hz > 200) {
        ps = 16;
    } else {
        ps = 256;
    }
    t = bus_frequency/hz/ps;
    assert(t > 1 && t < 0x10000);  // 16 bit register

    TMR2 = 0;
    PR2 = t-1;
    assert(ps == 1 || ps == 16 || ps == 256);
    T2CON = (ps==256?7:(ps==16?4:0)) <<_T2CON_TCKPS_POSITION;  // 1:ps prescale

    if (square) {
        // square wave
        OC11CONbits.ON = 0;
        g++;
        OC11CONbits.OCTSEL = 0;  // timer 2
        OC11CONbits.OCM = 5;  // initial low, continuous high pulses
        OC11R = t/2+1;
        OC11RS = 1;
        OC11CONbits.ON = 1;

        // turn dma off
        DCH5CONbits.CHEN = 0;

    } else {
        // ekg, sine, triangle wave
        DAC2CONbits.ON = 0;
        g++;
        DAC2CONbits.DACDAT = 0;  // input to 12 bit register
        DAC2CONbits.REFSEL = 3;  // Positive reference voltage = AVDD
        DAC2CONbits.DACOE = 1;  // enable DAC2OUT on CDAC2/RC10
        DAC2CONbits.ON = 1;  // CDAC2 on RC10

        dacdat = (short *)&DAC2CON;
        dacdat++;  // dacdat is high word of dac2con;

        if (parse_word(&pattern, "ekg")) {
            DCH5SSIZ = sizeof(ekg);  // source buffer size
            DCH5SSA = (uint32)KVA_TO_PA(ekg);  // source address
        } else if (parse_word(&pattern, "sine")) {
            DCH5SSIZ = sizeof(sine);  // source buffer size
            DCH5SSA = (uint32)KVA_TO_PA(sine);  // source address
        } else if (parse_word(&pattern, "triangle")) {
            DCH5SSIZ = sizeof(triangle);  // source buffer size
            DCH5SSA = (uint32)KVA_TO_PA(triangle);  // source address
        } else {
            return;
        }

        // turn dma on
        DCH5DSIZ = 2;  // dest buffer size
        DCH5DSA = (uint32)KVA_TO_PA(dacdat);  // dest address
        DCH5CSIZ = 2;  // 2 byte cell transfer on event
        DCH5ECONbits.CHSIRQ = 9;  // TMR2 is event
        DCH5ECONbits.SIRQEN = 1;
        DCH5CONbits.CHAEN = 1;  // run block after block!
        DCH5CONbits.CHEN = 1;
    }

    T2CONSET = _T2CON_ON_MASK;
}

#endif
