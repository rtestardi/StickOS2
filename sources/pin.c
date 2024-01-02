// *** pin.c **********************************************************
// this file implements basic I/O pin controls.

#include "main.h"

#define SHRINK  0

int servo_hz = 45;

#define SERVO_MAX (1000000/servo_hz)  // microseconds
#define SERVO_PRESCALE  64
#define SERVO_MOD  (bus_frequency/SERVO_PRESCALE/servo_hz)

const char * const pin_assignment_names[] = {
    "heartbeat",
    "safemode*",
    "qspi_cs*",  // zigflea
    "zigflea_rst*",
    "zigflea_attn*",
    "zigflea_rxtxen",
};

byte pin_assignments[pin_assignment_max] = {
    // set our default pin assignments
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    PIN_E1, PIN_S1, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED, PIN_UNASSIGNED,
#else
#error
#endif
};

const char * const pin_type_names[] = {
    "digital input",
    "digital output",
    "analog input",
    "analog output",
    "uart input",
    "uart output",
    "frequency output",
    "servo output"
};

byte const pin_qual_mask[] = {
    1<<pin_qual_inverted | 1<<pin_qual_debounced,  // digital input
    1<<pin_qual_inverted | 1<<pin_qual_open_drain,  // digital output
    1<<pin_qual_inverted | 1<<pin_qual_debounced,  // analog input
    1<<pin_qual_inverted,  // analog output
    0,  // uart input
    0,  // uart output
    0,  // frequency output
    1<<pin_qual_inverted  // servo output
};

// Keep in-sync with pin_qual.  Each element in this array corresponds to a bit in pin_qual.
const char * const pin_qual_names[] = {
    "debounced",
    "inverted",
    "open_drain"
};

int pin_last = PIN_LAST;

int32 pin_analog = 3300;

static byte declared[(PIN_UNASSIGNED+7)/8];

#define DIO  (1<<pin_type_digital_output|1<<pin_type_digital_input)

const struct pin pins[] = {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    "a0", DIO,
    "a1", DIO|1<<pin_type_analog_input,
    "a2", DIO|1<<pin_type_analog_input,
    "a3", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc5
    "a4", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc6
    "a5", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc3
    "a6", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc4
    "a7", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc2
    "a8", DIO|1<<pin_type_analog_input|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  //oc1
    "b0", DIO,
    "b1", DIO,
    "b2", DIO,
    "b3", DIO,
    "b4", DIO|1<<pin_type_uart_input,
    "b5", DIO,
    "b6", DIO|1<<pin_type_uart_input,
    "b7", DIO|1<<pin_type_uart_output,
    "b8", DIO|1<<pin_type_uart_output,
    "e1", DIO,
    "e2", DIO,
    "e3", DIO,
    "s1", DIO,
#else
#error
#endif
    "none", 0,
};

bool uart_armed[UART_INTS];

const char * const uart_names[MAX_UARTS] = {
    "1",
    "2",
};

static byte freq[2];  // 0, pin_type_analog_output, pin_type_servo_output, pin_type_frequency_output

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
#define FREQ_PRESCALE  256
#else
#error
#endif

#if ! STICK_GUEST

// Debounce history for digital inputs.

enum {
    // Tunable depth of digital pin history saved for debouncing.  Currently, the history is used to elect a majority.
    pin_digital_debounce_history_depth = 3
};

enum debounce_ports {
    port_a,
    port_b,
    port_c,
    port_d,
    port_e,
    port_f,
    port_g,
    port_max
};

// This structure records recent samples from digital pins.
typedef uint16 pin_port_sample_t;

static pin_port_sample_t pin_digital_debounce[pin_digital_debounce_history_depth][port_max];

static int pin_digital_debounce_cycle; // indexes into pin_digital_debounce_data.

// compute majority value of the pin's recently polled values.
//
// revisit -- find a way to rework and rename this to
// pin_get_digital() and have it returned debounced or non-debounced
// data based on pin_qual.  Also introduce a pin_set_digital() to
// consolidate setting of pins.
static int
pin_get_digital_debounced(int port_offset, int pin_offset)
{
    int i;
    int value;

    assert(pin_offset < sizeof(pin_digital_debounce[0][0]) * 8);

    value = 0;
    for (i = 0; i < pin_digital_debounce_history_depth; i++) {
        value += !!(pin_digital_debounce[i][port_offset] & (1 << pin_offset));
    }
    return value > pin_digital_debounce_history_depth/2;
}

