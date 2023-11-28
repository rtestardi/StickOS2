// *** serial.c *******************************************************
// This file implements an interrupt driven serial console.

#include "main.h"
#include <sys/attribs.h>

#define BAUDRATE  9600

#define BUFFERSIZE  64

static byte rxbuffer[BUFFERSIZE];
static int rxlength;  // number of rx bytes in buffer

static bool waiting;  // we have sent Ctrl-S at end of command
static bool suspend;  // set this to send Ctrl-S in next send() loop
static bool resume;  // set this to send Ctrl-Q in next send() loop
static bool busy;  // we're in the middle of a send() loop

#define CTRLS  ('S'-'@')  // suspend
#define CTRLQ  ('Q'-'@')  // resume

bool serial_active;

int serial_baudrate;

#if SODEBUG
volatile bool serial_in_isr;
volatile int32 serial_in_ticks;
volatile int32 serial_out_ticks;
volatile int32 serial_max_ticks;
#endif

// this function disables the serial isr when BASIC takes over control
void
serial_disable(void)
{
    serial_active = false;

    // Unconfigure UART2 RX Interrupt
#if SERIAL_UART
    IEC1CLR = _IEC1_U2RXIE_MASK;
#else
    IEC1CLR = _IEC1_U1RXIE_MASK;
#endif

    // don't allow the rx ball to start rolling
    waiting = false;
}


// this function acknowledges receipt of a serial command from upper
// level code.
void
serial_command_ack(void)
{
    int x;
    bool boo;

    ASSERT(gpl() == 0);

    x = splx(5);

    // if we held off the serial rx...
    if (waiting) {
        // receive new characters
        if (rxlength) {
            // accumulate new commands
            boo = terminal_receive(rxbuffer, rxlength);
            rxlength = 0;

            // if a full command was accumulated...
            if (! boo) {
                // wait for another terminal_command_ack();
                goto XXX_SKIP_XXX;
            }
        }

        // start the rx ball rolling
        waiting = false;

        // if the tx is ready...
        if (pin_uart_tx_ready(SERIAL_UART)) {
            // stuff a resume
            pin_uart_tx(SERIAL_UART, CTRLQ);
        } else {
            // record we need a resume
            suspend = false;
            resume = true;
            assert(busy);
        }
    }

XXX_SKIP_XXX:
    splx(x);
}

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define _UART_2_VECTOR  _UART2_RX_VECTOR
#define _UART_1_VECTOR  _UART1_RX_VECTOR
#endif

void
#if SERIAL_UART
__ISR(_UART_2_VECTOR, PIC32IPL4)
#else
__ISR(_UART_1_VECTOR, PIC32IPL4)
#endif
serial_isr(void)
{
    char c;
    bool boo;

    assert(! serial_in_isr);
    assert((serial_in_isr = true) ? true : true);
    assert((serial_in_ticks = ticks) ? true : true);

    // Clear the RX interrupt Flag
#if SERIAL_UART
    IFS1CLR = _IFS1_U2RXIF_MASK;
#else
    IFS1CLR = _IFS1_U1RXIF_MASK;
#endif

    do {
        // process the receive fifo
        while (pin_uart_rx_ready(SERIAL_UART)) {
            c = pin_uart_rx(SERIAL_UART);
            if (c && c != CTRLS && c != CTRLQ) {
                if (c == '\r') {
                    serial_active = true;
                }
                rxbuffer[rxlength++] = c;
                assert(rxlength < sizeof(rxbuffer));
            }
        }


        // receive characters
        if (rxlength && ! waiting) {
            // accumulate commands
            boo = terminal_receive(rxbuffer, rxlength);
            rxlength = 0;

            // if a full command was accumulated...
            if (! boo) {
                // if the tx is ready...
                if (pin_uart_tx_ready(SERIAL_UART)) {
                    // stuff a suspend
                    pin_uart_tx(SERIAL_UART, CTRLS);
                } else {
                    // record we need a suspend
                    suspend = true;
                    resume = false;
                    assert(busy);
                }

                // drop the ball
                waiting = true;
            }
        }
    } while (pin_uart_rx_ready(SERIAL_UART));

    assert(serial_in_isr);
    assert((serial_in_isr = false) ? true : true);
    assert((serial_out_ticks = ticks) ? true : true);
    assert((serial_max_ticks = MAX(serial_max_ticks, serial_out_ticks-serial_in_ticks)) ? true : true);
}

void
serial_send(const byte *buffer, int length)
{
    ASSERT(gpl() == 0);

    assert(! busy);
    busy = true;

    while (! pin_uart_tx_ready(SERIAL_UART)) {
        // revisit -- poll?
    }

    // send the characters synchronously
    for (;;) {
        // if we have a suspend or resume to send, they take precedence
        if (suspend) {
            pin_uart_tx(SERIAL_UART, CTRLS);
            suspend = false;
            assert(! resume);
        } else if (resume) {
            pin_uart_tx(SERIAL_UART, CTRLQ);
            resume = false;
            assert(! suspend);
        } else {
            if (! length) {
                break;
            }

            pin_uart_tx(SERIAL_UART, *buffer);
            buffer++;
            length--;
        }

        while (! pin_uart_tx_ready(SERIAL_UART)) {
            // revisit -- poll?
        }
    }

    assert(busy);
    busy = false;
}

// this function disables the serial isr when USB takes over control of the console
void
serial_uninitialize(void)
{
    //U1RXR = 0;  // no longer RB13 -> U1RX
    //RPB15R = 0;  // no longer U1TX -> RB15
    //U2RXR = 0;  // no longer RB11 -> U2RX
    //RPB14R = 0;  // no longer U2TX -> RB14

#if SERIAL_UART
    U2MODE &= ~_U2MODE_UARTEN_MASK;

    // Unconfigure UART2 RX Interrupt
    IEC1CLR = _IEC1_U2RXIE_MASK;
#else
    U1MODE &= ~_U1MODE_UARTEN_MASK;

    // Unconfigure UART2 RX Interrupt
    IEC1CLR = _IEC1_U1RXIE_MASK;
#endif
}

void
serial_initialize(void)
{
    // get the baudrate from flash; revert to 9600 on autorun disable
    serial_baudrate = var_get_flash(FLASH_BAUD);
    if (! serial_baudrate || serial_baudrate == -1 || disable_autorun) {
        serial_baudrate = BAUDRATE;
    }

    // configure the first uart for serial terminal by default
    pin_uart_configure(SERIAL_UART, serial_baudrate, 8, 2, false);

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#if ! SERIAL_UART
#error
#endif
    U1RXR = 3;  // RB13 -> U1RX
    RPB15R = 1;  // U1TX -> RB15
    U2RXR = 3;  // RB11 -> U2RX
    RPB14R = 2;  // U2TX -> RB14

    IEC1bits.U2RXIE = 1;
    IPC14bits.U2RXIP = 4;
    IPC14bits.U2RXIS = 0;
#endif

    // start us out with a CTRLQ
    pin_uart_tx(SERIAL_UART, CTRLQ);

#if SERIAL_UART
    U2MODE |= _U2MODE_UARTEN_MASK;

    // Configure UART2 RX Interrupt
    IEC1SET = _IEC1_U2RXIE_MASK;
#else
    U1MODE |= _U1MODE_UARTEN_MASK;

    // Configure UART1 RX Interrupt
    IEC1SET = _IEC1_U1RXIE_MASK;
#endif
}

