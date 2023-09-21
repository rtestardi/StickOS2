// *** run.c **********************************************************
// this file implements the bytecode execution engine for stickos,
// including the interrupt service module that invokes BASIC interrupt
// handlers.  it also implements the core of the interactive debugger,
// coupled with the command interpreter and variable access module.

// Copyright (c) CPUStick.com, 2008-2023.  All rights reserved.
// Patent U.S. 8,117,587.

#include "main.h"

bool run_step;
bool run_trace;

bool run_isr;

int run_line_number;
bool run_in_library;

int data_line_number[2];  // [run_in_library]
int data_line_offset[2];  // [run_in_library]

bool run_watchpoint;
bool run_condition = true;  // global condition flag

bool run_printf;

static int32 run_sleep_ticks;
static int run_sleep_line_number;
static int32 run_isr_sleep_ticks;
static int run_isr_sleep_line_number;

// N.B. we assume UART_INTS start at 0!

#define MAX_INTS  (UART_INTS+TIMER_INTS+num_watchpoints)

#define UART_MASK  ((1<<UART_INTS)-1)

#define TIMER_INT(timer)  (UART_INTS+timer)

#define WATCH_INT(watch)  (UART_INTS+TIMER_INTS+(watch))
#define WATCH_MASK  (all_watchpoints_mask<<(UART_INTS+TIMER_INTS))

static uint32 run_isr_enabled;  // bitmask
static uint32 run_isr_pending;  // bitmask
static uint32 run_isr_masked;  // bitmask

static const byte *run_isr_bytecode[MAX_INTS];
static int run_isr_length[MAX_INTS];

uint32 possible_watchpoints_mask; // bitmask: bit is set if respective watchpoint expression might be true.
static uint32 watchpoints_armed_mask;
bool watch_mode_smart;

static struct {
    int length;
    const byte *bytecode;
} watchpoints[num_watchpoints];

#if ! STICK_GUEST
// XXX -- use big_buffer???
static char run_buffer1[BASIC_OUTPUT_LINE_SIZE+2];  // 2 for \r\n
static char run_buffer2[BASIC_OUTPUT_LINE_SIZE+2];  // 2 for \r\n
#else
static char run_buffer1[BASIC_OUTPUT_LINE_SIZE+1];  // 1 for \n
static char run_buffer2[BASIC_OUTPUT_LINE_SIZE+1];  // 1 for \n
#endif

static struct {
    int32 interval_ticks; // ticks/interrupt
    int32 last_ticks;
} timers[MAX_TIMERS];

volatile bool run_stop;
volatile bool running;  // for profiler
volatile int run_breaks;

// these are the conditional scopes

enum scope_type {
    open_none,
    open_if,
    open_while,
    open_isr
};

#define MAX_SCOPES  20

static
struct open_scope {
    enum scope_type type;
    int line_number;

    // revisit -- this is a waste just for for!
    const char *for_variable_name;
    int for_variable_index;
    int32 for_final_value;
    int32 for_step_value;

    bool condition;
    bool condition_ever;
    bool condition_initial;
    bool condition_restore;
} scopes[MAX_SCOPES];

static int cur_scopes;

// these are the gosub/return stack

#define MAX_GOSUBS  10

static
struct running_gosub {
    int return_line_number;
    bool return_in_library;
    int return_var_scope;
    int return_scope;
} gosubs[MAX_GOSUBS];

static int max_gosubs;

// these are the parameter stack

#define MAX_STACK  10

static int32 stack[MAX_STACK];

static int max_stack;

static
void
clear_stack(void)
{
    max_stack = 0;
}

static
void
push_stack(int32 value)
{
    if (! run_condition) {
        return;
    }
    if (max_stack < MAX_STACK) {
        stack[max_stack] = value;
    } else if (max_stack == MAX_STACK) {
        printf("stack overflow\n");
        stop();
    }
    max_stack++;
}

static
int32
pop_stack()
{
    if (! run_condition) {
        return 0;
    }
    assert(max_stack);
    --max_stack;
    if (max_stack < MAX_STACK) {
        return stack[max_stack];
    } else {
        return 0;
    }
}

// this function reads the next piece of data from the program.
static
int32
read_data()
{
    int32 value;
    struct line *line;
    int line_number;

    if (! run_condition) {
        return 0;
    }

    line_number = data_line_number[run_in_library] ? data_line_number[run_in_library]-1 : 0;
    for (;;) {
        // find the next line to check
        line = code_next_line(false, run_in_library, &line_number);
        if (! line) {
            printf("out of data\n");
            stop();
            return 0;
        }

        // if it is not a data line...
        if (line->bytecode[0] != code_data) {
            // skip it
            continue;
        }

        data_line_number[run_in_library] = line->line_number;

        // if we have data left in the line...
        if (line->length > (int)(1+data_line_offset[run_in_library]*(sizeof(uint32)+1))) {
            value = read32(line->bytecode+1+data_line_offset[run_in_library]*(sizeof(uint32)+1)+1);
            data_line_offset[run_in_library]++;
            return value;
        }

        // on to the next line
        data_line_offset[run_in_library] = 0;
    }
}

int32
run_bytecode_const(IN const byte *bytecode, IN OUT int *index)
{
    byte code;
    int32 value;

    // skip the format
    code = bytecode[*index];
    assert(code == code_load_and_push_immediate || code == code_load_and_push_immediate_hex || code == code_load_and_push_immediate_ascii);
    (*index)++;

    // read the const
    value = read32(bytecode + *index);
    (*index) += sizeof(int32);

    return value;
}

// revisit -- merge this with run.c/basic.c/parse.c???
bool
run_input_const(IN OUT char **text, OUT int32 *value_out)
{
    char c;
    int32 value;

    if ((*text)[0] && ! isdigit((*text)[0])) {
        return false;
    }

    // parse constant value and advance *text past constant
    value = 0;
    if ((*text)[0] == '0' && (*text)[1] == 'x') {
        (*text) += 2;
        for (;;) {
            c = (*text)[0];
            if (c >= 'a' && c <= 'f') {
                value = value*16 + 10 + c - 'a';
                (*text)++;
            } else if (isdigit(c)) {
                value = value*16 + c - '0';
                (*text)++;
            } else {
                break;
            }
        }
    } else {
        while (isdigit((*text)[0])) {
            value = value*10 + (*text)[0] - '0';
            (*text)++;
        }
    }

    *value_out = value;
    parse_trim(text);
    return true;
}

int
run_var(const byte *bytecode_in, int length, OUT const char **name, OUT int32 *max_index)
{
    int blen;
    const byte *bytecode;

    bytecode = bytecode_in;

    // if we're reading into a simple variable...
    if (*bytecode == code_load_and_push_var || *bytecode == code_load_string) {
        // use array index 0
        bytecode++;
        *max_index = 0;
    } else {
        // we're reading into an array element
        assert(*bytecode == code_load_and_push_var_indexed || *bytecode == code_load_string_indexed);
        bytecode++;

        blen = read32(bytecode);
        bytecode += sizeof(uint32);

        // evaluate the array index
        bytecode += run_expression(bytecode, blen, NULL, max_index);
    }

    // get the variable name
    *name = (char *)bytecode;
    bytecode += strlen(*name)+1;

    return bytecode-bytecode_in;
}

static bool
run_expression_is_lvalue(const byte *bytecode, int length, const byte *bytecode_in)
{
    int var_len;

    var_len = strlen((const char *)bytecode)+1;

    // if there's no more bytecode, then this is an lvalue
    if (bytecode+var_len >= bytecode_in+length) {
        return true;
    }

    // if the expression is about to end due to a code_comma, then this is an lvalue.
    if (bytecode[var_len] == code_comma) {
        return true;
    }

    return false;
}

