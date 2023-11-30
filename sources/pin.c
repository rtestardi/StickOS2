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
    PIN_RE0, PIN_RE6, PIN_RE1, PIN_RE2, PIN_RE3, PIN_RE4,
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
    "an0", DIO|1<<pin_type_analog_input,  // rb0...
    "an1", DIO|1<<pin_type_analog_input,
    "an2", DIO|1<<pin_type_analog_input,
    "an3", DIO|1<<pin_type_analog_input,
    "an4", DIO|1<<pin_type_analog_input,
    "an5", DIO|1<<pin_type_analog_input,
    "an6", DIO|1<<pin_type_analog_input,
    "an7", DIO|1<<pin_type_analog_input,
    "an8", DIO|1<<pin_type_analog_input,  // U2CTS
    "an9", DIO|1<<pin_type_analog_input,
    "an10", DIO|1<<pin_type_analog_input,
    "an11", DIO|1<<pin_type_analog_input,
    "an12", DIO|1<<pin_type_analog_input,
    "an13", DIO|1<<pin_type_analog_input,
    "an14", DIO|1<<pin_type_analog_input,  // U2RTS
    "an15", DIO|1<<pin_type_analog_input,
    "rc0", 0,
    "rc1", DIO,  // rc1...
    "rc2", DIO,
    "rc3", DIO,
    "rc4", DIO,
    "rc5", 0,
    "rc6", 0,
    "rc7", 0,
    "rc8", 0,
    "rc9", 0,
    "rc10", 0,
    "rc11", 0,
    "rc12", DIO,
    "rc13", DIO,  // rc13...
    "rc14", DIO,
    "rc15", DIO,
    "rd0", DIO|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  // oc1
    "rd1", DIO|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  // oc2
    "rd2", DIO|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  // oc3
    "rd3", DIO|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  // oc4
    "rd4", DIO|1<<pin_type_analog_output|1<<pin_type_frequency_output|1<<pin_type_servo_output,  // oc5
    "rd5", DIO,
    "rd6", DIO,
    "rd7", DIO,
    "rd8", DIO,
    "rd9", DIO,
    "rd10", DIO,
    "rd11", DIO,
    "rd12", DIO,
    "rd13", DIO,
    "rd14", DIO,
    "rd15", DIO,
    "re0", DIO,
    "re1", DIO,
    "re2", DIO,
    "re3", DIO,
    "re4", DIO,
    "re5", DIO,
    "re6", DIO,
    "re7", DIO,
    "re8", DIO,
    "re9", DIO,
    "rf0", DIO,
    "rf1", DIO,
    "rf2", DIO,
    "rf3", DIO,
    "rf4", DIO|1<<pin_type_uart_input,  // u2rx
    "rf5", DIO|1<<pin_type_uart_output,  // u2tx
    "rf6", DIO,
    "rf7", 0,
    "rf8", DIO,
    "rf9", 0,
    "rf10", 0,
    "rf11", 0,
    "rf12", DIO,
    "rf13", DIO,
    "rg0", DIO,
    "rg1", DIO,
    "rg2", DIO,
    "rg3", DIO,
    "rg4", 0,
    "rg5", 0,
    "rg6", DIO,
    "rg7", DIO,
    "rg8", DIO,
    "rg9", DIO,
    "rg10", 0,
    "rg11", 0,
    "rg12", DIO,
    "rg13", DIO,
    "rg14", DIO,
    "rg15", DIO,
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
#define FREQ_PRESCALE  64
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
        case PIN_AN0:
        case PIN_AN1:
        case PIN_AN2:
        case PIN_AN3:
        case PIN_AN4:
        case PIN_AN5:
        case PIN_AN6:
        case PIN_AN7:
        case PIN_AN8:
        case PIN_AN9:
        case PIN_AN10:
        case PIN_AN11:
        case PIN_AN12:
        case PIN_AN13:
        case PIN_AN14:
        case PIN_AN15:
            offset = pin_number - PIN_AN0;
            assert(offset < 16);
            // if we have a pullup (see datasheet table 12-11)...
            if (offset < 6 || offset == 15) {
                if (pin_type == pin_type_digital_input) {
                    // turn on the pullup
                    CNPUESET = 1<<(offset<6?2+offset:12);
                } else {
                    // turn off the pullup
                    CNPUECLR = 1<<(offset<6?2+offset:12);
                }
            }
            if (pin_type == pin_type_analog_input) {
                AD1PCFGCLR = 1<<offset;
                TRISBSET = 1<<offset;
            } else {
                AD1PCFGSET = 1<<offset;
                if (pin_type == pin_type_digital_output) {
                    TRISBCLR = 1<<offset;
                } else {
                    assert(pin_type == pin_type_digital_input);
                    TRISBSET = 1<<offset;
                }
            }
            break;
        case PIN_RC1:
        case PIN_RC2:
        case PIN_RC3:
        case PIN_RC4:
        case PIN_RC13:
        case PIN_RC14:
            offset = pin_number - PIN_RC0;
            assert(offset < 16);
            // if we have a pullup (see datasheet table 12-11)...
            if (offset == 13 || offset == 14) {
                if (pin_type == pin_type_digital_input) {
                    // turn on the pullup
                    CNPUESET = 1<<(offset==13?1:0);
                } else {
                    // turn off the pullup
                    CNPUECLR = 1<<(offset==13?1:0);
                }
            }
            if (pin_type == pin_type_digital_output) {
                TRISCCLR = 1<<offset;
            } else {
                assert(pin_type == pin_type_digital_input);
                TRISCSET = 1<<offset;
            }
            break;
        case PIN_RD0:
        case PIN_RD1:
        case PIN_RD2:
        case PIN_RD3:
        case PIN_RD4:
        case PIN_RD5:
        case PIN_RD6:
        case PIN_RD7:
        case PIN_RD8:
        case PIN_RD9:
        case PIN_RD10:
        case PIN_RD11:
        case PIN_RD12:
        case PIN_RD13:
        case PIN_RD14:
        case PIN_RD15:
            offset = pin_number - PIN_RD0;
            assert(offset < 16);
            // if we have a pullup (see datasheet table 12-11)...
            if ((offset >= 4 && offset < 8) || (offset >= 13)) {
                if (pin_type == pin_type_digital_input) {
                    // turn on the pullup
                    CNPUESET = 1<<(offset>=13?19+offset-13:13+offset-4);
                } else {
                    // turn off the pullup
                    CNPUECLR = (offset>=13?19+offset-13:13+offset-4);
                }
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
                if (offset == 0) {
                    OC1CONCLR = _OC1CON_ON_MASK;
                } else if (offset == 1) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                } else if (offset == 2) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                } else if (offset == 3) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                } else if (offset == 4) {
                    OC5CONCLR = _OC5CON_ON_MASK;
                }

                if (pin_type == pin_type_digital_output) {
                    TRISDCLR = 1<<offset;
                } else {
                    assert(pin_type == pin_type_digital_input);
                    TRISDSET = 1<<offset;
                }
            }
            break;
        case PIN_RE0:
        case PIN_RE1:
        case PIN_RE2:
        case PIN_RE3:
        case PIN_RE4:
        case PIN_RE5:
        case PIN_RE6:
        case PIN_RE7:
        case PIN_RE8:
        case PIN_RE9:
            offset = pin_number - PIN_RE0;
            assert(offset < 16);
            if (pin_type == pin_type_digital_output) {
                TRISECLR = 1<<offset;
            } else {
                assert(pin_type == pin_type_digital_input);
                TRISESET = 1<<offset;
            }
            break;
        case PIN_RF0:
        case PIN_RF1:
        case PIN_RF2:
        case PIN_RF3:
        case PIN_RF4:
        case PIN_RF5:
        case PIN_RF6:
        case PIN_RF8:
        case PIN_RF12:
        case PIN_RF13:
            offset = pin_number - PIN_RF0;
            assert(offset < 16);
            // if we have a pullup (see datasheet table 12-11)...
            if (offset >= 4 && offset < 6) {
                if (pin_type == pin_type_digital_input) {
                    // turn on the pullup
                    CNPUESET = 1<<(17+offset-4);
                } else {
                    // turn off the pullup
                    CNPUECLR = (17+offset-4);
                }
            }
            if (pin_type == pin_type_uart_input || pin_type == pin_type_uart_output) {
                assert(offset == 4 || offset == 5);
                U2MODE |= _U2MODE_UARTEN_MASK;
            } else {
                if (offset == 4 || offset == 5) {
                    U2MODE &= ~_U2MODE_UARTEN_MASK;
                }

                if (pin_type == pin_type_digital_output) {
                    TRISFCLR = 1<<offset;
                } else {
                    assert(pin_type == pin_type_digital_input);
                    TRISFSET = 1<<offset;
                }
            }
            break;
        case PIN_RG0:
        case PIN_RG1:
        case PIN_RG6:
        case PIN_RG7:
        case PIN_RG8:
        case PIN_RG9:
        case PIN_RG12:
        case PIN_RG13:
        case PIN_RG14:
        case PIN_RG15:
            offset = pin_number - PIN_RG0;
            assert(offset < 16);
            // if we have a pullup (see datasheet table 12-11)...
            if (offset >= 6 && offset < 10) {
                if (pin_type == pin_type_digital_input) {
                    // turn on the pullup
                    CNPUESET = 1<<(8+offset-6);
                } else {
                    // turn off the pullup
                    CNPUECLR = (8+offset-6);
                }
            }
            if (pin_type == pin_type_digital_output) {
                TRISGCLR = 1<<offset;
            } else {
                assert(pin_type == pin_type_digital_input);
                TRISGSET = 1<<offset;
            }
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
        case PIN_AN0:
        case PIN_AN1:
        case PIN_AN2:
        case PIN_AN3:
        case PIN_AN4:
        case PIN_AN5:
        case PIN_AN6:
        case PIN_AN7:
        case PIN_AN8:
        case PIN_AN9:
        case PIN_AN10:
        case PIN_AN11:
        case PIN_AN12:
        case PIN_AN13:
        case PIN_AN14:
        case PIN_AN15:
            offset = pin_number - PIN_AN0;
            assert(offset < 16);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATBSET = 1<<offset;
            } else {
                LATBCLR = 1<<offset;
            }
            break;
        case PIN_RC1:
        case PIN_RC2:
        case PIN_RC3:
        case PIN_RC4:
        case PIN_RC13:
        case PIN_RC14:
            offset = pin_number - PIN_RC0;
            assert(offset < 16);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATCSET = 1<<offset;
            } else {
                LATCCLR = 1<<offset;
            }
            break;
        case PIN_RD0:
        case PIN_RD1:
        case PIN_RD2:
        case PIN_RD3:
        case PIN_RD4:
        case PIN_RD5:
        case PIN_RD6:
        case PIN_RD7:
        case PIN_RD8:
        case PIN_RD9:
        case PIN_RD10:
        case PIN_RD11:
        case PIN_RD12:
        case PIN_RD13:
        case PIN_RD14:
        case PIN_RD15:
            offset = pin_number - PIN_RD0;
            assert(offset < 16);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output) {
                if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MOD/SERVO_MAX;
                }
                if (offset == 0) {
                    OC1CONCLR = _OC1CON_ON_MASK;
                    OC1R = 0;
                    OC1RS = value;
                    OC1CON = _OC1CON_ON_MASK|(6<<_OC1CON_OCM0_POSITION);
                } else if (offset == 1) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                    OC2R = 0;
                    OC2RS = value;
                    OC2CON = _OC2CON_ON_MASK|(6<<_OC2CON_OCM0_POSITION);
                } else if (offset == 2) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                    OC3R = 0;
                    OC3RS = value;
                    OC3CON = _OC3CON_ON_MASK|(6<<_OC3CON_OCM0_POSITION);
                } else if (offset == 3) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                    OC4R = 0;
                    OC4RS = value;
                    OC4CON = _OC4CON_ON_MASK|(6<<_OC4CON_OCM0_POSITION);
                } else {
                    assert(offset == 4);
                    OC5CONCLR = _OC5CON_ON_MASK;
                    OC5R = 0;
                    OC5RS = value;
                    OC5CON = _OC5CON_ON_MASK|(6<<_OC5CON_OCM0_POSITION);
                }
            } else if (pin_type == pin_type_frequency_output) {
                // configure timer 3 for frequency generation
                T3CONCLR = _T3CON_ON_MASK;
                TMR3 = 0;
                PR3 = value;
                if (offset == 0) {
                    OC1CONCLR = _OC1CON_ON_MASK;
                    OC1R = 0;
                    OC1RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC1CON = _OC1CON_ON_MASK|_OC1CON_OCTSEL_MASK|(3<<_OC1CON_OCM0_POSITION);
                    }
                } else if (offset == 1) {
                    OC2CONCLR = _OC2CON_ON_MASK;
                    OC2R = 0;
                    OC2RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC2CON = _OC2CON_ON_MASK|_OC2CON_OCTSEL_MASK|(3<<_OC2CON_OCM0_POSITION);
                    }
                } else if (offset == 2) {
                    OC3CONCLR = _OC3CON_ON_MASK;
                    OC3R = 0;
                    OC3RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC3CON = _OC3CON_ON_MASK|_OC3CON_OCTSEL_MASK|(3<<_OC3CON_OCM0_POSITION);
                    }
                } else if (offset == 3) {
                    OC4CONCLR = _OC4CON_ON_MASK;
                    OC4R = 0;
                    OC4RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC4CON = _OC4CON_ON_MASK|_OC4CON_OCTSEL_MASK|(3<<_OC4CON_OCM0_POSITION);
                    }
                } else {
                    assert(offset == 4);
                    OC5CONCLR = _OC5CON_ON_MASK;
                    OC5R = 0;
                    OC5RS = value;
                    // hack to work around high minimum frequency
                    if (value) {
                        OC5CON = _OC5CON_ON_MASK|_OC5CON_OCTSEL_MASK|(3<<_OC5CON_OCM0_POSITION);
                    }
                }
                // set timer prescale to 64
                T3CON = _T3CON_ON_MASK|(6<<_T3CON_TCKPS0_POSITION);
                assert(FREQ_PRESCALE == 64);
            } else {
                assert(pin_type == pin_type_digital_output);
                if (value) {
                    LATDSET = 1<<offset;
                } else {
                    LATDCLR = 1 << offset;
                }
            }
            break;
        case PIN_RE0:
        case PIN_RE1:
        case PIN_RE2:
        case PIN_RE3:
        case PIN_RE4:
        case PIN_RE5:
        case PIN_RE6:
        case PIN_RE7:
        case PIN_RE8:
        case PIN_RE9:
            offset = pin_number - PIN_RE0;
            assert(offset < 16);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATESET = 1<<offset;
            } else {
                LATECLR = 1<<offset;
            }
            break;
        case PIN_RF0:
        case PIN_RF1:
        case PIN_RF2:
        case PIN_RF3:
        case PIN_RF4:
        case PIN_RF5:
        case PIN_RF6:
        case PIN_RF8:
        case PIN_RF12:
        case PIN_RF13:
            offset = pin_number - PIN_RF0;
            assert(offset < 16);
            if (pin_type == pin_type_uart_output) {
                assert(pin_number == PIN_RF5);
                pin_uart_tx(1, value);
            } else {
                assert(pin_type == pin_type_digital_output);
                if (value) {
                    LATFSET = 1<<offset;
                } else {
                    LATFCLR = 1<<offset;
                }
            }
            break;
        case PIN_RG0:
        case PIN_RG1:
        case PIN_RG6:
        case PIN_RG7:
        case PIN_RG8:
        case PIN_RG9:
        case PIN_RG12:
        case PIN_RG13:
        case PIN_RG14:
        case PIN_RG15:
            offset = pin_number - PIN_RG0;
            assert(offset < 16);
            assert(pin_type == pin_type_digital_output);
            if (value) {
                LATGSET = 1<<offset;
            } else {
                LATGCLR = 1<<offset;
            }
            break;
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
        case PIN_AN0:
        case PIN_AN1:
        case PIN_AN2:
        case PIN_AN3:
        case PIN_AN4:
        case PIN_AN5:
        case PIN_AN6:
        case PIN_AN7:
        case PIN_AN8:
        case PIN_AN9:
        case PIN_AN10:
        case PIN_AN11:
        case PIN_AN12:
        case PIN_AN13:
        case PIN_AN14:
        case PIN_AN15:
            offset = pin_number - PIN_AN0;
            assert(offset < 16);
            if (pin_type == pin_type_analog_input) {
                value = adc_get_value(offset, pin_qual);
            } else if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_b, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = !! (PORTB & 1 << offset);
            }
            break;
        case PIN_RC1:
        case PIN_RC2:
        case PIN_RC3:
        case PIN_RC4:
        case PIN_RC13:
        case PIN_RC14:
            offset = pin_number - PIN_RC0;
            assert(offset < 16);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_c, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = (PORTC & 1 << offset);
            }
            break;
        case PIN_RD0:
        case PIN_RD1:
        case PIN_RD2:
        case PIN_RD3:
        case PIN_RD4:
        case PIN_RD5:
        case PIN_RD6:
        case PIN_RD7:
        case PIN_RD8:
        case PIN_RD9:
        case PIN_RD10:
        case PIN_RD11:
        case PIN_RD12:
        case PIN_RD13:
        case PIN_RD14:
        case PIN_RD15:
            offset = pin_number - PIN_RD0;
            assert(offset < 16);
            if (pin_type == pin_type_analog_output || pin_type == pin_type_servo_output || pin_type == pin_type_frequency_output) {
                // XXX -- servo pins get 2 less than set
                if (offset == 0) {
                    value = OC1RS + ((pin_type == pin_type_frequency_output && (OC1CON & _OC1CON_ON_MASK) == 0)?1000000000:0);
                } else if (offset == 1) {
                    value = OC2RS + ((pin_type == pin_type_frequency_output && (OC2CON & _OC2CON_ON_MASK) == 0)?1000000000:0);
                } else if (offset == 2) {
                    value = OC3RS + ((pin_type == pin_type_frequency_output && (OC3CON & _OC3CON_ON_MASK) == 0)?1000000000:0);
                } else if (offset == 3) {
                    value = OC4RS + ((pin_type == pin_type_frequency_output && (OC4CON & _OC4CON_ON_MASK) == 0)?1000000000:0);
                } else {
                    assert(offset == 4);
                    value = OC5RS + ((pin_type == pin_type_frequency_output && (OC5CON & _OC5CON_ON_MASK) == 0)?1000000000:0);
                }
                if (pin_type == pin_type_frequency_output) {
                    if (! value) {
                        value = 0x10000;
                    }
                    value = bus_frequency/FREQ_PRESCALE/(value+1)/2;
                } else if (pin_type == pin_type_servo_output) {
                    value = value*SERVO_MAX/SERVO_MOD;
                }
            } else if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_d, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = (PORTD & 1<<offset);
            }
            break;
        case PIN_RE0:
        case PIN_RE1:
        case PIN_RE2:
        case PIN_RE3:
        case PIN_RE4:
        case PIN_RE5:
        case PIN_RE6:
        case PIN_RE7:
        case PIN_RE8:
        case PIN_RE9:
            offset = pin_number - PIN_RE0;
            assert(offset < 16);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_e, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = (PORTE & 1 << offset);
            }
            break;
        case PIN_RF0:
        case PIN_RF1:
        case PIN_RF2:
        case PIN_RF3:
        case PIN_RF4:
        case PIN_RF5:
        case PIN_RF6:
        case PIN_RF8:
        case PIN_RF12:
        case PIN_RF13:
            offset = pin_number - PIN_RF0;
            assert(offset < 16);
            if (pin_type == pin_type_uart_input) {
                assert(offset == 4);
                value = pin_uart_rx(1);
            } else if (pin_type == pin_type_uart_output) {
                assert(offset == 5);
                value = pin_uart_tx_empty(1)?0:ulasttx[1];
            } else if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_f, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = (PORTF & 1 << offset);
            }
            break;
        case PIN_RG0:
        case PIN_RG1:
        case PIN_RG6:
        case PIN_RG7:
        case PIN_RG8:
        case PIN_RG9:
        case PIN_RG12:
        case PIN_RG13:
        case PIN_RG14:
        case PIN_RG15:
            offset = pin_number - PIN_RG0;
            assert(offset < 16);
            if (pin_qual & 1<<pin_qual_debounced) {
                assert(pin_type == pin_type_digital_input);
                value = pin_get_digital_debounced(port_g, offset);
            } else {
                assert(pin_type == pin_type_digital_input || pin_type == pin_type_digital_output);
                value = (PORTG & 1 << offset);
            }
            break;
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
    //CNPUE = ~ANSELE;  // keep inputs at low power  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???

    TRISG = 0xffff;
    ANSELG = 0x0000;
    //CNPUG = ~ANSELG;  // keep inputs at low power  // XXX -- WHY DOES THIS BREAK TOASTER ANALOG INPUT???

    RPE14R = 6;  // OC5 -> RE14
    RPE15R = 6;  // OC6 -> RE15
    RPG9R = 5;  // OC3 -> RG9
    RPG8R = 5;  // OC4 -> RG8
    RPG7R = 5;  // OC2 -> RG7
    RPG6R = 5;  // OC1 -> RG6
#else
    // enable all pullups
    CNPUE = 0x3fffff;
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