#endif // ! STICK_GUEST

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
unsigned char pin_to_an(int pin)
{
    if (pin == PIN_A1) {
        return 0;
    } else if (pin == PIN_A2) {
        return 1;
    } else if (pin == PIN_A3){
        return 2;
    } else if (pin == PIN_A4){
        return 3;
    } else if (pin == PIN_A5){
        return 4;
    } else if (pin == PIN_A6){
        return 5;
    } else if (pin == PIN_A7){
        return 6;
    } else if (pin == PIN_A8){
        return 7;
    } else {
        assert(0);
        return -1;
    }
}
#endif


static
void
pin_declare_internal(IN int pin_number, IN int pin_type, IN int pin_qual, IN bool set, IN bool user)
{
#if ! STICK_GUEST && ! SHRINK
    uint32 offset;

    assert(pin_number != PIN_UNASSIGNED);
    if (user) {
        declared[pin_number/8] |= 1 << (pin_number%8);
    }

    if (! set && (pin_type == pin_type_digital_output) && (pin_qual & 1<<pin_qual_open_drain)) {
        // on initial declaration, configure open_drain outputs as inputs
        // N.B. this will be reconfigured as an output on pin_set to 0
        pin_type = pin_type_digital_input;
    }

    // configure the PIC32 pin for the requested function
    switch (pin_number) {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
        // configure MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = 12 + pin_number - PIN_A1;
            assert(offset >= 12 && offset <= 15);
            CNPUECLR = 1 << offset;
            if (pin_type == pin_type_analog_input) {
                ANSELESET = 1 << offset;
                TRISESET = 1 << offset;
            } else if (pin_type == pin_type_digital_input) {
                ANSELECLR = 1 << offset;
                CNPUESET = 1 << offset;
                TRISESET = 1 << offset;
            } else {
                // output
                ANSELECLR = 1 << offset;
                TRISECLR = 1 << offset;
            }
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output) {
                if (freq[0] && freq[0] != pin_type) {
                    printf("conflicting timer usage\n");
                    stop();
                } else {
                    if (! freq[0]) {
                        if (pin_type == pin_type_analog_output) {
                            // configure timer 2 for analog pwm generation
                            // set pwm prescale to 1
                            T2CON = 0;
                            TMR2 = 0;
                            PR2 = pin_analog-1;
                            T2CON = _T2CON_ON_MASK;
                        } else {
                            // configure timer 2 for servo pwm generation
                            // set pwm prescale to 64
                            T2CON = 0;
                            TMR2 = 0;
                            PR2 = SERVO_MOD-1;
                            assert(SERVO_PRESCALE == 64);
                            T2CON = _T2CON_ON_MASK|(6 << _T2CON_TCKPS_POSITION);
                        }
                        freq[0] = pin_type;
                    }
                }
            } else if (pin_type == pin_type_frequency_output) {
                if (! freq[1]) {
                    freq[1] = pin_type_frequency_output;
                    // NULL
                } else {
                    printf("conflicting timer usage\n");
                    stop();
                }
            } else {
                // not analog, servo, or frequency output
                if (pin_number == PIN_A3) {
                    OC5CONCLR = _OC5CON_ON_MASK;
                } else if (pin_number == PIN_A4) {
                    OC6CONCLR = _OC6CON_ON_MASK;
                }
            }
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = 6 + PIN_A8 - pin_number;
            assert(offset >= 6 && offset <= 9);
            CNPUGCLR = 1 << offset;
            if (pin_type == pin_type_analog_input) {
                ANSELGSET = 1 << offset;
                TRISGSET = 1 << offset;
            } else if (pin_type == pin_type_digital_input) {
                ANSELGCLR = 1 << offset;
                CNPUGSET = 1 << offset;
                TRISGSET = 1 << offset;
            } else {
                // output
                ANSELGCLR = 1 << offset;
                TRISGCLR = 1 << offset;
            }
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output) {
                if (freq[0] && freq[0] != pin_type) {
                    printf("conflicting timer usage\n");
                    stop();
                } else {
                    if (! freq[0]) {
                        if (pin_type == pin_type_analog_output) {
                            // configure timer 2 for analog pwm generation
                            // set pwm prescale to 1
                            T2CON = 0;
                            TMR2 = 0;
                            PR2 = pin_analog-1;
                            T2CON = _T2CON_ON_MASK;
                        } else {
                            // configure timer 2 for servo pwm generation
                            // set pwm prescale to 64
                            T2CON = 0;
                            TMR2 = 0;
                            PR2 = SERVO_MOD-1;
                            assert(SERVO_PRESCALE == 64);
                            T2CON = _T2CON_ON_MASK|(6 << _T2CON_TCKPS_POSITION);
                        }
                        freq[0] = pin_type;
                    }
                }
            } else if (pin_type == pin_type_frequency_output) {
                if (! freq[1]) {
                    freq[1] = pin_type_frequency_output;
                    // NULL
                } else {
                    printf("conflicting timer usage\n");
                    stop();
                }
            } else {
                // not analog, servo, or frequency output
                if (pin_number == PIN_A5) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                } else if (pin_number == PIN_A6) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                } else if (pin_number == PIN_A7) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                } else if (pin_number == PIN_A8) {
                    OC1CONCLR = _OC1CON_ON_MASK;
                }
            }
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = 4 + pin_number - PIN_B0;
            ASSERT(offset >= 4 && offset <= 6);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                // output
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            offset = 10 + pin_number - PIN_B3;
            assert(offset >= 10 && offset <= 15);
            CNPUBCLR = 1 << offset;
            if (pin_type == pin_type_uart_input || pin_type == pin_type_uart_output) {
                ANSELBCLR = 1 << offset;
                TRISBSET = 1 << offset;
            } else if (pin_type == pin_type_digital_input) {
                ANSELBCLR = 1 << offset;
                CNPUBSET = 1 << offset;
                TRISBSET = 1 << offset;
            } else {
                // output
                ANSELBCLR = 1 << offset;
                TRISBCLR = 1 << offset;
            }
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            CNPUCCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELCCLR = 1 << offset;
                CNPUCSET = 1 << offset;
                TRISCSET = 1 << offset;
            } else {
                // output
                ANSELCCLR = 1 << offset;
                TRISCCLR = 1 << offset;
            }
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = 4 + (pin_number - PIN_E2)*6;
            assert(offset == 4 || offset == 10);
            CNPUACLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELACLR = 1 << offset;
                CNPUASET = 1 << offset;
                TRISASET = 1 << offset;
            } else {
                // output
                ANSELACLR = 1 << offset;
                TRISACLR = 1 << offset;
            }
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            CNPUCCLR = 1 << offset;
            if (pin_type == pin_type_digital_input) {
                ANSELCCLR = 1 << offset;
                CNPUCSET = 1 << offset;
                TRISCSET = 1 << offset;
            } else {
                // output
                ANSELCCLR = 1 << offset;
                TRISCCLR = 1 << offset;
            }
            break;
