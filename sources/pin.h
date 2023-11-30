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
#error
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