// this function evaluates a bytecode expression as an rvalue if
// lvalue_var_name is NULL.
//
// If lvalue_var_name != NULL, then it attempts to evaluate bytecode
// expression as a simple (non array index) lvalue.
// - if an lvalue if found:
//   - return the name of the var in *lvalue_var_name.
// - in a non-lvalue is found:
//   - set *lvalue_var_name=NULL
//   - set *value to the expression value
static int
run_expression_watchpoint(const byte *bytecode_in, int length, IN uint32 running_watchpoint_mask, IN OUT const char **lvalue_var_name, OUT int32 *value)
{
    int32 lhs;
    int32 rhs;
    uint code;
    int32 push;
    const byte *bytecode;

    bytecode = bytecode_in;
    // assume a non-lvalue.
    if (lvalue_var_name != NULL) {
        *lvalue_var_name = NULL;
    }

    clear_stack();

    while (bytecode < bytecode_in+length && *bytecode != code_comma) {
        code = *bytecode++;
        switch (code) {
            case code_add:
            case code_subtract:
            case code_multiply:
            case code_divide:
            case code_mod:
            case code_shift_right:
            case code_shift_left:
            case code_bitwise_and:
            case code_bitwise_or:
            case code_bitwise_xor:
            case code_logical_and:
            case code_logical_or:
            case code_logical_xor:
            case code_greater:
            case code_less:
            case code_equal:
            case code_greater_or_equal:
            case code_less_or_equal:
            case code_not_equal:
                // get our sides in a deterministic order!
                rhs = pop_stack();
                lhs = pop_stack();
        }

        switch (code) {
            case code_load_and_push_immediate:
            case code_load_and_push_immediate_hex:
            case code_load_and_push_immediate_ascii:
                push = read32(bytecode);
                bytecode += sizeof(uint32);
                break;

            case code_load_and_push_var:  // variable name, '\0'
                if (lvalue_var_name && run_expression_is_lvalue(bytecode, length, bytecode_in)) {
                    // lvalue found, return reference to var name.
                    push = -1; // var index - give pop_stack() at end of this routine something to consume.
                    *lvalue_var_name = (const char *)bytecode;
                } else {
                    // do not want lvalue, or non-lvalue found, evaluate.
                    push = var_get((char *)bytecode, 0, running_watchpoint_mask);
                }
                bytecode += strlen((char *)bytecode)+1;
                break;

            case code_load_and_push_var_length:  // variable name, '\0'
                push = var_get_length((char *)bytecode);
                bytecode += strlen((char *)bytecode)+1;
                break;

            case code_load_and_push_var_indexed:  // index on stack; variable name, '\0'
                push = var_get((char *)bytecode, pop_stack(), running_watchpoint_mask);
                bytecode += strlen((char *)bytecode)+1;
                break;

            case code_logical_not:
                push = ! pop_stack();
                break;

            case code_bitwise_not:
                push = ~ pop_stack();
                break;

            case code_unary_plus:
                push = pop_stack();
                break;

            case code_unary_minus:
                push = -pop_stack();
                break;

            case code_add:
                push = lhs + rhs;
                break;

            case code_subtract:
                push = lhs - rhs;
                break;

            case code_multiply:
                push = lhs * rhs;
                break;

            case code_divide:
                if (! rhs) {
                    if (run_condition) {
                        printf("divide by 0\n");
                        stop();
                    }
                    push = 0;
                } else {
                    push = lhs / rhs;
                }
                break;

            case code_mod:
                if (! rhs) {
                    if (run_condition) {
                        printf("divide by 0\n");
                        stop();
                    }
                    push = 0;
                } else {
                    push = lhs % rhs;
                }
                break;

            case code_shift_right:
                push = lhs >> rhs;
                break;

            case code_shift_left:
                push = lhs << rhs;
                break;

            case code_bitwise_and:
                push = lhs & rhs;
                break;

            case code_bitwise_or:
                push = lhs | rhs;
                break;

            case code_bitwise_xor:
                push = lhs ^ rhs;
                break;

            case code_logical_and:
                push = lhs && rhs;
                break;

            case code_logical_or:
                push = lhs || rhs;
                break;

            case code_logical_xor:
                push = !!lhs != !!rhs;
                break;

            case code_greater:
                push = lhs > rhs;
                break;

            case code_less:
                push = lhs < rhs;
                break;

            case code_equal:
                push = lhs == rhs;
                break;

            case code_greater_or_equal:
                push = lhs >= rhs;
                break;

            case code_less_or_equal:
                push = lhs <= rhs;
                break;

            case code_not_equal:
                push = lhs != rhs;
                break;

            default:
                push = 0;
                assert(0);
                break;
        }

        push_stack(push);
    }

    *value = pop_stack();
    assert(! max_stack);
    return bytecode - bytecode_in;
}

int
run_expression(const byte *bytecode_in, int length, IN OUT const char **lvalue_var_name, OUT int32 *value)
{
    return run_expression_watchpoint(bytecode_in, length, 0, lvalue_var_name, value);
}

// read the next value from main_command
static
int32
subinput(
    int format,
    int size
    )
{
    bool boo;
    int32 value;

    if (format != code_raw) {
        parse_trim((char **)&main_command);
    }

    value = 0;

    if (format == code_dec || format == code_hex) {
        boo = run_input_const((char **)&main_command, &value);
        if (! boo) {
            printf("bad number\n");
            stop();
        }
    } else {
        assert(format == code_raw);

        switch (size) {
            case 4:
                value = value << 8 | *main_command;
                if (*main_command) {
                    main_command++;
                }
                value = value << 8 | *main_command;
                if (*main_command) {
                    main_command++;
                }
            case 2:
                value = value << 8 | *main_command;
                if (*main_command) {
                    main_command++;
                }
            case 1:
                value = value << 8 | *main_command;
                if (*main_command) {
                    main_command++;
                }
                break;
            default:
                assert(0);
        }
    }

    return value;
}

// print the next value
static
int
subprint(
    char *s,
    int n,
    int32 value,
    int format,
    int size  // only used for code_raw
    )
{
    int i;

    if (format == code_hex) {
        i = snprintf(s, n, "0x%lx", value);
    } else if (format == code_dec) {
        i = snprintf(s, n, "%ld", value);
    } else {
        assert(format == code_raw);
        if (size == 1) {
            i = snprintf(s, n, "%c", (int)value);
        } else if (size == 2) {
            i = snprintf(s, n, "%c%c", (int)(value>>8), (int)value);
        } else {
            assert(size == 4);
            i = snprintf(s, n, "%c%c%c%c", (int)(value>>24), (int)(value>>16), (int)(value>>8), (int)value);
        }
    }
    return i;
}

// print an array of values
static
int
subprintarray(
    char *s,
    int n,
    const char *name,
    int start,
    int length,
    int format,
    bool string
    )
{

    int i;
    int j;
    int size;
    int count;
    int32 value;

    i = 0;
    size = var_get_size(name, &count);
    if (count) {
        for (j = 0; j < count; j++) {
            if (j && format != code_raw) {
                i += snprintf(s+i, n-i, " ");
            }
            value = var_get(name, j, 0);
            if (string && ! value) {
                break;
            }
            if (j >= start && j < start+length) {
                i += subprint(s+i, n-i, value, format, size);
            }
        }
    }
    return i;
}