#else
#error
#endif
            break;
        default:
            assert(0);
            break;
    }
#endif
}

// this function declares a ram, flash, or pin variable!
void
pin_declare(IN int pin_number, IN int pin_type, IN int pin_qual)
{
    pin_declare_internal(pin_number, pin_type, pin_qual, false, true);
}

static byte ulasttx[MAX_UARTS];
static byte umask[MAX_UARTS];

// this function sets a pin variable!
void
pin_set(IN int pin_number, IN int pin_type, IN int pin_qual, IN int32 value)
{
#if ! STICK_GUEST && ! SHRINK
    int32 value2;
    uint32 offset;

    if (pin_number == PIN_UNASSIGNED) {
        return;
    }

    if (pin_type == pin_type_analog_output) {
        // trim the analog level
        if (value < 0) {
            value = 0;
        } else if (value > pin_analog) {
            value = pin_analog;
        }
    } else if (pin_type == pin_type_servo_output) {
        // trim the servo level
        if (value < 0) {
            value = 0;
        } else if (value > SERVO_MAX) {
            value = SERVO_MAX;
        }
    } else if (pin_type == pin_type_frequency_output) {
        // trim the frequency
        if (value < 0) {
            value = 0;
        }
        if (value) {
            value = bus_frequency/FREQ_PRESCALE/value/2;
            if (value) {
                value--;
            }
        }
        if (value > 0xffff) {
            value = 0xffff;
        }
    }

    if (pin_qual & 1<<pin_qual_inverted) {
        if (pin_type == pin_type_digital_output) {
            value2 = value;
            value = ! value;
            ASSERT(value != value2);  // catch CW bug
        } else if (pin_type == pin_type_analog_output) {
            value2 = value;
            value = pin_analog-value;
            ASSERT(value+value2 == pin_analog);  // catch CW bug
        } else if (pin_type == pin_type_servo_output) {
            value2 = value;
            value = SERVO_MAX-value;
            ASSERT(value+value2 == SERVO_MAX);  // catch CW bug
        }
    }

    // If setting to 1, then disable the driver before setting the data value to 1.  This avoids having the processor drive the
    // open_drain pin, which would be bad if the line is held high by another driver.
    if (value && (pin_qual & 1<<pin_qual_open_drain)) {
        assert(pin_type == pin_type_digital_output);
        pin_declare_internal(pin_number, pin_type_digital_input, pin_qual, true, false);
    }

    // set the PIC32 pin to value
    switch (pin_number) {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
        // set MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            if (value) {
                LATBSET = 1 << offset;
            } else {
                LATBCLR = 1 << offset;
            }
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = 12 + pin_number - PIN_A1;
            assert(offset >= 12 && offset <= 15);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output) {
                if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MOD/SERVO_MAX;
                }
                if (pin_number == PIN_A3) {
                    OC5CONCLR = _OC5CON_ON_MASK;
                    OC5R = 0;
                    OC5RS = value;
                    OC5CON = _OC5CON_ON_MASK|(6<<_OC5CON_OCM0_POSITION);
                } else {
                    assert(pin_number == PIN_A4);
                    OC6CONCLR = _OC6CON_ON_MASK;
                    OC6R = 0;
                    OC6RS = value;
                    OC6CON = _OC6CON_ON_MASK|(6<<_OC6CON_OCM0_POSITION);
                }
            } else if (pin_type == pin_type_frequency_output) {
                // configure timer 3 for frequency generation
                T3CONCLR = _T3CON_ON_MASK;
                TMR3 = 0;
                PR3 = value;
                if (pin_number == PIN_A3) {
                    OC5CONCLR = _OC5CON_ON_MASK;
                    OC5R = 0;
                    OC5RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC5CON = _OC5CON_ON_MASK|_OC5CON_OCTSEL_MASK|(3<<_OC5CON_OCM0_POSITION);
                    }
                } else {
                    assert(pin_number == PIN_A4);
                    OC6CONCLR = _OC6CON_ON_MASK;
                    OC6R = 0;
                    OC6RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC6CON = _OC6CON_ON_MASK|_OC6CON_OCTSEL_MASK|(3<<_OC6CON_OCM0_POSITION);
                    }
                }
                // set timer prescale to 256
                T3CON = _T3CON_ON_MASK|(7<<_T3CON_TCKPS_POSITION);
                assert(FREQ_PRESCALE == 256);
            } else {
                assert(pin_type == pin_type_digital_output);
                if (value) {
                    LATESET = 1 << offset;
                } else {
                    LATECLR = 1 << offset;
                }
            }
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = 6 + PIN_A8 - pin_number;
            assert(offset >= 6 && offset <= 9);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output) {
                if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MOD/SERVO_MAX;
                }
                if (pin_number == PIN_A5) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                    OC3R = 0;
                    OC3RS = value;
                    OC3CON = _OC3CON_ON_MASK|(6<<_OC3CON_OCM0_POSITION);
                } else if (pin_number == PIN_A6) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                    OC4R = 0;
                    OC4RS = value;
                    OC4CON = _OC4CON_ON_MASK|(6<<_OC4CON_OCM0_POSITION);
                } else if (pin_number == PIN_A7) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                    OC2R = 0;
                    OC2RS = value;
                    OC2CON = _OC2CON_ON_MASK|(6<<_OC2CON_OCM0_POSITION);
                } else {
                    assert(pin_number == PIN_A8);
                    OC1CONCLR = _OC6CON_ON_MASK;
                    OC1R = 0;
                    OC1RS = value;
                    OC1CON = _OC1CON_ON_MASK|(6<<_OC1CON_OCM0_POSITION);
                }
            } else if (pin_type == pin_type_frequency_output) {
                // configure timer 3 for frequency generation
                T3CONCLR = _T3CON_ON_MASK;
                TMR3 = 0;
                PR3 = value;
                if (pin_number == PIN_A5) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                    OC3R = 0;
                    OC3RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC3CON = _OC3CON_ON_MASK|_OC3CON_OCTSEL_MASK|(3<<_OC3CON_OCM0_POSITION);
                    }
                } else if (pin_number == PIN_A6) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                    OC4R = 0;
                    OC4RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC4CON = _OC4CON_ON_MASK|_OC4CON_OCTSEL_MASK|(3<<_OC4CON_OCM0_POSITION);
                    }
                } else if (pin_number == PIN_A7) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                    OC2R = 0;
                    OC2RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC2CON = _OC2CON_ON_MASK|_OC2CON_OCTSEL_MASK|(3<<_OC2CON_OCM0_POSITION);
                    }
                } else {
                    assert(pin_number == PIN_A8);
                    OC1CONCLR = _OC1CON_ON_MASK;
                    OC1R = 0;
                    OC1RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC1CON = _OC1CON_ON_MASK|_OC1CON_OCTSEL_MASK|(3<<_OC1CON_OCM0_POSITION);
                    }
                }
                // set timer prescale to 256
                T3CON = _T3CON_ON_MASK|(7<<_T3CON_TCKPS_POSITION);
                assert(FREQ_PRESCALE == 256);
            } else {
                assert(pin_type == pin_type_digital_output);
                if (value) {
                    LATGSET = 1 << offset;
                } else {
                    LATGCLR = 1 << offset;
                }
            }
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = 4 + pin_number - PIN_B0;
            ASSERT(offset >= 4 && offset <= 6);
            if (value) {
                LATBSET = 1 << offset;
            } else {
                LATBCLR = 1 << offset;
            }
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            if (pin_type == pin_type_uart_output) {
                assert(pin_number == PIN_B7 || pin_number == PIN_B8);
                pin_uart_tx(pin_number == PIN_B7, value);
            } else {
                offset = 10 + pin_number - PIN_B3;
                assert(offset >= 10 && offset <= 15);
                if (value) {
                    LATBSET = 1 << offset;
                } else {
                    LATBCLR = 1 << offset;
                }
            }
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            if (value) {
                LATCSET = 1 << offset;
            } else {
                LATCCLR = 1 << offset;
            }
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = 4 + (pin_number - PIN_E2)*6;
            assert(offset == 4 || offset == 10);
            if (value) {
                LATASET = 1 << offset;
            } else {
                LATACLR = 1 << offset;
            }
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            if (value) {
                LATCSET = 1 << offset;
            } else {
                LATCCLR = 1 << offset;
            }
            break;
