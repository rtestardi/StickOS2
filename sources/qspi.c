// *** qspi.c *********************************************************
// this file performs qspi master i/o transfers.

#include "main.h"

#define QSPI_BAUD_FAST  800000  // zigflea
#define QSPI_BAUD_SLOW  200000  // default

// perform both output and input qspi i/o
void
qspi_transfer(bool cs, byte *buffer, int length)
{
    int x;

    qspi_initialize();

    x = splx(5);
    if (cs) {
        // cs active
        pin_set(pin_assignments[pin_assignment_qspi_cs], pin_type_digital_output, 0, 0);
    }

    while (length) {
        assert(! (SPI2STATbits.SPIBUSY));
        assert(! (SPI2STATbits.SPIRBF));
        assert(SPI2STATbits.SPITBE);
        SPI2BUF = *buffer;

        while (! SPI2STATbits.SPIRBF) {
            // NULL
        }

        assert(! (SPI2STATbits.SPIBUSY));
        assert(SPI2STATbits.SPIRBF);
        assert(SPI2STATbits.SPITBE);
        *buffer = SPI2BUF;

        buffer++;
        length--;
    }

    if (cs) {
        // cs inactive
        pin_set(pin_assignments[pin_assignment_qspi_cs], pin_type_digital_output, 0, 1);
    }
    splx(x);
}

extern void
qspi_baud_fast(void)
{
    // initialize qspi master at 800k baud
    SPI2CON = 0;

    assert(bus_frequency/QSPI_BAUD_FAST/2 - 1 < 512);
    SPI2BRG = bus_frequency/QSPI_BAUD_FAST/2 - 1;

    SPI2CON = _SPI2CON_ON_MASK|_SPI2CON_CKE_MASK|_SPI2CON_MSTEN_MASK;
}

static bool initialized;

extern void
qspi_uninitialize(void)
{
    initialized = false;

    SPI2CON = 0;
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    RPB4R = 4;  // SDO2 -> RB4
    SDI2R = 1;  // RB5 -> SDI2
    SS2R = 3;  // RB10 -> SS2
#endif
}

extern void
qspi_initialize(void)
{
    if (initialized) {
        return;
    }
    initialized = true;

    SPI2CON = 0;

    assert(bus_frequency/QSPI_BAUD_SLOW/2 - 1 < 512);
    SPI2BRG = bus_frequency/QSPI_BAUD_SLOW/2 - 1;

    SPI2CON = _SPI2CON_ON_MASK|_SPI2CON_CKE_MASK|_SPI2CON_MSTEN_MASK;
}