int
run_string(IN const byte *bytecode_in, IN int length, IN int size, OUT char *string, OUT int *actual_out)
{
    int len;
    int blen;
    int blen2;
    byte code;
    int actual;
    int32 value1;
    int32 value2;
    const char *name;
    const byte *bytecode;

    actual = 0;
    bytecode = bytecode_in;
    while (bytecode < bytecode_in+length) {
        code = *bytecode;
        if (code == code_comma) {
            break;
        }
        bytecode++;

        switch (code) {
            case code_text:
                actual += snprintf(string+actual, size-actual, "%s", bytecode);
                len = strlen((const char *)bytecode);
                bytecode += len+1;
                break;
            case code_load_string:
            case code_load_string_indexed:
                if (code == code_load_string) {
                    value1 = 0;
                    value2 = 0x7fff;
                } else {
                    blen = read32(bytecode);
                    bytecode += sizeof(uint32);
                    blen2 = read32(bytecode);
                    bytecode += sizeof(uint32);
                    len = run_expression(bytecode, blen, NULL, &value1);
                    assert(len == blen);
                    bytecode += blen;
                    len = run_expression(bytecode, blen2, NULL, &value2);
                    assert(len == blen2);
                    bytecode += blen2;
                }
                name = (const char *)bytecode;
                len = strlen(name);
                bytecode += len+1;
                actual += subprintarray(string+actual, size-actual, name, value1, value2, code_raw, true);
                break;
            default:
                assert(0);
        }
    }

    *actual_out = actual;
    assert(strlen(string) < (size_t)size);
    return bytecode-bytecode_in;
}

int
run_relation(const byte *bytecode_in, int length, OUT int32 *value)
{
    int i;
    char *p;
    int len;
    byte code;
    int actual;
    const byte *bytecode;

    bytecode = bytecode_in;

    // get the left hand string
    *run_buffer1 = '\0';
    len = run_string(bytecode, length, sizeof(run_buffer1), run_buffer1, &actual);
    bytecode += len;
    length -= len;
    assert(*bytecode == code_comma);
    bytecode++;
    length--;

    // get the relation
    code = *bytecode;
    bytecode++;
    length--;

    // get the right hand string
    *run_buffer2 = '\0';
    len = run_string(bytecode, length, sizeof(run_buffer2), run_buffer2, &actual);
    bytecode += len;
    length -= len;
    assert(*bytecode == code_comma);
    bytecode++;

    // evaluate the relation
    i = strcmp(run_buffer1, run_buffer2);
    p = strstr(run_buffer1, run_buffer2);
    switch (code) {
        case code_greater:
            *value = i > 0;
            break;
        case code_less:
            *value = i < 0;
            break;
        case code_equal:
            *value = i == 0;
            break;
        case code_greater_or_equal:
            *value = i >= 0;
            break;
        case code_less_or_equal:
            *value = i <= 0;
            break;
        case code_not_equal:
            *value = i != 0;
            break;
        case code_contains:
            *value = !! p;
            break;
        case code_not_contains:
            *value = ! p;
            break;
        default:
            assert(0);
    }

    return bytecode - bytecode_in;
}

int
run_relation_or_expression(const byte *bytecode_in, int length, OUT int32 *value)
{
    const byte *bytecode;

    bytecode = bytecode_in;

    // if we're comparing strings...
    if (*bytecode == code_string) {
        bytecode++;
        length--;

        // evaluate the string relation
        bytecode += run_relation(bytecode, length, value);
    } else {
        assert(*bytecode == code_expression);
        bytecode++;
        length--;

        // evaluate the condition
        bytecode += run_expression(bytecode, length, NULL, value);
    }

    return bytecode - bytecode_in;
}

static
int
run_timer(const byte *bytecode_in, int length, OUT int32 *value)
{
    const byte *bytecode;
    enum timer_unit_type timer_unit;

    bytecode = bytecode_in;

    // evaluate the time units
    timer_unit = (enum timer_unit_type)*bytecode;
    bytecode++;

    // evaluate the sleep time
    bytecode += run_expression(bytecode, bytecode_in+length-bytecode, NULL, value);

    // scale the timer interval
    if (timer_units[timer_unit].scale < 0) {
        *value /= -timer_units[timer_unit].scale;
    } else {
        assert(timer_units[timer_unit].scale > 0);
        *value *= timer_units[timer_unit].scale;
    }

    return bytecode - bytecode_in;
}

static
void
uart_read_write(IN int uart, IN bool write, byte *buffer, int length)
{
    int breaks;

    breaks = run_breaks;
    while (length--) {
        if (write) {
            pin_uart_tx(uart, *buffer);
        } else {
            while (! pin_uart_rx_ready(uart)) {
                if (run_breaks != breaks) {
                    // XXX -- this hangs forever if statement is run in "immediate" mode at command prompt (because we have no terminal_command_ack(false)?))
                    break;
                }
            }
            *buffer = pin_uart_rx(uart);
        }
    }
}

#if STICK_GUEST
static
void
dumpbuffer(char *buffer, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        printf("  0x%x\n", (byte)buffer[i]);
    }
}
#endif

