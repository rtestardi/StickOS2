// *** pin.h **********************************************************

extern int servo_hz;

#define MAX_UARTS  2

#define UART_INTS  (2*MAX_UARTS)

#define UART_INT(uart, output)  ((uart)*2+output)

// assigned pins

enum pin_assignment {  // XXX -- should we allow skeleton to extend this?
    pin_assignment_heartbeat,
    pin_assignment_safemode,
    pin_assignment_qspi_cs,  // zigflea
    pin_assignment_zigflea_rst,
    pin_assignment_zigflea_attn,
    pin_assignment_zigflea_rxtxen,
    pin_assignment_max
};

extern const char * const pin_assignment_names[];

extern byte pin_assignments[pin_assignment_max];
    

// up to 16 bits
enum pin_type {  // XXX -- should we allow skeleton to extend this?
    pin_type_digital_input,
    pin_type_digital_output,
    pin_type_analog_input,
    pin_type_analog_output,
    pin_type_uart_input,
    pin_type_uart_output,
    pin_type_frequency_output,
    pin_type_servo_output,
    pin_type_last
};

extern const char * const pin_type_names[];

// up to 8 bits.  keep in-sync with pin_qual_names.
enum pin_qual {  // XXX -- should we allow skeleton to extend this?
    pin_qual_debounced,
    pin_qual_inverted,
    pin_qual_open_drain,
    pin_qual_last
};

extern const byte pin_qual_mask[];

extern const char * const pin_qual_names[];

// N.B. pins marked with *** may affect zigflea or other system operation
enum pin_number {  // skeleton may extend this
#if defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
    PIN_A0,
    PIN_A1,
    PIN_A2,
    PIN_A3,
    PIN_A4,
    PIN_A5,
    PIN_A6,
    PIN_A7,
    PIN_A8,
    PIN_B0,
    PIN_B1,
    PIN_B2,
    PIN_B3,
    PIN_B4,
    PIN_B5,
    PIN_B6,
    PIN_B7,
    PIN_B8,
    PIN_E1,
    PIN_E2,
    PIN_E3,
    PIN_S1,
#else
    PIN_AN0,  // rb0...
    PIN_AN1,
    PIN_AN2,
    PIN_AN3,
    PIN_AN4,
    PIN_AN5,
    PIN_AN6,
    PIN_AN7,
    PIN_AN8,
    PIN_AN9,
    PIN_AN10,
    PIN_AN11,
    PIN_AN12,
    PIN_AN13,
    PIN_AN14,
    PIN_AN15,
    PIN_RC0,  // unused
    PIN_RC1,
    PIN_RC2,
    PIN_RC3,
    PIN_RC4,
    PIN_RC5,  // unused
    PIN_RC6,  // unused
    PIN_RC7,  // unused
    PIN_RC8,  // unused
    PIN_RC9,  // unused
    PIN_RC10,  // unused
    PIN_RC11,  // unused
    PIN_RC12,  // unused
    PIN_RC13,
    PIN_RC14,
    PIN_RC15,
    PIN_RD0,  // oc1
    PIN_RD1,  // oc2
    PIN_RD2,  // oc3
    PIN_RD3,  // oc4
    PIN_RD4,  // oc5
    PIN_RD5,
    PIN_RD6,
    PIN_RD7,
    PIN_RD8,
    PIN_RD9,
    PIN_RD10,
    PIN_RD11,
    PIN_RD12,
    PIN_RD13,
    PIN_RD14,
    PIN_RD15,
    PIN_RE0,
    PIN_RE1,
    PIN_RE2,
    PIN_RE3,
    PIN_RE4,
    PIN_RE5,
    PIN_RE6,
    PIN_RE7,
    PIN_RE8,
    PIN_RE9,
    PIN_RF0,
    PIN_RF1,
    PIN_RF2,
    PIN_RF3,
    PIN_RF4,
    PIN_RF5,
    PIN_RF6,
    PIN_RF7,  // unused
    PIN_RF8,
    PIN_RF9,  // unused
    PIN_RF10,  // unused
    PIN_RF11,  // unused
    PIN_RF12,
    PIN_RF13,
    PIN_RG0,
    PIN_RG1,
    PIN_RG2,  // unused
    PIN_RG3,  // unused
    PIN_RG4,  // unused
    PIN_RG5,  // unused
    PIN_RG6,
    PIN_RG7,
    PIN_RG8,
    PIN_RG9,
    PIN_RG10,  // unused
    PIN_RG11,  // unused
    PIN_RG12,
    PIN_RG13,
    PIN_RG14,
    PIN_RG15,
#endif
    PIN_UNASSIGNED,
    PIN_LAST
};

#define PIN_MAX  255  // do not change; PIN_MAX/PIN_LAST may not exceed 255!!!

extern int pin_last;

extern int32 pin_analog;

const extern struct pin {
    char *name;
    uint16 pin_type_mask;
} pins[];  // indexed by pin_number

extern const char * const uart_names[MAX_UARTS];

extern bool uart_armed[UART_INTS];


// this function declares a pin variable!
void
pin_declare(IN int pin_number, IN int pin_type, IN int pin_qual);

// this function sets a pin variable!
void
pin_set(IN int pin_number, IN int pin_type, IN int pin_qual, IN int32 value);

// this function gets a pin variable!
int32
pin_get(IN int pin_number, IN int pin_type, IN int pin_qual);


void
pin_uart_configure(int uart, int baud, int data, byte parity, bool loopback);

bool
pin_uart_tx_ready(int uart);

bool
pin_uart_tx_empty(int uart);

bool
pin_uart_rx_ready(int uart);

void
pin_uart_tx(int uart, byte value);

byte
pin_uart_rx(int uart);


void
pin_clear(void);

void
pin_timer_poll(void);

void
pin_assign(int assign, int pin);

extern void
pin_initialize(void);

