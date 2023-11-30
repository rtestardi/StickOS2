// *** adc.c **********************************************************
// this file polls the analog-to-digital converters coupled with the
// an0-an7 pins when used in analog input mode.

#include "main.h"

static bool adc_initialized;

enum {
    adc_num_debounce_history = 3
};

// updated every tick
static
volatile uint16 adc_result[adc_num_channel];  // 0..4095

// updated every "debouncing" tick, the rate is determined by timer_isr()
static
volatile uint16 adc_debounce[adc_num_channel][adc_num_debounce_history];

// indexes into adc_debounce: adc_debounce[ch][adc_debounce_set].
static
int adc_debounce_set;


// poll the analog-to-digital converters
void
adc_timer_poll(bool debouncing)
{
#if ! STICK_GUEST
    int i;
#endif

    if (! adc_initialized) {
        return;
    }

    assert(adc_debounce_set < adc_num_debounce_history);

#if ! STICK_GUEST
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    for (i = 0; i < adc_num_channel; i++) {
        adc_result[i] = ReadADC10(i);  // 0..4095
        if (debouncing) {
            adc_debounce[i][adc_debounce_set] = adc_result[i];
        }
    }

    // trigger adc scan now
    ADCCON3bits.GSWTRG = 1;

#else
#error
#endif
#endif // ! STICK_GUEST

    // if a debounced set if full, then advance the debouncing index to the next set.
    if (debouncing) {
        if (++adc_debounce_set >= adc_num_debounce_history) {
            adc_debounce_set = 0;
        }
    }
}

void
adc_sleep()
{
#if ! STICK_GUEST
#endif // ! STICK_GUEST
}

int
adc_get_value(int offset, int pin_qual)
{
    int value;

    assert(offset < adc_num_channel);

    if (pin_qual & 1<<pin_qual_debounced) {
        unsigned int min, max, sum, i;
        min = ~0;
        max = 0;
        sum = 0;

        // Need at least 3 samples to exclude min and max.
        assert(adc_num_debounce_history > 2);

        for (i = 0; i < adc_num_debounce_history; i++) {
            min = MIN(min, adc_debounce[offset][i]);
            max = MAX(max, adc_debounce[offset][i]);
            sum += adc_debounce[offset][i];
        }
        value = (sum-min-max)/(adc_num_debounce_history-2);
    } else {
        value = adc_result[offset];
    }

    value = (uint32)value*pin_analog/4096;

    return value;
}

// this function initializes the adc module.
void
adc_initialize(void)
{
#if ! STICK_GUEST
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // calibrate shared adc 7
    ADC7CFG = DEVADC7;

    ADCCON3bits.ADCSEL = 3;  // Select ADC input clock source = SYSCLK 120Mhz
    ADCCON3bits.CONCLKDIV = 0;  // Analog-to-Digital Control Clock (TQ) Divider = SYSCLK Divide by 1 (8.33ns)
    ADCCON1bits.FSSCLKEN = 1;  // Fast synchronous SYSCLK to ADC control clock is enabled
    ADCCON1bits.FSPBCLKEN = 1;  // ???

    ADCANCONbits.WKUPCLKCNT = 0x9;  // ADC Warm up delay = (512 * TADx)

    // configure shared adc
    ADCCON1bits.SELRES = 2;  // 10 bit conversion for shared
    ADCCON2bits.SAMC = 200;  // sample time = 202*TAD for shared  // XXX -- WHY DOES 2 BREAK TOASTER ANALOG INPUT???
    ADCCON2bits.ADCDIV = 1;  // TAD = 2*TQ for shared

    ADCANCONbits.ANEN7 = 1;  // Enable ADC7 analog bias/logic

    // shared adc scans AN12 thru AN19
    ADCTRG4bits.TRGSRC12 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC13 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC14 = 1;  // GSWTRG
    ADCTRG4bits.TRGSRC15 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC16 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC17 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC18 = 1;  // GSWTRG
    ADCTRG5bits.TRGSRC19 = 1;  // GSWTRG
    ADCCSS1bits.CSS12 = 1;  // a1
    ADCCSS1bits.CSS13 = 1;  // a2
    ADCCSS1bits.CSS14 = 1;  // a3
    ADCCSS1bits.CSS15 = 1;  // a4
    ADCCSS1bits.CSS16 = 1;  // a5
    ADCCSS1bits.CSS17 = 1;  // a6
    ADCCSS1bits.CSS18 = 1;  // a7
    ADCCSS1bits.CSS19 = 1;  // a8

    ADCCON1bits.ON = 1;  // turn on adc

    while(!ADCCON2bits.BGVRRDY);  // Wait until the reference voltage is ready
    while(ADCCON2bits.REFFLT);  // Wait if there is a fault with the reference voltage

    while(!ADCANCONbits.WKRDY7);  // Wait until ADC is ready

    ADCCON3bits.DIGEN7 = 1;      // Enable ADC

#else
#error
#endif
#endif // ! STICK_GUEST

    adc_initialized = true;
}