// this function executes a bytecode statement, with an independent keyword
// bytecode.
bool  // end
run_bytecode_code(uint code, bool immediate, const byte *bytecode, int length)
{
    int i;
    int j;
    int k;
    int n;
    int uart;
    int32 baud;
    int data;
    byte parity;
    byte loopback;
    byte device;
    byte code2;
    int inter;
    int32 value;
    byte *p;
    char *s;
    int index;
    int oindex;
    int pass;
    const char *name;
    const char *name2;
    int size;
    int timer;
    int32 max_index;
    int32 max_count;
    int count;
    int var_type;
    int pin_number;
    int pin_type;
    int pin_qual;
    struct line *line;
    int format;
    bool semi;
    bool output;
    bool string;
    bool simple_var;
    int isr_length;
    const byte *isr_bytecode;
    int sub_length;
    const byte *sub_bytecode;
    int watch_length;
    const byte *watch_bytecode;
    int32 abs_addr;
    int nodeid;
    struct open_scope *scope;

    assert(code >= code_deleted && code < code_max_max);

    index = 0;

    scope = scopes+cur_scopes;

    run_condition = scope->condition && scope->condition_initial;
    run_condition |= immediate;

    switch (code) {
        case code_deleted:
            // nothing to do
            break;

        case code_rem:
        case code_norem:
            index = length;  // skip the comment
            break;

        case code_on:
        case code_off:
        case code_mask:
        case code_unmask:
            // get the device
            device = bytecode[index];
            index++;

            if (device == code_timer) {
                // *** timer control ***
                // get the timer number
                timer = run_bytecode_const(bytecode, &index);
                assert(timer >= 0 && timer < MAX_TIMERS);

                inter = TIMER_INT(timer);

                // if we are enabling the interrupt...
                if (code == code_on) {
                    timers[timer].last_ticks = ticks;
                }
            } else if (device == code_uart) {
                // *** uart control ***
                // get the uart number
                uart = bytecode[index++];
                assert(uart >= 0 && uart < MAX_UARTS);

                // get the input/output flag
                output = bytecode[index];
                index++;

                assert(output == true || output == false);
                inter = UART_INT(uart, output);
            } else if (device == code_watch) {
                // this is the expression to watch
                watch_length = read32(bytecode + index);
                index += sizeof(uint32);
                watch_bytecode = bytecode+index;
                index += watch_length;

                // are other watches with the same condition?
                n = -1;
                for (i = 0; i < num_watchpoints; i++) {
                    // remember the first empty watchpoint
                    if ((watchpoints[i].length == 0) && (n == -1)) {
                        n = i;
                        continue;
                    }
                    // if the i'th watch matches the current condition, then bail
                    if ((watchpoints[i].length == watch_length) &&
                        (memcmp(watchpoints[i].bytecode, watch_bytecode, watch_length) == 0)) {
                        break;
                    }
                }

                assert(bytecode[index] == code_comma);
                index++;

                if (code == code_on) {
                    // if watch expression already being watched, then bail
                    if (i < num_watchpoints) {
                        printf("watchpoint already defined\n");
                        goto XXX_SKIP_XXX;
                    }

                    i = n;

                    // are all watchpoints in-use?
                    if (i == -1) {
                        printf("too many watchpoints\n");
                        goto XXX_SKIP_XXX;
                    }

                    watchpoints[i].length = watch_length;
                    watchpoints[i].bytecode = watch_bytecode;
                }

                inter = WATCH_INT((i == -1) ? 0 : i);
            } else {
                assert(0);
            }

            if (code == code_on) {
                // this is the handler
                isr_length = read32(bytecode + index);
                index += sizeof(uint32);
                isr_bytecode = bytecode+index;
                index += isr_length;
            }

            if (run_condition) {
                if (code == code_mask) {
                    run_isr_masked |= 1<<inter;
                } else if (code == code_unmask) {
                    run_isr_masked &= ~(1<<inter);

                    if (device == code_watch) {
                        // schedule watchpoint for evaluation.
                        possible_watchpoints_mask |= 1 << n;
                    }
                } else if (code == code_off) {
                    run_isr_enabled &= ~(1<<inter);
                    run_isr_length[inter] = 0;
                    run_isr_bytecode[inter] = NULL;
                } else {
                    assert(code == code_on);
                    run_isr_enabled |= 1<<inter;
                    run_isr_length[inter] = isr_length;
                    run_isr_bytecode[inter] = isr_bytecode;

                    if (device == code_watch) {
                        // schedule watchpoint for evaluation.
                        possible_watchpoints_mask |= 1 << n;
                    }
                }
            }
            break;

        case code_configure:
            // get the device
            device = bytecode[index];
            index++;

            if (device == code_timer) {
                // *** timer control ***
                // get the timer number
                timer = run_bytecode_const(bytecode, &index);

                // get the timer ticks
                index += run_timer(bytecode+index, length-index, &value);

                if (run_condition) {
                    // set the timer
                    timers[timer].interval_ticks = value;
                }
            } else if (device == code_uart) {
                // *** uart control ***
                // get the uart number
                uart = bytecode[index++];
                assert(uart >= 0 && uart < MAX_UARTS);

                // get the protocol and optional loopback specifier
                baud = run_bytecode_const(bytecode, &index);
                data = run_bytecode_const(bytecode, &index);
                parity = bytecode[index];
                index++;
                loopback = bytecode[index];
                index++;

#if ! STICK_GUEST
                if (run_condition) {
                    if (uart == SERIAL_UART) {
                        serial_disable();
                    }
                    pin_uart_configure(uart, baud, data, parity, loopback);
                }
#endif
            } else {
                assert(0);
            }
            break;

        case code_assert:
            // *** interactive debugger ***
            // evaluate the assertion expression
            index += run_expression(bytecode+index, length-index, NULL, &value);
            if (run_condition && ! value) {
                printf("assertion failed\n");
                stop();
            }
            break;

        case code_read:
            // while there are more variables to read to...
            do {
                // skip commas
                if (bytecode[index] == code_comma) {
                    index++;
                }

                // get the variable
                index += run_var(bytecode+index, length-index, &name, &max_index);

                // get the next data
                value = read_data();

                // assign the variable with the next data
                var_set(name, max_index, value);
            } while (index < length && bytecode[index] == code_comma);
            break;

        case code_data:
            index = length;  // skip the data
            break;

        case code_restore:
            if (run_condition) {
                if (bytecode[index]) {
                    // XXX -- we can find label in main program by mistake
                    line = code_line(code_label, bytecode+index, false, run_in_library, NULL);
                    if (line == NULL) {
                        printf("missing label: %s\n", bytecode+index);
                        goto XXX_SKIP_XXX;
                    }
                } else {
                    line = NULL;
                }
                data_line_number[run_in_library] = line ? line->line_number : 0;
                data_line_offset[run_in_library] = 0;
            }

            index += strlen((char *)bytecode+index)+1;
            break;

        case code_dim:
            // while there are more variables to dimension...
            do {
                // skip commas
                if (bytecode[index] == code_comma) {
                    index++;
                }

                // if we're dimensioning a string...
                if (bytecode[index] == code_string) {
                    string = true;
                } else {
                    assert(bytecode[index] == code_expression);
                    string = false;
                }
                index++;

                // if we're dimensioning a simple variable
                simple_var = false;
                if (bytecode[index] == code_load_and_push_var || bytecode[index] == code_load_string) {
                    // set the array length to 1
                    simple_var = true;
                }

                // get the variable
                index += run_var(bytecode+index, length-index, &name, &max_index);

                if (simple_var) {
                    max_index = 1;
                }

                // if the string is larger than run.c can handle...
                if (string && max_index > BASIC_OUTPUT_LINE_SIZE) {
                    printf("string buffer overflow\n");
                    goto XXX_SKIP_XXX;
                }

                // get the size and type specifier
                size = bytecode[index++];
                var_type = bytecode[index++];

                assert(size == sizeof(byte) || size == sizeof(short) || size == sizeof(uint32));

                // setup default var_declare() parameter values.
                pin_number = 0;
                pin_type = 0;
                pin_qual = 0;
                nodeid = 0;
                abs_addr = -1;

                // extract variable type dependent parameters
                switch (var_type) {
                    case code_ram:
                    case code_flash:
                        break;

                    case code_absolute:
                        index += run_expression(bytecode+index, length-index, NULL, &abs_addr);
                        break;

                    case code_pin:
                        // get the pin number (from the pin name) and pin type (from the pin usage)
                        pin_number = bytecode[index++];
                        pin_type = bytecode[index++];
                        pin_qual = bytecode[index++];

                        assert(pin_number >= 0 && pin_number < pin_last);

                        // treat a non-array variable element as element zero of an array.
                        if (simple_var) {
                            max_index = 0;
                        }
                        break;

                    case code_nodeid:
                        nodeid = run_bytecode_const(bytecode, &index);
                        break;

                    default:
                        assert(0);
                        break;
                }

                var_declare(name, max_gosubs, var_type, string, size, max_index, pin_number, pin_type, pin_qual, nodeid, abs_addr);
            } while (index < length && bytecode[index] == code_comma);
            break;

        case code_let:
        case code_nolet:
            // while there are more items to print...
            do {
                // skip commas
                if (bytecode[index] == code_comma) {
                    index++;
                }

                // if we're assigning a string...
                if (bytecode[index] == code_string) {
                    index++;

                    assert(bytecode[index] == code_load_string);
                    index++;

                    // get the variable name
                    name = (char *)bytecode+index;
                    index += strlen(name)+1;

                    // evaluate the assignment string
                    index += run_string(bytecode+index, length-index, sizeof(run_buffer1), run_buffer1, &k);

                    // this is how many characters we actually printed
                    k = MIN(k, sizeof(run_buffer1)-1);

                    // assign an array of values
                    size = var_get_size(name, &count);
                    for (j = 0; j < count; j++) {
                        var_set(name, j, j<k?(byte)(run_buffer1[j]):0);
                    }

                } else {
                    // we're assigning an expression
                    assert(bytecode[index] == code_expression);
                    index++;

                    // get the variable
                    index += run_var(bytecode+index, length-index, &name, &max_index);

                    // evaluate the assignment expression
                    index += run_expression(bytecode+index, length-index, NULL, &value);

                    // assign the variable with the assignment expression
                    var_set(name, max_index, value);
                }
            } while (index < length && bytecode[index] == code_comma);
            break;

        case code_input:
            if (! run_condition) {
                index = length;  // blindly skip the bytecode
                break;
            }

#if STICK_GUEST
            {
                static char text[2*BASIC_INPUT_LINE_SIZE];

                if (isatty(0) && main_prompt) {
                    write(1, "? ", 2);
                }
                if (! gets(text)) {
                    break;
                }
                text[BASIC_INPUT_LINE_SIZE-1] = '\0';
                main_command = text;
            }
#else
            // if this is a new input statement...
            if (terminal_command_discarding()) {
                // temporarily stop discarding input
                terminal_command_discard(false);
                if (main_prompt) {
                    printf("? ");
                }
            }

            // if we don't yet have input...
            if (! main_command) {
                index = length;  // blindly skip the bytecode

                // wait for input by simply executing the bytecode again
                // N.B. waits occur in the main loop so we can service interrupts
                run_line_number--;
                break;
            }

            // process the input
            printf("\n");
#endif

            format = code_dec;
            // while there are more items to input...
            do {
                // skip commas
                if (bytecode[index] == code_comma) {
                    index++;
                }

                // if we're changing format specifiers...
                if (bytecode[index] == code_hex || bytecode[index] == code_dec || bytecode[index] == code_raw) {
                    format = bytecode[index];
                    index++;
                }

                // if we're dimensioning a string...
                if (bytecode[index] == code_string) {
                    string = true;
                } else {
                    assert(bytecode[index] == code_expression);
                    string = false;
                }
                index++;

                // if we're reading into a simple variable...
                max_count = 0;
                if (bytecode[index] == code_load_and_push_var || bytecode[index] == code_load_string) {
                    max_count = -1;
                }

                // get the variable
                index += run_var(bytecode+index, length-index, &name, &max_index);

                // get the variable size
                size = var_get_size(name, &count);
                if (max_count == -1) {
                    max_count = count;
                } else {
                    assert(! max_count);
                    max_count = max_index+1;
                }

                // for all variables...
                for (i = max_index; i < max_count; i++) {
                    // get and set the variable
                    value = subinput(string?code_raw:format, size);
                    var_set(name, i, value);
                }
            } while (index < length && bytecode[index] == code_comma);

            if (*main_command) {
                printf("trailing garbage\n");
                stop();
            }

            main_command = NULL;
#if ! STICK_GUEST
            terminal_command_ack(false);

            // resume discarding input
            terminal_command_discard(true);
#endif
            break;

        case code_print:
        case code_vprint:
            s = run_buffer1;
            n = sizeof(run_buffer1);

            assert(! index);

            // if this is an vprint...
            if (code == code_vprint) {
                // if we're printing to a string...
                if (bytecode[index] == code_string) {
                    string = true;
                } else {
                    assert(bytecode[index] == code_expression);
                    string = false;
                }
                index++;

                // get the variable
                index += run_var(bytecode+index, length-index, &name2, &max_index);
            }

            // if we're not printing a newline...
            semi = false;
            if (bytecode[index] == code_semi) {
                index++;
                semi = true;
            }

            format = code_dec;
            // while there are more items to print...
            do {
                // skip commas
                if (bytecode[index] == code_comma) {
                    if (format != code_raw) {
                        i = snprintf(s, n, " ");
                        s += i;
                        n -= i;
                    }
                    index++;
                }

                // if we're changing format specifiers...
                while (bytecode[index] == code_hex || bytecode[index] == code_dec || bytecode[index] == code_raw) {
                    format = bytecode[index];
                    index++;
                }

                // if we're printing a string...
                if (bytecode[index] == code_string) {
                    index++;

                    index += run_string(bytecode+index, length-index, n, s, &i);
                    s += i;
                    n -= i;
                } else {
                    // we're printing an expression
                    assert(bytecode[index] == code_expression);
                    index++;

                    // evaluate the expression
                    index += run_expression(bytecode+index, length-index, &name, &value);
                    if (name) {
                        // print an array of values
                        i = subprintarray(s, n, name, 0, 0x7fff, format, false);
                    } else {
                        // print a single value
                        i = subprint(s, n, value, format, 4);
                    }
                    s += i;
                    n -= i;
                }
            } while (index < length && bytecode[index] == code_comma);

            // if this is a print...
            if (code == code_print) {
                // append newline if appropriate
                if (! semi) {
                    i = snprintf(s, n, "\n");
                    s += i;
                    n -= i;
                }
            }

            // this is how many characters we actually printed
            k = MIN(s - run_buffer1, sizeof(run_buffer1)-1);

            // if this is a print...
            if (code == code_print) {
                // make sure the run_buffer1 has a trailing \r\n or \n
                assert(! run_buffer1[sizeof(run_buffer1)-1]);
                if (k == sizeof(run_buffer1)-1) {
#if ! STICK_GUEST
                    strcpy(run_buffer1+sizeof(run_buffer1)-3, "\r\n");
#else
                    strcpy(run_buffer1+sizeof(run_buffer1)-2, "\n");
#endif
                }
            }

            if (run_condition) {
                // if this is a print...
                if (code == code_print) {
                    run_printf = true;
                    printf_write(run_buffer1, k);
                    run_printf = false;
                } else {
                    assert(code == code_vprint);

                    if (string) {
                        // assign an array of values
                        size = var_get_size(name2, &count);
                        for (j = 0; j < count; j++) {
                            var_set(name2, j, j<k?(byte)(run_buffer1[j]):0);
                        }
                    } else {
                        s = run_buffer1;
                        if (! run_input_const(&s, &value)) {
                            printf("bad number\n");
                            stop();
                        } else if (*s) {
                            printf("trailing garbage\n");
                            stop();
                        }
                        var_set(name2, max_index, value);
                    }
                }
            }
            break;

        case code_uart:
        case code_qspi:
        case code_i2c:
            if (! index) {
                if (code == code_uart) {
                    // get the uart number
                    uart = bytecode[index++];
                }

                code2 = bytecode[index];
                if (code2 == code_device_start) {
                    assert(code == code_i2c);
                    index++;
                    // get the address
                    index += run_expression(bytecode+index, length-index, NULL, &value);
                    if (run_condition) {
#if ! STICK_GUEST
                        i2c_start(value);
#else
                        printf("i2c start %d\n", (int)value);
#endif
                    }
                    break;
                } else if (code2 == code_device_stop) {
                    assert(code == code_i2c);
                    index++;
                    if (run_condition) {
#if ! STICK_GUEST
                        i2c_stop();
#else
                        printf("i2c stop\n");
#endif
                    }
                    break;
                } else if (code == code_i2c || code == code_uart) {
                    assert(code2 == code_device_read || code2 == code_device_write);
                    index++;
                } else {
                    assert(code == code_qspi);
                    code2 = 0;
                }
            }

            // we'll walk the variable list twice
            oindex = index;

            // N.B. on the first pass we send variables out;
            //      on the second pass we read them in

            // while there are more variables to qspi/i2c from or to...
            for (pass = 0; pass < 2; pass++) {
                p = big_buffer;

                do {
                    // skip commas
                    if (bytecode[index] == code_comma) {
                        index++;
                    }

                    // if we're reading into a simple variable...
                    max_count = 0;
                    if (bytecode[index] == code_load_and_push_var) {
                        max_count = -1;
                    }

                    // get the variable
                    index += run_var(bytecode+index, length-index, &name, &max_index);

                    // get the variable size
                    size = var_get_size(name, &count);
                    if (max_count == -1) {
                        max_count = count;
                    } else {
                        assert(! max_count);
                        max_count = max_index+1;
                    }

                    if (! pass) {
                        // on the first pass, we get the variables size and data
                        // for all variables...
                        for (i = max_index; i < max_count; i++) {
                            // get the variable data
                            value = var_get(name, i, 0);

                            // pack it into the qspi/i2c buffer
                            if (size == sizeof(byte)) {
                                *(byte *)p = value;
                                p += sizeof(byte);
                            } else if (size == sizeof(short)) {
                                write16(p, TF_BIG((uint16)value));
                                p += sizeof(short);
                            } else {
                                assert(size == sizeof(uint32));
                                write32(p, TF_BIG((uint32)value));
                                p += sizeof(uint32);
                            }

                            if (p > big_buffer+sizeof(big_buffer)-sizeof(uint32)) {
                                printf("transfer buffer overflow\n");
                                stop();
                                break;
                            }
                        }
                    } else {
                        // on the second pass, we update the variables
                        // for all variables...
                        for (i = max_index; i < max_count; i++) {
                            // unpack it from the qspi/i2c buffer
                            if (size == sizeof(byte)) {
                                value = *(byte *)p;
                                p += sizeof(byte);
                            } else if (size == sizeof(short)) {
                                value = TF_BIG(read16(p));
                                p += sizeof(short);
                            } else {
                                assert(size == sizeof(uint32));
                                value = TF_BIG(read32(p));
                                p += sizeof(uint32);
                            }

                            // set the variable
                            var_set(name, i, value);

                            if (p > big_buffer+sizeof(big_buffer)-sizeof(uint32)) {
                                printf("transfer buffer overflow\n");
                                stop();
                                break;
                            }
                        }
                    }
                } while (index < length && bytecode[index] == code_comma);

                if (! pass) {
                    // we do the real work between the first and second passes
                    if (run_condition) {
                        if (code == code_uart) {
                            if (code2 == code_device_read) {
                                // perform the uart read
#if ! STICK_GUEST
                                uart_read_write(uart, false, big_buffer, p-big_buffer);
#else
                                printf("uart %d read transfer %d bytes\n", uart, (int)(p-big_buffer));
#endif
                            } else {
                                assert(code2 == code_device_write);
                                // perform the uart write
#if ! STICK_GUEST
                                uart_read_write(uart, true, big_buffer, p-big_buffer);
#else
                                printf("uart %d write transfer:\n", uart);
                                dumpbuffer(big_buffer, p-big_buffer);
#endif
                                // N.B. no need for the next pass for a write
                                break;
                            }
                        } else if (code == code_qspi) {
                            // perform the qspi transfer
#if ! STICK_GUEST
                            qspi_transfer(false, big_buffer, p-big_buffer);
#else
                            printf("qspi transfer:\n");
                            dumpbuffer(big_buffer, p-big_buffer);
#endif
                        } else {
                            assert(code == code_i2c);
                            if (code2 == code_device_read) {
                                // perform the i2c read
#if ! STICK_GUEST
                                i2c_read_write(false, big_buffer, p-big_buffer);
#else
                                printf("i2c read transfer %d bytes\n", (int)(p-big_buffer));
#endif
                            } else {
                                assert(code2 == code_device_write);
                                // perform the i2c write
#if ! STICK_GUEST
                                i2c_read_write(true, big_buffer, p-big_buffer);
#else
                                printf("i2c write transfer:\n");
                                dumpbuffer(big_buffer, p-big_buffer);
#endif
                                // N.B. no need for the next pass for a write
                                break;
                            }
                        }
                    }

                    // now update the variables for the next pass
                    index = oindex;
                }
            }
            break;

        case code_if:
            if (cur_scopes >= MAX_SCOPES-2) {
                printf("too many scopes\n");
                goto XXX_SKIP_XXX;
            }
            // open a new conditional scope
            cur_scopes++;
            scope = scopes+cur_scopes;
            scope->line_number = run_line_number;
            scope->type = open_if;

            // evaluate the condition
            index += run_relation_or_expression(bytecode+index, length-index, &value);

            // incorporate the condition
            scope->condition = !! value;
            scope->condition_ever = !! value;
            scope->condition_initial = run_condition;
            scope->condition_restore = false;
            break;

        case code_elseif:
            if (scope->type != open_if) {
                printf("mismatched elseif\n");
                goto XXX_SKIP_XXX;
            }

            // reevaluate the condition
            run_condition = scope->condition_initial;
            index += run_relation_or_expression(bytecode+index, length-index, &value);

            // if the condition has ever been true...
            if (scope->condition_ever) {
                // elseif's remain false, regardless of the new condition
                scope->condition = false;
            } else {
                scope->condition = !! value;
                scope->condition_ever |= !! value;
            }
            break;

        case code_else:
            if (scope->type != open_if) {
                printf("mismatched else\n");
                goto XXX_SKIP_XXX;
            }

            // flip the condition
            scope->condition = ! scope->condition_ever;
            break;

        case code_endif:
            if (scope->type != open_if) {
                printf("mismatched endif\n");
                goto XXX_SKIP_XXX;
            }

            // close the conditional scope
            assert(cur_scopes);
            cur_scopes--;
            scope = scopes+cur_scopes;
            break;

        case code_while:
        case code_for:
        case code_do:
            if (cur_scopes >= MAX_SCOPES-2) {
                printf("too many scopes\n");
                goto XXX_SKIP_XXX;
            }
            // open a new conditional scope
            cur_scopes++;
            scope = scopes+cur_scopes;
            scope->line_number = run_line_number;
            scope->type = open_while;

            scope->for_variable_name = NULL;
            scope->for_variable_index = 0;
            scope->for_final_value = 0;
            scope->for_step_value = 0;

            // if this is a for loop...
            if (code == code_for) {
                // get the variable
                index += run_var(bytecode+index, length-index, &name, &max_index);

                // evaluate and set the initial value
                index += run_expression(bytecode+index, length-index, NULL, &value);
                var_set(name, max_index, value);

                assert(name);
                scope->for_variable_name = name;
                scope->for_variable_index = max_index;

                assert(bytecode[index] == code_comma);
                index++;

                // evaluate the final value
                index += run_expression(bytecode+index, length-index, NULL, &scope->for_final_value);

                // if there is a step value...
                if (index < length && bytecode[index] == code_comma) {
                    index++;

                    // evaluate the step value
                    index += run_expression(bytecode+index, length-index, NULL, &scope->for_step_value);
                } else {
                    scope->for_step_value = 1;
                }

                // set the initial condition
                if (scope->for_step_value >= 0) {
                    value = value <= scope->for_final_value;
                } else {
                    value = value >= scope->for_final_value;
                }
            } else if (code == code_while) {
                // evaluate the condition
                index += run_relation_or_expression(bytecode+index, length-index, &value);
            } else {
                assert(code == code_do);

                value = 1;
            }

            // incorporate the condition
            scope->condition = !! value;
            scope->condition_ever = !! value;
            scope->condition_initial = run_condition;
            scope->condition_restore = false;
            break;

        case code_break:
        case code_continue:
            // if the user specified a break/continue level...
            if (index < length) {
                n = run_bytecode_const(bytecode, &index);
            } else {
                n = 1;
            }
            assert(n);

            // find the outermost while loop
            for (i = cur_scopes; i > 0; i--) {
                if (scopes[i].type == open_while) {
                    if (! --n) {
                        break;
                    }
                }
            }
            if (i == 0) {
                printf("break/continue without while/for\n");
                goto XXX_SKIP_XXX;
            }

            assert(! n);
            if (run_condition) {
                // negate all conditions
                while (i <= cur_scopes) {
                    assert(scopes[i].condition_initial);
                    assert(scopes[i].condition);
                    if (code == code_break) {
                        scopes[i].condition_initial = false;
                    } else {
                        scopes[i].condition = false;
                        if (! n) {
                            // the outermost continue gets to run again
                            assert(scopes[i].type == open_while);
                            scopes[i].condition_restore = true;
                        }
                    }
                    i++;
                    n++;
                }
            }
            break;

        case code_next:
        case code_endwhile:
        case code_until:
            if (scope->type != open_while || (code == code_next && ! scope->for_variable_name) ||
                ((code == code_endwhile || code == code_until) && scope->for_variable_name)) {
                printf("mismatched endwhile/until/next\n");
                goto XXX_SKIP_XXX;
            }

            if (scope->condition_restore) {
                scope->condition = true;
                run_condition = true;
            }

            // if this is an until loop...
            if (code == code_until) {
                // evaluate the condition
                index += run_relation_or_expression(bytecode+index, length-index, &value);
            }

            if (run_condition) {
                // if this is a for loop...
                if (code == code_next) {
                    value = var_get(scope->for_variable_name, scope->for_variable_index, 0);
                    value += scope->for_step_value;

                    // if the stepped value is still in range...
                    if (scope->for_step_value >= 0) {
                        if (value <= scope->for_final_value) {
                            // set the variable to the stepped value
                            var_set(scope->for_variable_name, scope->for_variable_index, value);
                        } else {
                            run_condition = false;
                        }
                    } else {
                        if (value >= scope->for_final_value) {
                            // set the variable to the stepped value
                            var_set(scope->for_variable_name, scope->for_variable_index, value);
                        } else {
                            run_condition = false;
                        }
                    }

                    // conditionally go back for more (skip the for line)!
                    if (run_condition) {
                        run_line_number = scope->line_number;
                        // N.B we re-open the scope here!
                        goto XXX_PERF_XXX;
                    }
                } else if (code == code_endwhile) {
                    // go back for more (including the while line)!
                    run_line_number = scope->line_number-1;
                } else {
                    // if the condition has not been achieved...
                    if (! value) {
                        // go back for more (skip the do line)!
                        run_line_number = scope->line_number;
                        // N.B we re-open the scope here!
                        goto XXX_PERF_XXX;
                    }
                }
            }

            // close the conditional scope
            assert(cur_scopes);
            cur_scopes--;
            scope = scopes+cur_scopes;
XXX_PERF_XXX:
            break;

        case code_gosub:
            if (run_condition) {
                if (max_gosubs >= MAX_GOSUBS) {
                    printf("too many gosubs\n");
                    goto XXX_SKIP_XXX;
                }

                // open a new gosub scope
                gosubs[max_gosubs].return_line_number = run_line_number;
                gosubs[max_gosubs].return_in_library = run_in_library;
                gosubs[max_gosubs].return_var_scope = var_open_scope();
                gosubs[max_gosubs].return_scope = cur_scopes;
                max_gosubs++;

                // jump to the gosub line
                line = code_line(code_sub, bytecode+index, false, true, &run_in_library);
                if (line == NULL) {
                    printf("missing sub: %s\n", bytecode+index);
                    goto XXX_SKIP_XXX;
                }
                run_line_number = line->line_number-1;

                // skip sub name to address sub parameters.
                sub_bytecode = line->bytecode;
                assert(*sub_bytecode == code_sub);
                sub_bytecode += 1 + strlen((const char *)bytecode + index) + 1;
                sub_length = line->length - 1 - strlen((const char * )bytecode+index) - 1;

                index += strlen((char *)bytecode+index)+1;

                // for each sub parameter...
                while (sub_length > 0) {
                    assert(*sub_bytecode == code_load_and_push_var);
                    sub_bytecode++;
                    sub_length--;

                    // check that there's a gosub parameter to pass into this parameter
                    if (index >= length) {
                        printf("not enough gosub parameters\n");
                        goto XXX_SKIP_XXX;
                    }

                    index += run_expression(bytecode+index, length-index, &name, &value);

                    // if the param is an lvalue, then just get its name and index (if its an array element).
                    if (name) {
                        var_declare_reference((const char *)sub_bytecode, max_gosubs, name);
                    } else {
                        // the current parameter value is not an lvalue expression, pass it by-value into the sub's parameter.
                        var_declare((const char *)sub_bytecode, max_gosubs, code_ram, false, sizeof(uint32), 1, 0, 0, 0, -1, -1);
                        var_set((const char *)sub_bytecode, 0, value);
                    }

                    // if there's more bytecode for this gosub, they should be parameters separated by code_comma
                    if (index < length) {
                        assert(bytecode[index] == code_comma);
                        index++;
                    }

                    // advance over the sub's current parameter to the next parameter
                    n = strlen((const char *)sub_bytecode) + 1;
                    assert(sub_length >= n);
                    sub_length -= n;
                    sub_bytecode += n;

                    if (sub_length > 0 && *sub_bytecode == code_comma) {
                        sub_length--;
                        sub_bytecode++;
                    } else {
                        break;
                    }
                }

                // check to see that all gosub parameter were consumed.  check for call site parameter overflow.
                if (index != length) {
                    printf("too many gosub parameters\n");
                    goto XXX_SKIP_XXX;
                }
            } else {
                index = length;
            }
            break;

        case code_label:
        case code_sub:
            // we do nothing here
            // revisit -- we could push false conditional scope for sub (and pop on endsub)
            index += strlen((char *)bytecode+index)+1; // skip label/sub name

            // if there might be parameters...
            if ((code == code_sub) && (index < length)) {
                // skip parameters.
                while ((index < length) && (bytecode[index] == code_load_and_push_var)) {
                    index++; // skip code
                    index += strlen((char *)bytecode+index)+1; // skip parameter name
                    if (index < length) {
                        assert(bytecode[index] == code_comma);
                        index++;
                    }
                }
            }
            break;

        case code_return:
        case code_endsub:
            if (run_condition) {
                if (! max_gosubs) {
                    printf("missing gosub\n");
                    goto XXX_SKIP_XXX;
                }

                // close the gosub scope
                assert(max_gosubs);
                max_gosubs--;
                var_close_scope(gosubs[max_gosubs].return_var_scope);
                cur_scopes = gosubs[max_gosubs].return_scope;
                scope = scopes+cur_scopes;

                // and jump to the return line
                run_line_number = gosubs[max_gosubs].return_line_number;
                run_in_library = gosubs[max_gosubs].return_in_library;
            }
            break;

        case code_sleep:
            // N.B. sleeps occur in the main loop so we can service interrupts

            // get the sleep ticks
            index += run_timer(bytecode+index, length-index, &value);

            if (run_condition) {
                // prepare to sleep
                run_sleep_ticks = ticks+value;
                assert(! run_sleep_line_number);
                run_sleep_line_number = run_line_number;
            }
            break;

        case code_halt:
            if (run_condition) {
                // loop forever
                run_line_number--;
            }
            break;

        case code_stop:
            if (run_condition) {
                stop();
            }
            break;

        case code_end:
            if (run_condition) {
                // we end in a stopped infinite loop
                run_line_number = run_line_number-1;
                return true;
            }
            break;

        default:
            return run2_bytecode_code(code, bytecode, length);
            break;
    }

    assert(index == length);
    return false;

XXX_SKIP_XXX:
    stop();
    return false;
}