#else
#error
#endif
        default:
            assert(0);
            break;
    }
#endif // ! STICK_GUEST

    // if setting to 0, then enable the pin driver after the pin data value has been set to 0.  This prevents the processor from
    // driving a 1 (which the pin's latch may held at the start of this function).
    if ((! value) && (pin_qual & 1<<pin_qual_open_drain)) {
        assert(pin_type == pin_type_digital_output);
        pin_declare_internal(pin_number, pin_type_digital_output, pin_qual, true, false);
    }
}

// this function gets a pin variable!
int32
pin_get(IN int pin_number, IN int pin_type, IN int pin_qual)
{
#if ! STICK_GUEST && ! SHRINK
    int32 value;
    uint32 offset;

    value = 0;

    if (pin_number == PIN_UNASSIGNED) {
        return value;
    }

    // get the value of the PIC32 pin
    switch (pin_number) {
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
        // get MK
        case PIN_A0:
            // RB
            offset = 7;
            assert(offset == 7);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_b, offset);
            } else {
              value = !! (PORTB & 1 << offset);
            }
            break;
        case PIN_A1:
        case PIN_A2:
        case PIN_A3:
        case PIN_A4:
            // RE
            offset = 12 + pin_number - PIN_A1;
            assert(offset >= 12 && offset <= 15);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output || pin_type == pin_type_frequency_output) {
                // XXX -- servo pins get 2 less than set
                if (pin_number == PIN_A3) {
                    value = OC5RS + ((pin_type == pin_type_frequency_output && (OC5CON & _OC5CON_ON_MASK) == 0)?1000000000:0);
                } else {
                    assert(pin_number == PIN_A4);
                    value = OC6RS + ((pin_type == pin_type_frequency_output && (OC6CON & _OC6CON_ON_MASK) == 0)?1000000000:0);
                }
                if (pin_type == pin_type_frequency_output) {
                    if (! value) {
                        value = 0x10000;
                    }
                    value = bus_frequency/FREQ_PRESCALE/(value+1)/2;
                } else if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MAX/SERVO_MOD;
                }
            } else if (pin_type == pin_type_analog_input) {
                value = adc_get_value(pin_to_an(pin_number), pin_qual);
            } else if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_e, offset);
            } else {
                value = !! (PORTE & 1 << offset);
            }
            break;
        case PIN_A5:
        case PIN_A6:
        case PIN_A7:
        case PIN_A8:
            // RG
            offset = 6 + PIN_A8 - pin_number;
            assert(offset >= 6 && offset <= 9);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output || pin_type == pin_type_frequency_output) {
                // XXX -- servo pins get 2 less than set
                if (pin_number == PIN_A5) {
                    value = OC3RS + ((pin_type == pin_type_frequency_output && (OC3CON & _OC3CON_ON_MASK) == 0)?1000000000:0);
                } else if (pin_number == PIN_A6) {
                    value = OC4RS + ((pin_type == pin_type_frequency_output && (OC4CON & _OC4CON_ON_MASK) == 0)?1000000000:0);
                } else if (pin_number == PIN_A7) {
                    value = OC2RS + ((pin_type == pin_type_frequency_output && (OC2CON & _OC2CON_ON_MASK) == 0)?1000000000:0);
                } else {
                    assert(pin_number == PIN_A8);
                    value = OC1RS + ((pin_type == pin_type_frequency_output && (OC1CON & _OC1CON_ON_MASK) == 0)?1000000000:0);
                }
                if (pin_type == pin_type_frequency_output) {
                    if (! value) {
                        value = 0x10000;
                    }
                    value = bus_frequency/FREQ_PRESCALE/(value+1)/2;
                } else if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MAX/SERVO_MOD;
                }
            } else if (pin_type == pin_type_analog_input) {
                value = adc_get_value(pin_to_an(pin_number), pin_qual);
            } else if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_g, offset);
            } else {
                value = !! (PORTG & 1 << offset);
            }
            break;
        case PIN_B0:
        case PIN_B1:
        case PIN_B2:
            // RB
            offset = 4 + pin_number - PIN_B0;
            ASSERT(offset >= 4 && offset <= 6);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_b, offset);
            } else {
                value = !! (PORTB & 1 << offset);
            }
            break;
        case PIN_B3:
        case PIN_B4:
        case PIN_B5:
        case PIN_B6:
        case PIN_B7:
        case PIN_B8:
            // RB
            if (pin_type == pin_type_uart_input) {
                assert(pin_number == PIN_B4 || pin_number == PIN_B6);
                value = pin_uart_rx(pin_number == PIN_B4);
            } else if (pin_type == pin_type_uart_output) {
                assert(pin_number == PIN_B7 || pin_number == PIN_B8);
                value = pin_uart_tx_empty(pin_number == PIN_B7)?0:ulasttx[pin_number == PIN_B7];
            } else {
                offset = 10 + pin_number - PIN_B3;
                assert(offset >= 10 && offset <= 15);
                if (pin_qual & 1<<pin_qual_debounced) {
                    assert(pin_type == pin_type_digital_input);
                    value = pin_get_digital_debounced(port_b, offset);
                } else {
                    value = !! (PORTB & 1 << offset);
                }
            }
            break;
        case PIN_E1:
            // RC
            offset = 6;
            assert(offset == 6);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_c, offset);
            } else {
                value = !! (PORTC & 1 << offset);
            }
            break;
        case PIN_E2:
        case PIN_E3:
            // RA
            offset = 4 + (pin_number - PIN_E2)*6;
            assert(offset == 4 || offset == 10);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_a, offset);
            } else {
                value = !! (PORTA & 1 << offset);
            }
            break;
        case PIN_S1:
            // RC
            offset = 7;
            assert(offset == 7);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_c, offset);
            } else {
                value = !! (PORTC & 1 << offset);
            }
            break;
