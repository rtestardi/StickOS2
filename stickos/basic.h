// *** basic.h ********************************************************

// bytecodes
enum bytecode {
    code_deleted = 0x01,  // used to indicate a deleted line in ram
    code_rem,
      code_norem,
    code_on,
    code_off,
    code_mask,
    code_unmask,
    code_configure,
    code_assert,
    code_read,
      code_data,
      code_label,
      code_restore,
    code_dim,
      code_comma,  // used for dim, print, and others.
      code_ram,  // page_offset in RAM_VARIABLE_PAGE (allocated internally)
      code_flash,  // page_offset in FLASH_PARAM_PAGE (allocated internally)
      code_pin,  // pin_number, pin_type
      code_nodeid,
      code_var_reference, // for sub reference parameters
      code_absolute, // for absolute variables
    code_let,
      code_nolet,
    code_input,
    code_vprint,
    code_print,
      code_hex,
      code_dec,
      code_raw,
      code_string,
      code_expression,
      code_semi,
    code_timer,
    code_uart,
    code_qspi,
    code_i2c,
    code_watch,
      code_device_start,
      code_device_stop,
      code_device_read,
      code_device_write,
    code_if,
      code_elseif,
      code_else,
      code_endif,
    code_while,
      code_break,
      code_continue,
      code_endwhile,  // N.B. for loops translate into while/endwhile
    code_for,
      code_next,
    code_do,
      code_until,
    code_gosub,
      code_sub,
      code_return,
      code_endsub,
    code_sleep,
    code_halt,
    code_stop,
    code_end,
    
    // expressions
    code_load_and_push_immediate,  // integer
    code_load_and_push_immediate_hex,  // hex integer
    code_load_and_push_immediate_ascii,  // ascii integer
    code_load_and_push_var,  // variable name, '\0'
    code_load_and_push_var_length,  // variable name, '\0'
    code_load_and_push_var_indexed, // index on stack; variable name, '\0'
    code_logical_not, code_bitwise_not,
    code_unary_plus, code_unary_minus,
    code_add, code_subtract, code_multiply, code_divide, code_mod,
    code_shift_right, code_shift_left,
    code_bitwise_and, code_bitwise_or, code_bitwise_xor,
    code_logical_and, code_logical_or, code_logical_xor,
    code_greater, code_less, code_equal,
    code_greater_or_equal, code_less_or_equal, code_not_equal,
    code_contains, code_not_contains,

    // strings
    code_text,  // literal, '\0'
    code_load_string,  // name, '\0'
    code_load_string_indexed,  // length, expr, length, expr, name, '\0'

    code_max
};

enum timer_unit_type {
    timer_unit_usecs,
    timer_unit_msecs,
    timer_unit_secs,
    timer_unit_max
};

extern struct timer_unit {
    char *name;
    int scale;  // negative -> less than tick; positive -> tick or greater
} const timer_units[/*timer_unit_max*/];

#define BASIC_BYTECODE_SIZE  (4*BASIC_OUTPUT_LINE_SIZE)

#if ! STICK_GUEST
#define START_DYNAMIC  (FLASH_START+FLASH_BYTES - ((2+BASIC_STORES)*BASIC_LARGE_PAGE_SIZE+3*BASIC_SMALL_PAGE_SIZE))
#define FLASH_CODE1_PAGE     (byte *)(START_DYNAMIC)
#define FLASH_CODE2_PAGE     (byte *)(START_DYNAMIC+BASIC_LARGE_PAGE_SIZE)
#define FLASH_STORE_PAGE(x)  (byte *)(START_DYNAMIC+((2+(x))*BASIC_LARGE_PAGE_SIZE))
#define FLASH_CATALOG_PAGE   (byte *)(START_DYNAMIC+(2+BASIC_STORES)*BASIC_LARGE_PAGE_SIZE)
#define FLASH_PARAM1_PAGE    (byte *)(START_DYNAMIC+(2+BASIC_STORES)*BASIC_LARGE_PAGE_SIZE+BASIC_SMALL_PAGE_SIZE)
#define FLASH_PARAM2_PAGE    (byte *)(START_DYNAMIC+(2+BASIC_STORES)*BASIC_LARGE_PAGE_SIZE+2*BASIC_SMALL_PAGE_SIZE)
#else
extern byte FLASH_CODE1_PAGE[BASIC_LARGE_PAGE_SIZE];
extern byte FLASH_CODE2_PAGE[BASIC_LARGE_PAGE_SIZE];
extern byte FLASH_STORE_PAGES[BASIC_STORES][BASIC_LARGE_PAGE_SIZE];
#define FLASH_STORE_PAGE(x)  FLASH_STORE_PAGES[x]
extern byte FLASH_CATALOG_PAGE[BASIC_SMALL_PAGE_SIZE];
extern byte FLASH_PARAM1_PAGE[BASIC_SMALL_PAGE_SIZE];
extern byte FLASH_PARAM2_PAGE[BASIC_SMALL_PAGE_SIZE];
#endif

extern byte RAM_CODE_PAGE[BASIC_RAM_PAGE_SIZE];
extern byte RAM_VARIABLE_PAGE[BASIC_RAM_PAGE_SIZE];

extern byte *start_of_dynamic;
extern byte *end_of_dynamic;

bool basic_const(IN OUT char **text, OUT int *value_out);

void basic_run(char *line);

void basic_initialize(void);