// this function executes a bytecode statement.
bool  // end
run_bytecode(bool immediate, const byte *bytecode, int length)
{
    assert(length);
    return run_bytecode_code(*bytecode, immediate, bytecode+1, length-1);
}

void
run_clear(bool flash)
{
    int i;

    // open an unconditional scope to run in
    cur_scopes = 0;
    scopes[cur_scopes].line_number = 0;
    scopes[cur_scopes].type = open_none;
    scopes[cur_scopes].condition = true;
    scopes[cur_scopes].condition_ever = true;
    scopes[cur_scopes].condition_initial = true;
    scopes[cur_scopes].condition_restore = false;

    max_gosubs = 0;

    run_isr_enabled = 0;
    run_isr_pending = 0;
    run_isr_masked = 0;
    for (i = 0; i < MAX_INTS; i++) {
        run_isr_bytecode[i] = NULL;
        run_isr_length[i] = 0;
    }

    memset(timers, 0, sizeof(timers));

    memset(uart_armed, 1, sizeof(uart_armed));

    memset(watchpoints, 0, sizeof(watchpoints));
    watchpoints_armed_mask = all_watchpoints_mask;

    memset(data_line_number, 0, sizeof(data_line_number));
    memset(data_line_offset, 0, sizeof(data_line_offset));

    code_clear2();
    var_clear(flash);

#if ! STICK_GUEST
    i2c_uninitialize();
#endif
}