#else
#error
#endif
        default:
            assert(0);
            break;
    }

    if (pin_type == pin_type_digital_input || pin_type == pin_type_digital_output) {
        value = !!value;
    }

    if (pin_qual & (1<<pin_qual_inverted)) {
        if (pin_type == pin_type_digital_input || pin_type == pin_type_digital_output) {
            value = ! value;
        } else if (pin_type == pin_type_analog_input || pin_type == pin_type_analog_output) {
            value = pin_analog-value;
        } else if (pin_type == pin_type_servo_output) {
            value = SERVO_MAX-value;
        }
    }

    return value;
#else
    return 0;
#endif
}


// XXX -- could we uninitialize on run_clear()?
// N.B. parity: 0 -> even; 1 -> odd; 2 -> none
void
pin_uart_configure(int uart, int baud, int data, byte parity, bool loopback)
{
#if ! STICK_GUEST
    int divisor;

    if (! uart) {
        U1MODE &= ~_U1MODE_UARTEN_MASK;

        divisor = bus_frequency/baud/16;
        if (divisor >= 0x10000) {
            divisor = 0xffff;
        }
        U1BRG = divisor;

        U1MODE = _U1MODE_UARTEN_MASK|(loopback?_U1MODE_LPBACK_MASK:0)|((data==8&&parity!=2)?(parity?_U1MODE_PDSEL1_MASK:_U1MODE_PDSEL0_MASK):0);

        U1STA = _U1STA_URXEN_MASK|_U1STA_UTXEN_MASK;
    } else {
        U2MODE &= ~_U2MODE_UARTEN_MASK;

        divisor = bus_frequency/baud/16;
        if (divisor >= 0x10000) {
            divisor = 0xffff;
        }
        U2BRG = divisor;

        U2MODE = _U2MODE_UARTEN_MASK|(loopback?_U2MODE_LPBACK_MASK:0)|((data==8&&parity!=2)?(parity?_U2MODE_PDSEL1_MASK:_U2MODE_PDSEL0_MASK):0);

        U2STA = _U2STA_URXEN_MASK|_U2STA_UTXEN_MASK;
    }
#endif
    umask[uart] = data==8?0xff:0x7f;
}

