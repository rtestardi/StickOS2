// *** i2c.c **********************************************************
// this file performs i2c master i/o transfers.

#include "main.h"

#define I2C_BAUD  100000  // 100 kHz

static byte address;
static bool started;
static bool initialized;
static int breaks;
int i2c_reset_reason;

// something went wrong; abort all remaining i2c transfers
static
void
i2c_reset(int reason)
{
    i2c_reset_reason = reason;
    i2c_uninitialize();
    printf("i2c reset 0x%x\n", reason);
    stop();
    started = false;
}

// wait for any i2c transfers to complete
static
bool  // true -> indicates things are fine; keep going
i2c_wait(bool write)
{
    int con;
    int stat;

    for (;;) {
        con = I2C1CON;
        stat = I2C1STAT;

        // if we have an error...
        if (stat & (_I2C1STAT_BCL_MASK|_I2C1STAT_IWCOL_MASK|_I2C1STAT_I2COV_MASK)) {
            i2c_reset(stat);
            return false;
        }

        // if the user requested us to stop...
        if (run_breaks != breaks) {
            i2c_reset(0);
            return false;
        }

        // if the transfer operation is still in progress...
        if (con & (_I2C1CON_SEN_MASK|_I2C1CON_RSEN_MASK|_I2C1CON_PEN_MASK|_I2C1CON_RCEN_MASK|_I2C1CON_ACKEN_MASK) || stat & (_I2C1STAT_TRSTAT_MASK)) {
            continue;
        }

        // if we are still waiting for data...
        if ((write && I2C1STATbits.TBF) || (!write && !I2C1STATbits.RBF)) {
            continue;
        }

        // we are done waiting!
        return true;
    }
}

// external api; record the i2c device address
void
i2c_start(int address_in)
{
    address = (byte)address_in;
    started = false;
}

// internal api; actually send the start when we have a read/write to perform
static
bool  // true -> indicates things are fine
i2c_start_real(bool write)
{
    assert(! started);

    if (! i2c_wait(true)) {
        return false;
    }

    // generate start
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN);

    if (! i2c_wait(true)) {
        return false;
    }

    // send address and read/write flag
    I2C1TRN = (address<<1)|(! write);

    if (! i2c_wait(true)) {
        return false;
    }

    if (! write) {
        // enable receive
        I2C1CONbits.RCEN = 1;
    }

    started = true;
    return true;
}

// internal api; actually send the restart when we have a read/write to perform
static
bool  // true -> indicates things are fine
i2c_repeat_start_real(bool write)
{
    assert(started);

    if (! i2c_wait(true)) {
        return false;
    }

    // generate repeat start
    I2C1CONbits.RSEN = 1;

    if (! i2c_wait(true)) {
        return false;
    }

    // send address and read/write flag
    I2C1TRN = (address<<1)|(! write);

    if (! i2c_wait(true)) {
        return false;
    }

    if (! write) {
        // enable receive
        I2C1CONbits.RCEN = 1;
    }

    return true;
}

// external api; perform one or more i2c read/writes within the context of an external api start/stop
void
i2c_read_write(bool write, byte *buffer, int length)
{
    if (! initialized) {
        i2c_initialize();
    }

    breaks = run_breaks;

    // if we need a start...
    if (! started) {
        // if we timed out...
        if (! i2c_start_real(write)) {
            return;
        }
    } else {
        // we need a restart
        if (! i2c_repeat_start_real(write)) {
            return;
        }
    }

    if (write) {
        while (length--) {
            // wait for transmitter ready
            if (! i2c_wait(write)) {
                return;
            }

            // send data
            I2C1TRN = *buffer++;

            if (! i2c_wait(write)) {
                return;
            }

            // if no ack...
            if (I2C1STATbits.ACKSTAT) {
                i2c_reset(-1);
                return;
            }
        }
    } else {
        while (length--) {
            // wait for byte received and previous acknowledge
            if (! i2c_wait(write)) {
                return;
            }

            // if this is not the last byte...
            if (length) {
                // ack
                I2C1CONCLR = _I2C1CON_ACKDT_MASK;
                I2C1CONbits.ACKEN = 1;
                if (! i2c_wait(false)) {
                    return;
                }

                // enable receive
                I2C1CONbits.RCEN = 1;
            } else {
                // no ack
                I2C1CONSET = _I2C1CON_ACKDT_MASK;
                I2C1CONbits.ACKEN = 1;
                if (! i2c_wait(false)) {
                    return;
                }
            }

            // get the data
            *buffer++ = I2C1RCV;
        }
    }
}

// external api; we're done talking to this i2c device address
void
i2c_stop(void)
{
    assert(started);

    breaks = run_breaks;

    if (! i2c_wait(true)) {
        return;
    }

    // generate stop
    I2C1CONbits.PEN = 1;

    if (! i2c_wait(true)) {
        return;
    }

    started = false;
    return;
}

// release i2c pins to other users; disable i2c module
void
i2c_uninitialize(void)
{
    initialized = false;

    I2C1CONbits.ON = false;

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // restore default pin usage
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
#endif
}

// claim pins for i2c use; enable i2c module
void
i2c_initialize(void)
{
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // hijack these pins while i2c is in use
    // XXX -- we should prevent someone else from using these pins!
    RPG8R = 0;  // undo OC4 -> RG8
    RPG7R = 0;  // undo OC2 -> RG7
#endif

    I2C1CONbits.ON = false;
    I2C1BRG = ((bus_frequency/I2C_BAUD)/2) - 2;
    I2C1CONbits.DISSLW = 1;  // for 100 kHz
    I2C1CONbits.ON = true;

    initialized = true;
}