// this function executes a BASIC program!!!
bool
run(bool cont, int start_line_number)
{
    int i;
    int32 tick;
    uint32 mask;
    int length;
    int32 value;
    int line_number;
    int32 last_tick;
    struct line *line;

    last_tick = ticks;

    if (! cont) {
        // prepare for a new run
        run_clear(false);
        run_line_number = start_line_number?start_line_number-1:0;
        run_in_library = false;
    } else {
        // continue a stopped run
        run_line_number = start_line_number?start_line_number-1:run_line_number;
    }
    run_sleep_line_number = 0;

    run_isr = false;
    run_stop = 1;

    // this is the main run loop that executes program statements!
    running = true;
    for (;;) {
#if STICK_GUEST
        // let timer ticks arrive
        timer_ticks(false);
#endif

        // if we're still sleeping...
        if (run_sleep_line_number && run_sleep_line_number == run_line_number) {
            if (ticks >= run_sleep_ticks) {
                run_sleep_line_number = 0;
            }
        } else {
            // we're running; get the next statement to execute
            line = code_next_line(false, run_in_library, &run_line_number);
            if (! line) {
                // we fell off the end
                break;
            }

            line_number = run_line_number;

            // if we're in trace mode...
            if (run_trace && line->bytecode[0] != code_halt && line->bytecode[0] != code_input) {
                // trace every line
                code_list(false, line_number, line_number, run_in_library);
            }

            // run the statement's bytecodes
            if (run_bytecode_code(*line->bytecode, false, line->bytecode+1, line->length-1)) {
                // we explicitly ended
                stop();
                break;
            }
        }

        // if we're in single-step mode...
        if (run_step) {
            // stop after every line
            stop();
        }

        // if we stopped for any reason...
        if (! run_stop) {
            printf("STOP at line %d!\n", line_number);
            break;
        }

        // *** interrupt service module ***

        // if it has been a tick since we last checked...
        tick = ticks;
        if (tick != last_tick) {
            // pin values may have changed so consider all watchpoints possible.  they're likely still false, and but
            // will be checked once per tick because of this.
            possible_watchpoints_mask = all_watchpoints_mask;

#if ! STICK_GUEST
            // If a msec elapsed...
            if ((tick & (ticks_per_msec - 1)) == 0) {
                // poll for non-isr work to do
                basic0_poll();

                // blink our led fast
                led_unknown_progress();
            }
#endif

            if (run_isr_enabled) {
                // see if any timers have expired
                for (i = 0; i < MAX_TIMERS; i++) {
                    // if the timer is set...
                    if (run_isr_bytecode[TIMER_INT(i)] && timers[i].interval_ticks) {
                        // if its time is due...
                        if ((int32)(tick - timers[i].last_ticks) >= timers[i].interval_ticks) {
                            // if it is not already pending...
                            if (! (run_isr_pending & (1 << TIMER_INT(i)))) {
                                // mark the interrupt as pending
                                run_isr_pending |= 1<<TIMER_INT(i);
                                timers[i].last_ticks = tick;
                            }
                        }
                    } else {
                        timers[i].last_ticks = tick;
                    }
                }
            }

            last_tick = tick;
        }

        // if any uart ints are enabled...
        if (run_isr_enabled & UART_MASK) {
            for (i = 0; i < MAX_UARTS; i++) {
#if ! STICK_GUEST
                // if the uart transmit int is enabled and armed...
                if (run_isr_bytecode[UART_INT(i, true)] && uart_armed[UART_INT(i, true)]) {
                    // if the uart transmitter is empty...
                    if (pin_uart_tx_empty(i)) {
                        // mark the interrupt as pending
                        run_isr_pending |= 1<<UART_INT(i, true);
                        uart_armed[UART_INT(i, true)] = false;
                    }
                }

                // if the uart receive int is enabled and armed...
                if (run_isr_bytecode[UART_INT(i, false)] && uart_armed[UART_INT(i, false)]) {
                    // if the uart receiver is full...
                    if (pin_uart_rx_ready(i)) {
                        // mark the interrupt as pending
                        run_isr_pending |= 1<<UART_INT(i, false);
                        uart_armed[UART_INT(i, false)] = false;
                    }
                }
#endif
            }
        }

        if (run_isr_enabled & WATCH_MASK) {
            for (i = 0; i < num_watchpoints; i++) {
                // skip if the watch int is disabled
                if (!(run_isr_enabled & (1 << WATCH_INT(i)))) {
                    continue;
                }
                // skip if this watchpoint is not set
                if (watchpoints[i].length == 0) {
                    continue;
                }
                // in smart watch mode, skip if this watchpoint is known to be false
                if (watch_mode_smart && !(possible_watchpoints_mask & (1<<i))) {
                    continue;
                }
                // evaluation watchpoint condition.  tell evaluator to decorate each evaluated pin/var with the current
                // watchpoint num so changes to these pins/vars set the watchpoint as possible.
                run_watchpoint = true;
                run_condition = true;
                length = run_expression_watchpoint(watchpoints[i].bytecode, watchpoints[i].length, 1 << i, NULL, &value);
                assert(length == watchpoints[i].length);
                run_watchpoint = false;

                // if the watch is non-0...
                if (value) {
                    // if this transition has not yet been delivered...
                    if (watchpoints_armed_mask & (1<<i)) {
                        // mark the interrupt as pending
                        run_isr_pending |= 1 << WATCH_INT(i);
                        watchpoints_armed_mask &= ~(1<<i);
                    }
                } else {
                    // watchpoint just evaluated to false.  it will remain false until on a var/pin change
                    possible_watchpoints_mask &= ~(1<<i);

                    watchpoints_armed_mask |= 1<<i;
                }
            }
        }

        // if we're not already running an isr...
      XXX_CHECK_ISRS_XXX:
        if (run_isr_pending && ! run_isr) {
            for (i = 0; i < MAX_INTS; i++) {
                // if we have an isr to run...
                mask = 1<<i;
                if ((run_isr_pending & mask) && ! (run_isr_masked & mask) && run_isr_bytecode[i]) {
                    // open a new temporary (unconditional) scope
                    assert(cur_scopes < MAX_SCOPES-1);
                    cur_scopes++;
                    scopes[cur_scopes].line_number = run_line_number;
                    scopes[cur_scopes].type = open_isr;

                    scopes[cur_scopes].condition = true;
                    scopes[cur_scopes].condition_ever = true;
                    scopes[cur_scopes].condition_initial = true;
                    scopes[cur_scopes].condition_restore = false;

                    // save the current sleep state
                    run_isr_sleep_ticks = run_sleep_ticks;
                    run_isr_sleep_line_number = run_sleep_line_number;
                    run_sleep_line_number = 0;

                    // *** interrupt handler ***

                    // run the isr, starting with the handler statement (which might be a gosub)
                    assert(! run_isr);
                    run_isr = true;
                    run_line_number = -1;
                    if (run_bytecode_code(*run_isr_bytecode[i], false, run_isr_bytecode[i]+1, run_isr_length[i]-1)) {
                        stop();
                    }

                    run_isr_pending &= ~(1<<i);

                    break;
                }
            }
        }

        // if we're returning from our isr...
        if (run_line_number == -1) {
            run_line_number = scopes[cur_scopes].line_number;

            // restore the current sleep state
            run_sleep_ticks = run_isr_sleep_ticks;
            run_sleep_line_number = run_isr_sleep_line_number;

            // close the temporary (unconditional) scope
            assert(scopes[cur_scopes].type == open_isr);
            assert(cur_scopes);
            cur_scopes--;

            assert(run_isr);
            run_isr = false;

            // check for more isrs before running any non-isr code.  multiple watchpoints may be enabled and should all be
            // executed to completion before continuing non-isr code.
            goto XXX_CHECK_ISRS_XXX;
        }
    }
    running = false;

    if (! run_stop) {
        // we stopped or ended
        return false;
    } else {
        // we fell off the end
        while (cur_scopes) {
            if (scopes[cur_scopes].type != open_isr) {
                printf("missing %s\n", scopes[cur_scopes].type == open_if ? "endif" : "endwhile/next");
            }
            assert(cur_scopes);
            cur_scopes--;
        }

        run_stop = false;
        return true;  // we completed
    }
}

// this function stops execution of the BASIC program.
void
stop()
{
    run_stop = 0;
    run_breaks++;
}

void
run_initialize(void)
{
    watch_mode_smart = !! var_get_flash(FLASH_WATCH_SMART);
    assert(MAX_INTS < sizeof(run_isr_enabled)*8);
}