bool
pin_uart_tx_ready(int uart)
{
#if ! STICK_GUEST
    int usr;

    // if the uart transmitter is ready...
    usr = uart?U2STA:U1STA;
    if (! (usr & _U1STA_UTXBF_MASK)) {
        return true;
    }
    return false;
#else
    return true;
#endif
}

bool
pin_uart_tx_empty(int uart)
{
#if ! STICK_GUEST
    int usr;

    // if the uart transmitter is empty...
    usr = uart?U2STA:U1STA;
    if (usr & _U1STA_TRMT_MASK) {
        return true;
    }
    return false;
#else
    return true;
#endif
}

bool
pin_uart_rx_ready(int uart)
{
#if ! STICK_GUEST
    int usr;

    // if the uart receiver is ready...
    usr = uart?U2STA:U1STA;
    if (usr & _U1STA_URXDA_MASK) {
        return true;
    }
#endif
    return false;
}

void
pin_uart_tx(int uart, byte value)
{
    int x;
    x = splx(5);

    while (! pin_uart_tx_ready(uart)) {
        splx(x);
        assert(gpl() < SPL_SERIAL);
        // revisit -- poll?
        x = splx(5);
    }

#if ! STICK_GUEST
    if (uart) {
        U2TXREG = value;
    } else {
        U1TXREG = value;
    }
#endif
    ulasttx[uart] = value;
    uart_armed[UART_INT(uart, true)] = true;

    splx(x);
}

byte
pin_uart_rx(int uart)
{
    int x;
    byte value;

    x = splx(5);

    if (pin_uart_rx_ready(uart)) {
#if ! STICK_GUEST
        if (uart) {
            value = U2RXREG;
            U2STACLR = _U2STA_OERR_MASK;
        } else {
            value = U1RXREG;
            U1STACLR = _U1STA_OERR_MASK;
        }
#else
        value = 0;
#endif
        value = value & umask[uart];
    } else {
        value = 0;
    }

    uart_armed[UART_INT(uart, false)] = true;

    splx(x);

    return value;
}


void
pin_clear(void)
{
    int i;

    // N.B. we use 0 to mean no frequency type
    assert(! pin_type_digital_input);

#if ! STICK_GUEST
    i2c_uninitialize();
    qspi_uninitialize();
    // XXX -- should we unconfigure uarts?
#endif

    // reset all explicitly declared pins to digital input
    for (i = 0; i < PIN_UNASSIGNED; i++) {
        if (declared[i/8] & (1 << (i%8))) {
            if (pins[i].pin_type_mask & (1 << pin_type_digital_input)) {
                pin_declare_internal(i, pin_type_digital_input, 0, false, false);
            }
        }
    }

#if ! STICK_GUEST
    // we have to manage shared timer resources across pins
    // REVISIT -- for now, we force timer 2 to pwm mode, disallowing two
    // different frequency output pins; we can do better in the long run
    // by dynamically allocating both timer 2 and 3 as needed.
    memset(freq, pin_type_digital_input, sizeof(freq));
#endif
}

// only called on debouncing ticks from timer_isr()
void
pin_timer_poll(void)
{
#if ! STICK_GUEST
    pin_port_sample_t *sample;

    sample = pin_digital_debounce[pin_digital_debounce_cycle];

    // for each port...
#ifdef PORTA
    sample[port_a] = PORTA;
#endif
    sample[port_b] = PORTB;
    sample[port_c] = PORTC;
    sample[port_d] = PORTD;
    sample[port_e] = PORTE;
    sample[port_f] = PORTF;
    sample[port_g] = PORTG;

    if (++pin_digital_debounce_cycle >= pin_digital_debounce_history_depth) {
        pin_digital_debounce_cycle = 0;
    }
#endif // ! STICK_GUEST
}

void
pin_assign(int assign, int pin)
{
    assert(assign < pin_assignment_max);
    assert(pin < PIN_LAST);

    pin_assignments[assign] = pin;

    if (pin < PIN_UNASSIGNED) {
        if (assign == pin_assignment_safemode) {
            pin_declare_internal(pin, pin_type_digital_input, 0, false, false);
        } else {
            pin_declare_internal(pin, pin_type_digital_output, 0, false, false);
        }
    }
}

extern void
pin_initialize(void)
{
#if ! STICK_GUEST
    int i;
    int32 hz;
    int32 pin;
    int32 analog;
#endif

    assert(PIN_LAST <= PIN_MAX);
    assert(pin_type_last < (sizeof(uint16)*8));
    assert(pin_qual_last < (sizeof(byte)*8));
    assert(LENGTHOF(pins) == PIN_LAST);

    memset(umask, -1, sizeof(umask));

#if ! STICK_GUEST
    // load up our servo frequency
    hz = var_get_flash(FLASH_SERVO);
    if (hz != (uint32)-1 && hz >= 20 && hz <= 500) {
        servo_hz = hz;
    }

    // we have to manage shared timer resources across pins
    pin_clear();

#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    // N.B. ports ABC setup in scope.c; ports EG setup in pin.c
    TRISE = 0xffff;
    ANSELE = 0x0000;
    CNPUE = ~ANSELE;  // keep inputs at low power

    TRISG = 0xffff;
    ANSELG = 0x0000;
    CNPUG = ~ANSELG;  // keep inputs at low power

    RPE14R = 6;  // OC5 -> RE14
    RPE15R = 6;  // OC6 -> RE15
    RPG9R = 5;  // OC3 -> RG9
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    RPG6R = 5;  // OC1 -> RG6
#else
#error
#endif

    // load up our pin assignments
    for (i = 0; i < pin_assignment_max; i++) {
        pin = var_get_flash(FLASH_ASSIGNMENTS_BEGIN+i);
        if (pin != (uint32)-1 && pin >= 0 && pin < PIN_LAST) {
            pin_assignments[i] = pin;
        }
        pin_assign(i, pin_assignments[i]);
    }

    // if autorun disable is asserted on boot, skip autorun
    disable_autorun = ! pin_get(pin_assignments[pin_assignment_safemode], pin_type_digital_input, 0);

    // set our analog level
    analog = var_get_flash(FLASH_ANALOG);
    if (analog != (uint32)-1 && analog >= 1000 && analog <= 5000) {
        pin_analog = analog;
    }
#endif
}

