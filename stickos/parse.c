// *** parse.c ********************************************************
// this file implements the bytecode compiler and de-compiler for
// stickos.

// Copyright (c) CPUStick.com, 2008-2023.  All rights reserved.
// Patent U.S. 8,117,587.

#include "main.h"

static
const struct op {
    char *op;
    enum bytecode code;
    int precedence;
} ops[] = {
    // double character
    "||", code_logical_or, 0,
    "&&", code_logical_and, 1,
    "^^", code_logical_xor, 2,
    "==", code_equal, 6,
    "!=", code_not_equal, 6,
    "<=", code_less_or_equal, 7,
    ">=", code_greater_or_equal, 7,
    ">>", code_shift_right, 8,
    "<<", code_shift_left, 8,

    // single character (follow double character)
    "|", code_bitwise_or, 3,
    "^", code_bitwise_xor, 4,
    "&", code_bitwise_and, 5,
    "<", code_less, 7,
    ">", code_greater, 7,
    "+", code_add, 9,
    "+", code_unary_plus, 12,  // N.B. must follow code_add
    "-", code_subtract, 9,
    "-", code_unary_minus, 12,  // N.B. must follow code_subtract
    "*", code_multiply, 10,
    "/", code_divide, 10,
    "%", code_mod, 10,
    "!", code_logical_not, 11,  // right to left associativity
    "~", code_bitwise_not, 11,  // right to left associativity
};

static
const struct rel {
    char *rel;
    enum bytecode code;
} rels[] = {
    // double character
    "==", code_equal,
    "!=", code_not_equal,
    "!~", code_not_contains,
    "<=", code_less_or_equal,
    ">=", code_greater_or_equal,

    // single character (follow double character)
    "~", code_contains,
    "<", code_less,
    ">", code_greater,
};

static
const struct keyword {
    char *keyword;
    enum bytecode code;
} keywords[] = {
    "assert", code_assert,
    "break", code_break,
    "configure", code_configure,
    "continue", code_continue,
    "data", code_data,
    "dim", code_dim,
    "do", code_do,
    "elseif", code_elseif,
    "else", code_else,  // N.B. must follow code_elseif
    "endif", code_endif,
    "endsub", code_endsub,
    "endwhile", code_endwhile,
    "end", code_end,  // N.B. must follow code_endif, code_endsub, and code_endwhile
    "for", code_for,
    "gosub", code_gosub,
    "halt", code_halt,
    "i2c", code_i2c,
    "if", code_if,
    "input", code_input,
    "label", code_label,
    "let", code_let,
    "", code_nolet,
    "mask", code_mask,
    "next", code_next,
    "off", code_off,
    "on", code_on,
    "print", code_print,
    "?", code_print,
    "qspi", code_qspi,
    "read", code_read,
    "rem", code_rem,
    "", code_norem,
    "restore", code_restore,
    "return", code_return,
    "sleep", code_sleep,
    "vprint", code_vprint,
    "stop", code_stop,
    "sub", code_sub,
    "uart", code_uart,
    "unmask", code_unmask,
    "until", code_until,
    "while", code_while
};


void
parse_trim(IN char **p)
{
    // advance *p past any leading spaces
    while (isspace(**p)) {
        (*p)++;
    }
}

bool
parse_char(IN OUT char **text, IN char c)
{
    if (**text != c) {
        return false;
    }

    // advance *text past c
    (*text)++;

    parse_trim(text);
    return true;
}

bool
parse_word(IN OUT char **text, IN const char *word)
{
    int len;

    len = strlen(word);

    if (strncmp(*text, word, len)) {
        return false;
    }

    // advance *text past word
    (*text) += len;

    parse_trim(text);
    return true;
}

bool
parse_wordn(IN OUT char **text, IN const char *word)
{
    int len;

    len = strlen(word);

    if (strncmp(*text, word, len) || isdigit(*(*text+len))) {
        return false;
    }

    // advance *text past word
    (*text) += len;

    parse_trim(text);
    return true;
}

bool
parse_words(IN OUT char **text_in, IN OUT char **text_err, IN const char *words)
{
    int len;
    char *p;
    char *text;

    text = *text_in;
    while (words[0]) {
        len = strlen(words);
        p = strchr(words, ' ');
        if (p) {
            len = MIN(len, p-words);
        }

        if (strncmp(text, words, len)) {
            if (text > *text_err) {
                *text_err = text;
            }
            return false;
        }

        text += len;
        words += len;
        parse_trim(&text);
        if (words[0]) {
            assert(words[0] == ' ');
            words++;  // N.B. we assume a single space
        }
    }

    // advance *text_in past words
    *text_in = text;
    return true;
}

char *
parse_find_keyword(IN OUT char *text, IN char *word)
{
    int n;
    bool w;
    char *p;

    n = strlen(word);

    // find delimited keyword in text
    w = false;
    for (p = text; *p; p++) {
        if (! w && ! strncmp(p, word, n) && ! isalpha(*(p+n))) {
            return p;
        }
        w = isalpha(*p) && isalpha(*word);
    }
    return NULL;
}

// find final keyword in text
bool
parse_find_tail(IN OUT char *text, IN char *tail)
{
    int n;
    int m;

    n = strlen(text);
    m = strlen(tail);

    while (n && isspace(text[n-1])) {
        n--;
        text[n] = '\0';
    }

    if (m > n) {
        return false;
    }

    if (strcmp(text+n-m, tail)) {
        return false;
    }

    text[n-m] = '\0';
    return true;
}

char *
parse_match_paren(char *p)
{
    char c;
    char cc;
    int level;

    assert(*p == '(' || *p == '[');
    cc = *p;

    // return a pointer to the matching close parenthesis or brace of p
    level = 0;
    for (;;) {
        c = *p;
        if (! c) {
            break;
        }
        if (c == cc) {
            level++;
        } else if (c == ((cc == '(') ? ')' : ']')) {
            assert(level);
            level--;
            if (! level) {
                return p;
            }
        }
        p++;
    }

    return NULL;
}

char *
parse_match_quote(char *p)
{
    char c;

    assert(*p == '"');

    // return a pointer to the matching close quote of p
    p++;
    for (;;) {
        c = *p;
        if (! c) {
            break;
        }
        if (c == '"') {
            return p;
        }
        p++;
    }

    return NULL;
}

// *** bytecode compiler ***

// revisit -- merge this with run.c/basic.c/parse.c???
bool
parse_const(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    byte c;
    bool neg;
    int32 value;

    // if this is an ascii constant
    if ((*text)[0] == '\'') {
        value = (byte)((*text)[1]);
        if ((*text)[2] != '\'') {
            return false;
        }
        (*text) += 3;
        bytecode[(*length)++] = code_load_and_push_immediate_ascii;
    } else {
        // parse constant value and advance *text past constant
        value = 0;
        if ((*text)[0] == '0' && (*text)[1] == 'x') {
            (*text) += 2;
            bytecode[(*length)++] = code_load_and_push_immediate_hex;
            for (;;) {
                c = (*text)[0];
                if (c >= 'a' && c <= 'f') {
                    value = value*16 + 10 + c - 'a';
                } else if (isdigit(c)) {
                    value = value*16 + c - '0';
                } else {
                    break;
                }
                (*text)++;
            }
        } else {
            neg = false;
            if ((*text)[0] == '-') {
                neg = true;
                (*text)++;
            }
            if (! isdigit((*text)[0])) {
                return false;
            }
            bytecode[(*length)++] = code_load_and_push_immediate;
            while (isdigit((*text)[0])) {
                value = value*10 + (*text)[0] - '0';
                (*text)++;
            }
            if (neg) {
                value = -value;
            }
        }
    }

    write32(bytecode+*length, value);
    *length += sizeof(uint32);

    parse_trim(text);
    return true;
}

bool
parse_word_const_word(IN char *prefix, IN char *suffix, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    if (prefix && ! parse_word(text, prefix)) {
        return false;
    }
    if (! parse_const(text, length, bytecode)) {
        return false;
    }
    if (suffix && ! parse_word(text, suffix)) {
        return false;
    }
    return true;
}

static
bool
isvarstart(char c)
{
    return isalpha(c) || (c) == '_';
}

static
bool
isvarmiddle(char c)
{
    return isalpha(c) || isdigit(c) || (c) == '_';
}

// this function parses (compiles) a variable to bytecode.
bool
parse_var(IN bool string, IN bool lvalue, IN int indices, IN int obase, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    int i;
    char *p;
    char *q;
    int32 *hole;
    int olength;
    char name[BASIC_OUTPUT_LINE_SIZE];

    if (! isvarstart(**text)) {
        return false;
    }

    // extract the variable name and advance *text past name
    i = 0;
    while (isvarmiddle(**text)) {
        name[i++] = *(*text)++;
    }
    name[i] = '\0';

    if (string) {
        // advance *text past '$'
        if (**text != '$') {
            return false;
        }
        (*text)++;
    }

    // if the variable is an array element...
    if (**text == '[') {
        if (! indices) {
            return false;
        }

        if (lvalue || string) {
            // for lvalues or strings we generate the code first, followed by the index expression length(s)
            bytecode[(*length)++] = string ? code_load_string_indexed : code_load_and_push_var_indexed;
            hole = (int32 *)(bytecode + *length);
            *length += sizeof(uint32);
            if (indices == 2) {
                *length += sizeof(uint32);
            }
            olength = *length;
        }

        // find the end of the array index expression
        p = parse_match_paren(*text);
        if (! p) {
            return false;
        }
        assert(*p == ']');
        *p = '\0';
        assert(**text == '[');
        (*text)++;

        // if we're allowed (required!) 2 indices...
        if (indices == 2) {
            // find the colon
            assert(string);
            q = strchr(*text, ':');
            if (! q) {
                return false;
            }
            *q = '\0';
        }

        // parse the (first) array index expression
        if (! parse_expression(obase, text, length, bytecode)) {
            return false;
        }

        if (indices == 2) {
            *text = q;
        } else {
            *text = p;
        }
        (*text)++;

        if (lvalue || string) {
            // fill in the index expression length from the parse
            write32((byte *)hole, *length - olength);

            // if we're allowed 2 indices...
            if (indices == 2) {
                assert(string);
                hole++;

                // parse the second array index expression
                olength = *length;
                if (! parse_expression(0, text, length, bytecode)) {
                    return false;
                }

                // fill in the index expression length from the parse
                write32((byte *)hole, *length - olength);

                *text = p;
                (*text)++;
            }
        } else {
            // for non-string rvalues we generate the code last, following the index expression
            assert(! string);
            bytecode[(*length)++] = code_load_and_push_var_indexed;
        }
    } else {
        // if this is a variable length request...
        if (! string && **text == '#') {
            (*text)++;

            // generate the code
            bytecode[(*length)++] = code_load_and_push_var_length;
        } else {
            // for simple variables we just generate the code
            bytecode[(*length)++] = string ? code_load_string : code_load_and_push_var;
        }
    }

    // generate the variable name to bytecode, and advance *length past the name
    for (i = 0; name[i]; i++) {
        bytecode[(*length)++] = name[i];
    }
    bytecode[(*length)++] = '\0';

    parse_trim(text);
    return true;
}

#define MAX_OP_STACK  10

static const struct op *op_stack[MAX_OP_STACK];

// this function determines which stacked operators need to be popped.
static
void
parse_clean(IN int obase, IN OUT int *otop, IN int precedence, IN OUT int *length, IN OUT byte *bytecode)
{
    int j;
    bool pop;

    for (j = *otop-1; j >= obase+1; j--) {
        assert(op_stack[j]->precedence >= op_stack[j-1]->precedence);
    }

    // for pending operators at the top of the stack...
    for (j = *otop-1; j >= obase; j--) {
        // if the operator has right to left associativity...
        if (op_stack[j]->code == code_logical_not || op_stack[j]->code == code_bitwise_not) {
            // pop if the operator precedence is greater than the current precedence
            pop = op_stack[j]->precedence > precedence;
        } else {
            // pop if the operator precedence is greater than or equal to the current precedence
            pop = op_stack[j]->precedence >= precedence;
        }

        // if the operator needs to be popped...
        if (pop) {
            // pop it
            *otop = j;

            // and generate its bytecode
            bytecode[(*length)++] = op_stack[j]->code;
        } else {
            // keep the remaining (low precedence) operators on the stack for now
            break;
        }
    }
}

// this function parses (compiles) an expression to bytecode.
bool
parse_expression(IN int obase, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    int i;
    char c;
    char *p;
    int len;
    int otop;
    bool number;

    otop = obase;

    number = true;
    for (;;) {
        c = **text;
        if (! c || c == ',') {
            if (number) {
                return false;
            }
            break;
        }

        // if this is the start of a number...
        if (isdigit(c) || c == '\'') {
            if (! number) {
                return false;
            }

            // generate the bytecode and parse the constant
            if (! parse_const(text, length, bytecode)) {
                return false;
            }
            number = false;

        // otherwise, if this is the start of a variable...
        } else if (isvarstart(c)) {
            if (! number) {
                return false;
            }

            // generate the bytecode and parse the variable
            if (! parse_var(false, false, 1, otop, text, length, bytecode)) {
                return false;
            }
            number = false;

        } else {
            // if this is the start of a parenthetical expression...
            if (c == '(') {
                if (! number) {
                    return false;
                }
                p = parse_match_paren(*text);
                if (! p) {
                    return false;
                }
                assert(*p == ')');
                *p = '\0';
                assert(**text == '(');
                (*text)++;

                // parse the sub-expression
                if (! parse_expression(otop, text, length, bytecode)) {
                    return false;
                }

                *text = p;
                (*text)++;
                number = false;

            // otherwise, this should be an operator...
            } else {
                for (i = 0; i < LENGTHOF(ops); i++) {
                    len = strlen(ops[i].op);
                    if (! strncmp(*text, ops[i].op, len)) {
                        break;
                    }
                }
                if (i == LENGTHOF(ops)) {
                    return false;
                }

                if (number && ops[i].code == code_add) {
                    i++;  // convert to unary
                    assert(ops[i].code == code_unary_plus);
                } else if (number && ops[i].code == code_subtract) {
                    i++;  // convert to unary
                    assert(ops[i].code == code_unary_minus);
                } else {
                    if (ops[i].code == code_logical_not || ops[i].code == code_bitwise_not) {
                        if (! number) {
                            return false;
                        }
                    } else {
                        if (number) {
                            return false;
                        }
                    }

                    // if we need to clean the top of the op stack
                    parse_clean(obase, &otop, ops[i].precedence, length, bytecode);
                    if (ops[i].code != code_logical_not && ops[i].code != code_bitwise_not) {
                        number = true;
                    }
                }

                (*text) += len;

                if (otop >= MAX_OP_STACK) {
                    return false;
                }

                // push the new top
                op_stack[otop++] = &ops[i];
            }

            parse_trim(text);
        }
    }

    // clean the entire op stack!
    parse_clean(obase, &otop, 0, length, bytecode);
    assert(otop == obase);
    return true;
}

bool
parse_string(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    char c;
    char *p;
    bool string;

    string = true;
    for (;;) {
        c = **text;

        // if this is the start of a string literal
        if (c == '"') {
            if (! string) {
                return false;
            }

            bytecode[(*length)++] = code_text;

            // find the matching quote
            p = parse_match_quote(*text);
            if (! p) {
                return false;
            }

            assert(*p == '"');
            *p = '\0';
            assert(**text == '"');
            (*text)++;

            // generate the string to bytecode, and advance length and *text past the name
            while (**text) {
                bytecode[(*length)++] = *(*text)++;
            }
            bytecode[(*length)++] = **text;

            assert(*text == p);
            (*text)++;

        // otherwise, if this is the start of a variable...
        } else if (isvarstart(c)) {
            if (! string) {
                return false;
            }

            if (! parse_var(true, false, 2, 0, text, length, bytecode)) {
                return false;
            }

        // otherwise, this should be an operator...
        } else if (c == '+') {
            if (string) {
                return false;
            }
            (*text)++;
        } else {
            break;
        }

        string = ! string;
        parse_trim(text);
    }
    if (string) {
        return false;
    }
    return true;
}

// this function parses (compiles) a string relation to bytecode.
bool
parse_relation(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    int i;

    if (! parse_string(text, length, bytecode)) {
        return false;
    }
    bytecode[(*length)++] = code_comma;

    for (i = 0; i < LENGTHOF(rels); i++) {
        if (parse_word(text, rels[i].rel)) {
            bytecode[(*length)++] = rels[i].code;
            break;
        }
    }
    if (i == LENGTHOF(rels)) {
        return false;
    }

    if (! parse_string(text, length, bytecode)) {
        return false;
    }
    bytecode[(*length)++] = code_comma;

    return true;
}

bool
parse_class(IN char *text, IN OUT int *length, IN OUT byte *bytecode)
{
    char *p;
    char *q;

    p = strchr(text, ',');
    if (! p) {
        p = strchr(text, '\0');
    }
    q = strchr(text, '=');
    if (q && q < p) {
        p = q;
    }
    while (text != p) {
        if (*text == '$' || *text == '"') {
            bytecode[(*length)++] = code_string;
            return true;
        }
        text++;
    }
    bytecode[(*length)++] = code_expression;
    return false;
}

bool
parse_string_or_expression(IN bool string, IN char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    bool boo;

    // if the next item is a string...
    if (string) {
        // parse the string
        boo = parse_string(text, length, bytecode);

    // otherwise, the next item is an expression...
    } else {
        // parse the expression
        boo = parse_expression(0, text, length, bytecode);
    }

    return boo;
}

static
bool
parse_uart(IN char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    int uart;

    // parse the uart name
    for (uart = 0; uart < MAX_UARTS; uart++) {
        if (parse_word(text, uart_names[uart])) {
            break;
        }
    }
    if (uart == MAX_UARTS) {
        return false;
    }
    bytecode[(*length)++] = uart;
    return true;
}

static
void
parse_format(IN char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    if (parse_word(text, "hex")) {
        bytecode[(*length)++] = code_hex;
    } else if (parse_word(text, "dec")) {
        bytecode[(*length)++] = code_dec;
    } else if (parse_word(text, "raw")) {
        bytecode[(*length)++] = code_raw;
    }
}

static
bool
parse_time(IN char **text, IN OUT int *length, IN OUT byte *bytecode)
{
    int i;

    // parse and push the trailing timer interval unit specifier
    for (i = 0; i < timer_unit_max; i++) {
        if (parse_find_tail(*text, timer_units[i].name)) {
            bytecode[(*length)++] = i;
            break;
        }
    }
    if (i == timer_unit_max) {
        *text += strlen(*text);
        return false;
    }

    // parse the sleep time expression
    return parse_expression(0, text, length, bytecode);
}

// this function parses (compiles) a public statement line to bytecode,
// excluding the keyword.
bool
parse_line_code(IN byte code, IN char *text_in, OUT int *length_out, OUT byte *bytecode, OUT int *syntax_error)
{
    int i;
    char *d;
    char *p;
    int len;
    int length;
    int olength;
    bool string;
    bool multi;
    char *text;
    char *text_ok;
    char *text_err;
    int pin_type;
    int pin_qual;
    int pin_number;
    int ram_or_abs_var_size;

    length = 0;
    text = text_in;
    multi = false;

XXX_AGAIN_XXX:
    parse_trim(&text);

    // if this is a multi-statement...
    if (length) {
        assert(multi);
        // parse the comma
        if (! parse_char(&text, ',')) {
            goto XXX_ERROR_XXX;
        }
        if (code != code_data) {
            bytecode[length++] = code_comma;
        }
    }

    switch (code) {
        case code_rem:
            // generate the comment to bytecode
            while (*text) {
                bytecode[length++] = *text++;
            }
            bytecode[length++] = *text;
            break;
        case code_norem:
            break;

        case code_on:
        case code_off:
        case code_mask:
        case code_unmask:
            // parse the device type
            if (parse_word(&text, "timer")) {
                bytecode[length++] = code_timer;

                // parse the timer number
                text_ok = text;
                if (! parse_const(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
                assert(length >= sizeof(uint32));
                if (read32(bytecode+length-sizeof(uint32)) < 0 || read32(bytecode+length-sizeof(uint32)) >= MAX_TIMERS) {
                    text = text_ok;
                    goto XXX_ERROR_XXX;
                }

            } else if (parse_word(&text, "uart")) {
                bytecode[length++] = code_uart;

                // parse the uart name
                if (! parse_uart(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }

                // parse the uart data direction
                if (parse_word(&text, "output")) {
                    bytecode[length++] = 1;
                } else if (parse_word(&text, "input")) {
                    bytecode[length++] = 0;
                } else {
                    goto XXX_ERROR_XXX;
                }
            } else {
                // this should be an expression

                // find the "do"
                d = parse_find_keyword(text, "do");

                bytecode[length++] = code_watch;

                // parse the expression
                if (d) {
                    assert(*d == 'd');
                    *d = '\0';
                }
                len = length+sizeof(uint32);
                if (! parse_expression(0, &text, &len, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
                write32(bytecode+length, len-(length+sizeof(uint32)));
                length = len;
                text += strlen(text);
                if (d) {
                    *d = 'd';
                }

                bytecode[length++] = code_comma;
            }

            // if we're enabling interrupts...
            if (code == code_on) {
                // parse the "do"
                if (! parse_word(&text, "do")) {
                    goto XXX_ERROR_XXX;
                }

                // parse the interrupt handler statement
                if (! parse_line(text, &len, bytecode+length+sizeof(uint32), syntax_error)) {
                    goto XXX_ERROR_XXX;
                }
                write32(bytecode+length, len);
                length += sizeof(uint32);
                length += len;
                text += strlen(text);
            }
            break;

        case code_configure:
            // parse the device type
            if (parse_word(&text, "timer")) {
                bytecode[length++] = code_timer;

                // parse the timer number
                text_ok = text;
                if (! parse_const(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
                assert(length >= sizeof(uint32));
                if (read32(bytecode+length-sizeof(uint32)) < 0 || read32(bytecode+length-sizeof(uint32)) >= MAX_TIMERS) {
                    text = text_ok;
                    goto XXX_ERROR_XXX;
                }

                if (! parse_word(&text, "for")) {
                    goto XXX_ERROR_XXX;
                }

                if (! parse_time(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }

            } else if (parse_word(&text, "uart")) {
                bytecode[length++] = code_uart;

                // parse the uart name
                if (! parse_uart(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }

                // parse the baud rate
                if (! parse_word_const_word("for", "baud", &text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }

                // parse the data bits
                text_ok = text;
                if (! parse_word_const_word(NULL, "data", &text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
                assert(length >= sizeof(uint32));
                if (read32(bytecode+length-sizeof(uint32)) < 5 || read32(bytecode+length-sizeof(uint32)) > 8) {
                    text = text_ok;
                    goto XXX_ERROR_XXX;
                }

                // parse the parity
                if (parse_word(&text, "even")) {
                    bytecode[length++] = 0;
                } else if (parse_word(&text, "odd")) {
                    bytecode[length++] = 1;
                } else if (parse_word(&text, "no")) {
                    bytecode[length++] = 2;
                } else {
                    goto XXX_ERROR_XXX;
                }
                if (! parse_word(&text, "parity")) {
                    goto XXX_ERROR_XXX;
                }

                // parse the optional loopback specifier
                if (parse_word(&text, "loopback")) {
                    bytecode[length++] = 1;
                } else {
                    bytecode[length++] = 0;
                }
            } else {
                goto XXX_ERROR_XXX;
            }
            break;

        case code_assert:
            // parse the assertion expression
            if (! parse_expression(0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }
            break;

        case code_read:
            // parse the variable
            if (! parse_var(false, true, 1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_data:
            // parse the constant
            assert(code == code_data);
            if (! parse_const(&text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_dim:
            string = parse_class(text, &length, bytecode);

            // parse the variable
            if (! parse_var(string, true, 1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            ram_or_abs_var_size = 0;

            // if the user specified "as"...
            if (parse_word(&text, "as")) {
                if (string) {
                    goto XXX_ERROR_XXX;
                }

                // parse the size specifier
                if (parse_word(&text, "byte")) {
                    ram_or_abs_var_size = 1;
                } else if (parse_word(&text, "short")) {
                    ram_or_abs_var_size = 2;
                } else {
                    bytecode[length++] = sizeof(uint32);

                    // parse the type specifier
                    if (parse_word(&text, "flash")) {
                        bytecode[length++] = code_flash;
                    } else if (parse_word(&text, "pin")) {
                        bytecode[length++] = code_pin;

                        // parse the pin name
                        for (pin_number = 0; pin_number < pin_last; pin_number++) {
                            if (parse_wordn(&text, pins[pin_number].name)) {
                                break;
                            }
                        }
                        if (pin_number == pin_last) {
                            goto XXX_ERROR_XXX;
                        }
                        assert(pin_number < pin_last);
                        bytecode[length++] = pin_number;

                        if (! parse_word(&text, "for")) {
                            goto XXX_ERROR_XXX;
                        }

                        // parse the pin usage
                        text_ok = text;
                        text_err = NULL;
                        for (pin_type = 0; pin_type < pin_type_last; pin_type++) {
                            if (parse_words(&text, &text_err, pin_type_names[pin_type])) {
                                break;
                            }
                        }
                        if (pin_type == pin_type_last) {
                            assert(text_err);
                            text = text_err;
                            goto XXX_ERROR_XXX;
                        }
                        bytecode[length++] = pin_type;

                        // parse the pin qualifier(s)
                        pin_qual = 0;
                        for (i = 0; i < pin_qual_last; i++) {
                            if (parse_word(&text, pin_qual_names[i])) {
                                pin_qual |= 1<<i;
                            }
                        }
                        bytecode[length++] = pin_qual;

                        // see if the pin usage is legal
                        if (! (pins[pin_number].pin_type_mask & (1<<pin_type))) {
                            text = text_ok;
                            printf("unsupported pin type\n");
                            goto XXX_ERROR_XXX;
                        }

                        // see if the pin qualifier usage is legal
                        if (pin_qual &~ pin_qual_mask[pin_type]) {
                            printf("unsupported pin qualifier\n");
                            goto XXX_ERROR_XXX;
                        }
                    } else if (parse_words(&text, &text, "remote on nodeid")) {
                        bytecode[length++] = code_nodeid;

                        // parse the nodeid
                        if (! parse_const(&text, &length, bytecode)) {
                            goto XXX_ERROR_XXX;
                        }
                    } else {
                        goto XXX_ERROR_XXX;
                    }
                }
            } else {
                ram_or_abs_var_size = string?1:4;
            }

            // if parsing a ram or absolute variable...
            if (ram_or_abs_var_size) {
                bytecode[length++] = ram_or_abs_var_size;
                // parse optional "at address <address>" suffix
                text_err = NULL;
                if (parse_words(&text, &text_err, "at address")) {
                    bytecode[length++] = code_absolute;
                    // parse the <address>
                    if (! parse_expression(0, &text, &length, bytecode)) {
                        goto XXX_ERROR_XXX;
                    }
                } else if (*text && *text != ',') {
                    assert(text_err);
                    text = text_err;
                    goto XXX_ERROR_XXX;
                } else {
                    bytecode[length++] = code_ram;
                }
            }

            multi = true;
            break;

        case code_let:
        case code_nolet:
            string = parse_class(text, &length, bytecode);

            // parse the variable
            if (! parse_var(string, true, string?0:1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            // parse the assignment
            if (! parse_char(&text, '=')) {
                goto XXX_ERROR_XXX;
            }

            // parse the string or expression
            if (! parse_string_or_expression(string, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_input:
            parse_format(&text, &length, bytecode);

            string = parse_class(text, &length, bytecode);

            // parse the variable
            if (! parse_var(string, true, string?0:1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_print:
        case code_vprint:
            if (! length) {
                // if this is an vprint...
                if (code == code_vprint) {
                    string = parse_class(text, &length, bytecode);

                    // parse the variable
                    if (! parse_var(string, true, string?0:1, 0, &text, &length, bytecode)) {
                        goto XXX_ERROR_XXX;
                    }

                    // parse the assignment
                    if (! parse_char(&text, '=')) {
                        goto XXX_ERROR_XXX;
                    }
                }

                // if we're not printing a newline...
                if (parse_find_tail(text, ";")) {
                    bytecode[length++] = code_semi;
                }
            }

            parse_format(&text, &length, bytecode);

            string = parse_class(text, &length, bytecode);

            // parse the string or expression
            if (! parse_string_or_expression(string, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_uart:
        case code_qspi:
        case code_i2c:
            if (! length) {
                if (code == code_uart) {
                    // parse the uart name
                    if (! parse_uart(&text, &length, bytecode)) {
                        goto XXX_ERROR_XXX;
                    }
                }

                if (code == code_i2c && parse_word(&text, "start")) {
                    bytecode[length++] = code_device_start;
                    // parse the address
                    if (! parse_expression(0, &text, &length, bytecode)) {
                        goto XXX_ERROR_XXX;
                    }
                    break;
                } else if (code == code_i2c && parse_word(&text, "stop")) {
                    bytecode[length++] = code_device_stop;
                    break;
                } else

                if ((code == code_uart || code == code_i2c) && parse_word(&text, "read")) {
                    bytecode[length++] = code_device_read;
                } else if ((code == code_uart || code == code_i2c) && parse_word(&text, "write")) {
                    bytecode[length++] = code_device_write;
                } else if (code == code_uart || code == code_i2c) {
                    goto XXX_ERROR_XXX;
                }
            }

            // parse the variable
            if (! parse_var(false, true, 1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            multi = true;
            break;

        case code_if:
        case code_elseif:
        case code_while:
        case code_until:
            string = parse_class(text, &length, bytecode);

            if (code == code_if || code == code_elseif) {
                // make sure we have a "then"
                if (! parse_find_tail(text, "then")) {
                    text += strlen(text);
                    goto XXX_ERROR_XXX;
                }
            } else if (code == code_while) {
                // make sure we have a "do"
                if (! parse_find_tail(text, "do")) {
                    text += strlen(text);
                    goto XXX_ERROR_XXX;
                }
            }

            if (string) {
                // parse the string relation
                if (! parse_relation(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
            } else {
                // parse the conditional expression
                if (! parse_expression(0, &text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
            }
            break;

        case code_else:
        case code_endif:
        case code_endwhile:
        case code_do:
            // nothing to do here
            break;

        case code_break:
        case code_continue:
            // if the user specified a break/continue level...
            if (*text) {
                // parse the break/continue level
                text_ok = text;
                if (! parse_const(&text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
                assert(length >= sizeof(uint32));
                if (! read32(bytecode+length-sizeof(uint32))) {
                    text = text_ok;
                    goto XXX_ERROR_XXX;
                }
            }
            break;

        case code_for:
            // parse the for loop variable
            if (! parse_var(false, true, 1, 0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }

            // parse the assignment
            if (! parse_char(&text, '=')) {
                goto XXX_ERROR_XXX;
            }

            // find the "to" keyword
            p = parse_find_keyword(text, "to");
            if (! p) {
                goto XXX_ERROR_XXX;
            }
            *p = 0;

            // parse initial value
            if (! parse_expression(0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            text = p+2;
            parse_trim(&text);

            // see if there is a "step" keyword
            p = parse_find_keyword(text, "step");
            if (p) {
                *p = 0;
            }

            bytecode[length++] = code_comma;

            // parse final value
            if (! parse_expression(0, &text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            // if there was a step keyword...
            if (p) {
                text = p+4;
                parse_trim(&text);

                bytecode[length++] = code_comma;

                // parse step value
                if (! parse_expression(0, &text, &length, bytecode)) {
                    goto XXX_ERROR_XXX;
                }
            }
            break;

        case code_next:
            // nothing to do here
            break;

        case code_label:
        case code_restore:
        case code_gosub:
        case code_sub:
            if (! *text && code != code_restore) {
                goto XXX_ERROR_XXX;
            }
            // generate the label/subname to bytecode
            while (*text && isvarmiddle(*text)) {
                bytecode[length++] = *text++;
            }
            bytecode[length++] = '\0';

            parse_trim(&text);

            // if there's more text after the sub name...
            if (*text) {
                if ((code != code_sub) && (code != code_gosub)) {
                    goto XXX_ERROR_XXX;
                }

                // while there are more parameters to parse...
                olength = length;
                while (*text) {
                    if (length > olength) {
                        if (! parse_char(&text, ',')) {
                            goto XXX_ERROR_XXX;
                        }
                        bytecode[length++] = code_comma;
                    }

                    if (code == code_sub) {
                        // parse the next parameter name
                        if (! parse_var(false, true, 0, 0, &text, &length, bytecode)) {
                            goto XXX_ERROR_XXX;
                        }
                    } else if (code == code_gosub) {
                        // parse the next parameter to pass
                        if (! parse_expression(0, &text, &length, bytecode)) {
                            goto XXX_ERROR_XXX;
                        }
                    }
                }
            }
            break;

        case code_return:
        case code_endsub:
            // nothing to do here
            break;

        case code_sleep:
            if (! parse_time(&text, &length, bytecode)) {
                goto XXX_ERROR_XXX;
            }
            break;

        case code_halt:
        case code_stop:
        case code_end:
            // nothing to do here
            break;

        default:
            assert(0);
            break;
    }

    if (*text && multi) {
        goto XXX_AGAIN_XXX;
    }
    if (*text) {
        goto XXX_ERROR_XXX;
    }

    *length_out = length;
    return true;

XXX_ERROR_XXX:
    *syntax_error = text - text_in;
    assert(*syntax_error >= 0 && *syntax_error < BASIC_OUTPUT_LINE_SIZE);
    return false;
}

// this function parses (compiles) a statement line to bytecode.
bool
parse_line(IN char *text_in, OUT int *length_out, OUT byte *bytecode, OUT int *syntax_error_in)
{
    int i;
    int len;
    bool boo;
    byte code;
    int length;
    char *text;
    int syntax_error1;
    int syntax_error2;

    syntax_error1 = 0;
    syntax_error2 = 0;

    text = text_in;
    parse_trim(&text);

    // check for public commands
    for (i = 0; i < LENGTHOF(keywords); i++) {
        len = strlen(keywords[i].keyword);
        if (len && ! strncmp(text, keywords[i].keyword, len)) {
            text += len;
            code = keywords[i].code;
            break;
        }
    }

    // if no command was found...
    if (i == LENGTHOF(keywords)) {
        // check for private commands
        if (parse2_line(text_in, length_out, bytecode, &syntax_error1)) {
            return true;
        }

        if (*text) {
            // assume a let (nolet)
            code = code_nolet;
        } else {
            // assume a rem (norem)
            code = code_norem;
        }
    }

    *bytecode = code;

    // parse the public command
    boo = parse_line_code(code, text, &length, bytecode+1, &syntax_error2);
    if (! boo) {
        *syntax_error_in = text - text_in + MAX(syntax_error1, syntax_error2);
        assert(*syntax_error_in >= 0 && *syntax_error_in < BASIC_OUTPUT_LINE_SIZE);
        return boo;
    }

    *length_out = 1+length;
    return true;
}


// *** bytecode de-compiler ***

// this function unparses (de-compiles) a constant from bytecode.
int
unparse_const(byte code OPTIONAL, byte *bytecode_in, char **out)
{
    int32 value;
    byte *bytecode;

    bytecode = bytecode_in;

    // if the code was not specified...
    if (! code) {
        // get it from the bytecode stream
        code = *bytecode++;
    }

    // get the constant
    value = read32(bytecode);
    bytecode += sizeof(uint32);

    // if the constant is an integer...
    if (code == code_load_and_push_immediate) {
        // decompile the constant
        *out += sprintf(*out, "%ld", value);
    // otherwise, if the constant is hexadecimal...
    } else if (code == code_load_and_push_immediate_hex) {
        // decompile the constant
        *out += sprintf(*out, "0x%lx", value);
    } else {
        // the constant must be ascii
        assert(code == code_load_and_push_immediate_ascii);
        // decompile the constant
        *out += sprintf(*out, "'%c'", (char)value);
    }

    // return the number of bytecodes consumed
    return bytecode - bytecode_in;
}

int
unparse_word_const_word(char *prefix, char *suffix, byte *bytecode_in, char **out)
{
    int rv;

    if (prefix) {
        *out += sprintf(*out, "%s ", prefix);
    }
    rv = unparse_const(0, bytecode_in, out);
    if (suffix) {
        *out += sprintf(*out, " %s", suffix);
    }
    return rv;
}

// this function unparses (de-compiles) a variable from bytecode.
int
unparse_var(IN bool string, IN bool lvalue, IN int indices, byte *bytecode_in, char **out)
{
    int blen;
    int blen2;
    byte *bytecode;

    bytecode = bytecode_in;

    // if the bytecode is an array element...
    if ((*bytecode) == code_load_and_push_var_indexed || (*bytecode) == code_load_string_indexed) {
        if (string) {
            assert((*bytecode) == code_load_string_indexed);
        } else {
            assert((*bytecode) == code_load_and_push_var_indexed);
        }
        bytecode++;

        // get the index expression length(s); the variable name follows the index expression
        blen = read32(bytecode);
        bytecode += sizeof(uint32);
        if (indices == 2) {
            blen2 = read32(bytecode);
            bytecode += sizeof(uint32);
        } else {
            blen2 = 0;
        }

        // decompile the variable name and index expression
        *out += sprintf(*out, "%s%s[", bytecode+blen+blen2, string?"$":"");
        unparse_expression(0, bytecode, blen, out);
        bytecode += blen;
        if (indices == 2) {
            *out += sprintf(*out, ":");
            unparse_expression(0, bytecode, blen2, out);
            bytecode += blen2;
        }
        *out += sprintf(*out, "]");
        bytecode += strlen((char *)bytecode)+1;
    } else {
        if (string) {
            assert((*bytecode) == code_load_string);
        } else {
            assert((*bytecode) == code_load_and_push_var);
        }
        bytecode++;

        // decompile the variable name
        *out += sprintf(*out, "%s", bytecode);
        bytecode += strlen((char *)bytecode)+1;

        if (string) {
            // decompile the assignment
            *out += sprintf(*out, "$");
        }
    }

    // return the number of bytecodes consumed
    return bytecode - bytecode_in;
}

#define MAX_TEXTS  10

// revisit -- can we reduce memory???
static char texts[MAX_TEXTS][BASIC_OUTPUT_LINE_SIZE];
static int precedence[MAX_TEXTS];

// this function unparses (de-compiles) an expression from bytecode.
int
unparse_expression(int tbase, byte *bytecode_in, int length, char **out)
{
    int i;
    int n;
    int ttop;
    byte code;
    char *tout;
    bool unary;
    byte *bytecode;
    char temp[BASIC_OUTPUT_LINE_SIZE];

    ttop = tbase;
    bytecode = bytecode_in;

    // while there are more bytecodes to decompile...
    while (bytecode < bytecode_in+length) {
        code = *bytecode;
        if (code == code_comma) {
            break;
        }
        bytecode++;

        switch (code) {
            case code_load_and_push_immediate:
            case code_load_and_push_immediate_hex:
            case code_load_and_push_immediate_ascii:
                // decompile the constant
                tout = texts[ttop];
                bytecode += unparse_const(code, bytecode, &tout);
                precedence[ttop] = 100;
                ttop++;
                break;

            case code_load_and_push_var:
            case code_load_and_push_var_length:
                precedence[ttop] = 100;
                // decompile the variable
                sprintf(texts[ttop++], "%s%s", bytecode, code==code_load_and_push_var_length?"#":"");
                bytecode += strlen((char *)bytecode)+1;
                break;

            case code_load_and_push_var_indexed:
                // our index is already on the stack
                strcpy(temp, texts[ttop-1]);

                precedence[ttop-1] = 100;
                // decompile the indexed variable array element
                sprintf(texts[ttop-1], "%s[%s]", bytecode, temp);
                bytecode += strlen((char *)bytecode)+1;
                break;

            default:
                // this should be an operator!!!
                for (i = 0; i < LENGTHOF(ops); i++) {
                    if (ops[i].code == code) {
                        break;
                    }
                }
                assert(i != LENGTHOF(ops));

                unary = (code == code_logical_not || code == code_bitwise_not || code == code_unary_plus || code == code_unary_minus);

                n = 0;

                // if this is a binary operator...
                if (! unary) {
                    // if the lhs's operator's precedence is greater than the current precedence...
                    if (ops[i].precedence > precedence[ttop-2]) {
                        // we need to parenthesize the left hand side
                        n += sprintf(temp+n, "(");
                    }
                    // decompile the left hand side
                    n += sprintf(temp+n, "%s", texts[ttop-2]);
                    if (ops[i].precedence > precedence[ttop-2]) {
                        n += sprintf(temp+n, ")");
                    }
                }

                // decompile the operator
                n += sprintf(temp+n, "%s", ops[i].op);

                // if the rhs's operator's precedence is greater than the current precedence...
                if (ops[i].precedence >= precedence[ttop-1]+unary) {
                    // we need to parenthesize the right hand side
                    n += sprintf(temp+n, "(");
                }
                // decompile the right hand side
                n += sprintf(temp+n, "%s", texts[ttop-1]);
                if (ops[i].precedence >= precedence[ttop-1]+unary) {
                    n += sprintf(temp+n, ")");
                }

                // pop the operator's arguments off the stack
                if (! unary) {
                    ttop -= 2;
                } else {
                    ttop -= 1;
                }

                // and push the operator's result back on the stack
                precedence[ttop] = ops[i].precedence;
                sprintf(texts[ttop++], "%s", temp);
                break;
        }
    }

    // decompile the resulting expression
    assert(ttop == tbase+1);
    *out += sprintf(*out, "%s", texts[tbase]);

    // return the number of bytecodes consumed
    return bytecode - bytecode_in;
}

int
unparse_string(byte *bytecode_in, int length, char **out)
{
    int len;
    byte code;
    byte *bytecode;

    bytecode = bytecode_in;

    // while there are more bytecodes to decompile...
    while (bytecode < bytecode_in+length) {
        code = *bytecode;
        if (code == code_comma) {
            break;
        }
        if (bytecode > bytecode_in) {
            *out += sprintf(*out, "+");
        }

        switch (code) {
            case code_text:
                // decompile the string
                bytecode++;
                *out += sprintf(*out, "\"");
                len = sprintf(*out, "%s", bytecode);
                *out += len;
                *out += sprintf(*out, "\"");
                bytecode += len+1;
                break;

            case code_load_string:
            case code_load_string_indexed:
                bytecode += unparse_var(true, false, 2, bytecode, out);
                break;

            default:
                assert(0);
        }
    }

    return bytecode-bytecode_in;
}

// this function unparses (de-compiles) a string relation to bytecode.
int
unparse_relation(byte *bytecode_in, int length, char **out)
{
    int i;
    byte *bytecode;

    bytecode = bytecode_in;

    bytecode += unparse_string(bytecode, length, out);
    assert(*bytecode == code_comma);
    bytecode++;

    for (i = 0; i < LENGTHOF(rels); i++) {
        if (*bytecode == rels[i].code) {
            break;
        }
    }
    assert(i < LENGTHOF(rels));
    *out += sprintf(*out, " %s ", rels[i].rel);
    bytecode++;

    bytecode += unparse_string(bytecode, length, out);
    assert(*bytecode == code_comma);
    bytecode++;

    return bytecode-bytecode_in;
}

int
unparse_class(byte *bytecode, OUT bool *string)
{
    if (*bytecode == code_string) {
        *string = true;
    } else {
        assert(*bytecode == code_expression);
        *string = false;
    }
    return 1;
}

int
unparse_string_or_expression(IN bool string, byte *bytecode, int length, char **out)
{
    int rv;

    if (string) {
        // decompile the string
        rv = unparse_string(bytecode, length, out);
    } else {
        // decompile the expression
        rv = unparse_expression(0, bytecode, length, out);
    }

    return rv;
}

static
int
unparse_uart(byte *bytecode, char **out)
{
    int uart;

    // unparse the uart name
    uart = *bytecode++;
    assert(uart >= 0 && uart < MAX_UARTS);
    *out += sprintf(*out, "%s ", uart_names[uart]);

    return 1;
}

static
int
unparse_format(byte *bytecode, char **out)
{

    if (*bytecode == code_hex) {
        *out += sprintf(*out, "hex ");
    } else if (*bytecode == code_dec) {
        *out += sprintf(*out, "dec ");
    } else if (*bytecode == code_raw) {
        *out += sprintf(*out, "raw ");
    } else {
        return 0;
    }

    return 1;
}

static
int
unparse_time(byte *bytecode_in, int length, char **out)
{
    byte *bytecode;
    enum timer_unit_type timer_unit;

    bytecode = bytecode_in;

    // decompile the sleep time units
    timer_unit = (enum timer_unit_type)*(bytecode++);

    // and the sleep time expression
    bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, out);

    *out += sprintf(*out, " %s", timer_units[timer_unit].name);

    return bytecode-bytecode_in;
}

// this function unparses (de-compiles) a public statement line from bytecode,
// excluding the keyword.
void
unparse_bytecode_code(IN byte code, IN byte *bytecode_in, IN int length, OUT char *out)
{
    int i;
    int n;
    int len;
    int size;
    int pin;
    int type;
    int qual;
    bool semi;
    bool multi;
    byte parity;
    byte loopback;
    byte device;
    byte code2;
    byte *bytecode;
    bool output;
    bool string;

    semi = false;
    multi = false;
    bytecode = bytecode_in;

XXX_AGAIN_XXX:
    // if this is a multi-statement...
    if (bytecode > bytecode_in) {
        assert(multi);
        // separate with a comma
        out += sprintf(out, ", ");
        if (code != code_data) {
            assert(*bytecode == code_comma);
            bytecode++;
        }
    }

    switch (code) {
        case code_rem:
            // decompile the comment
            len = sprintf(out, "%s", bytecode);
            out += len;
            bytecode += len+1;
            break;
        case code_norem:
            *out = '\0';
            break;

        case code_on:
        case code_off:
        case code_mask:
        case code_unmask:
            // find the device type
            device = *bytecode++;
            if (device == code_timer) {
                // decompile the timer and the timer number
                bytecode += unparse_word_const_word("timer", NULL, bytecode, &out);

            } else if (device == code_uart) {
                // decompile the uart
                out += sprintf(out, "uart ");

                // and the uart name
                bytecode += unparse_uart(bytecode, &out);

                // and the uart data direction
                output = *bytecode++;
                out += sprintf(out, "%s", output?"output":"input");
            } else if (device == code_watch) {

                // this is an expression
                len = read32(bytecode);
                bytecode += sizeof(uint32);

                bytecode += unparse_expression(0, bytecode, len, &out);

                assert(*bytecode == code_comma);
                bytecode++;
            } else {
                assert(0);
            }

            // if we're enabling interrupts...
            if (code == code_on) {
                out += sprintf(out, " do ");

                len = read32(bytecode);
                bytecode += sizeof(uint32);

                // decompile the interrupt handler statement
                unparse_bytecode(bytecode, len, out);
                bytecode += len;
            }
            break;

        case code_configure:
            // find the device type
            device = *bytecode++;
            if (device == code_timer) {
                // decompile the timer and the timer number
                bytecode += unparse_word_const_word("timer", "for ", bytecode, &out);

                bytecode += unparse_time(bytecode, bytecode_in+length-bytecode, &out);

            } else if (device == code_uart) {
                // decompile the uart
                out += sprintf(out, "uart ");

                // and the uart name
                bytecode += unparse_uart(bytecode, &out);

                // decompile the baud rate and data bits
                bytecode += unparse_word_const_word("for", "baud ", bytecode, &out);
                bytecode += unparse_word_const_word(NULL, "data ", bytecode, &out);

                // find the uart protocol and optional loopback specifier
                parity = *bytecode++;
                loopback = *bytecode++;

                // and decompile it
                out += sprintf(out, "%s parity%s", parity==0?"even":(parity==1?"odd":"no"), loopback?" loopback":"");
            } else {
                assert(0);
            }
            break;

        case code_assert:
            // decompile the assertion expression
            bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
            break;

        case code_read:
            // decompile the variable
            bytecode += unparse_var(false, true, 1, bytecode, &out);

            multi = true;
            break;

        case code_data:
            // decompile the constant
            assert(code == code_data);
            bytecode += unparse_const(0, bytecode, &out);

            multi = true;
            break;

        case code_dim:
            bytecode += unparse_class(bytecode, &string);

            // decompile the variable
            bytecode += unparse_var(string, true, 1, bytecode, &out);

            size = *bytecode++;
            code2 = *bytecode++;

            // decompile the "as"
            if (! string) {
                if (size != sizeof(uint32) || (code2 != code_ram) && (code2 != code_absolute)) {
                    out += sprintf(out, " as ");

                    // decompile the size specifier
                    if (size == sizeof(byte)) {
                        assert(code2 == code_ram || code2 == code_absolute);
                        out += sprintf(out, "byte");
                    } else if (size == sizeof(short)) {
                        assert(code2 == code_ram || code2 == code_absolute);
                        out += sprintf(out, "short");
                    } else {
                        assert(size == sizeof(uint32));
                    }
                }
            }

            // decompile the type specifier
            if (code2 == code_ram) {
                // NULL
            } else if (code2 == code_flash) {
                out += sprintf(out, "flash");
            } else if (code2 == code_pin) {
                out += sprintf(out, "pin ");

                pin = *bytecode++;
                type = *bytecode++;
                qual = *bytecode++;

                // decompile the pin name
                assert(pin >= 0 && pin < pin_last);
                out += sprintf(out, "%s ", pins[pin].name);

                out += sprintf(out, "for ");

                // decompile the pin usage
                assert(type >= 0 && type < pin_type_last);
                out += sprintf(out, "%s", pin_type_names[type]);

                // decompile the pin qualifier(s)
                for (i = 0; i < pin_qual_last; i++) {
                    if (qual & (1<<i)) {
                        out += sprintf(out, " %s", pin_qual_names[i]);
                    }
                }
            } else if (code2 == code_absolute) {
                out += sprintf(out, " at address ");
                bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
            } else {
                assert(code2 == code_nodeid);

                bytecode += unparse_word_const_word("remote on nodeid", NULL, bytecode, &out);
            }

            multi = true;
            break;

        case code_let:
        case code_nolet:
            bytecode += unparse_class(bytecode, &string);

            // decompile the variable
            bytecode += unparse_var(string, true, string?0:1, bytecode, &out);

            // decompile the assignment
            out += sprintf(out, " = ");

            // decompile the string or expression
            bytecode += unparse_string_or_expression(string, bytecode, bytecode_in+length-bytecode, &out);

            multi = true;
            break;

        case code_input:
            bytecode += unparse_format(bytecode, &out);

            bytecode += unparse_class(bytecode, &string);

            // decompile the variable
            bytecode += unparse_var(string, true, string?0:1, bytecode, &out);

            multi = true;
            break;

        case code_print:
        case code_vprint:
            if (bytecode == bytecode_in) {
                // if this is an vprint...
                if (code == code_vprint) {
                    bytecode += unparse_class(bytecode, &string);

                    // decompile the variable
                    bytecode += unparse_var(string, true, string?0:1, bytecode, &out);

                    // decompile the assignment
                    out += sprintf(out, " = ");
                }

                // if we're not printing a newline...
                if (*bytecode == code_semi) {
                    bytecode++;
                    semi = true;
                }
            }

            bytecode += unparse_format(bytecode, &out);

            bytecode += unparse_class(bytecode, &string);

            // decompile the string or expression
            bytecode += unparse_string_or_expression(string, bytecode, bytecode_in+length-bytecode, &out);

            if (bytecode == bytecode_in+length) {
                if (semi) {
                    out += sprintf(out, ";");
                }
            }
            multi = true;
            break;

        case code_uart:
        case code_qspi:
        case code_i2c:
            if (bytecode == bytecode_in) {
                if (code == code_uart) {
                    // and the uart name
                    bytecode += unparse_uart(bytecode, &out);
                }

                if (code == code_i2c && *bytecode == code_device_start) {
                    bytecode++;
                    out += sprintf(out, "start ");
                    // unparse the address
                    bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
                    break;
                } else if (code == code_i2c && *bytecode == code_device_stop) {
                    bytecode++;
                    out += sprintf(out, "stop");
                    break;
                } else

                if ((code == code_uart || code == code_i2c) && *bytecode == code_device_read) {
                    bytecode++;
                    out += sprintf(out, "read ");
                } else if (code == code_uart || code == code_i2c) {
                    assert(*bytecode == code_device_write);
                    bytecode++;
                    out += sprintf(out, "write ");
                }
            }

            // decompile the variable
            bytecode += unparse_var(false, true, 1, bytecode, &out);

            multi = true;
            break;

        case code_if:
        case code_elseif:
        case code_while:
        case code_until:
            bytecode += unparse_class(bytecode, &string);

            if (string) {
                // decompile the string relation
                bytecode += unparse_relation(bytecode, bytecode_in+length-bytecode, &out);
            } else {
                // decompile the conditional expression
                bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
            }
            // decompile the "then" or "do"
            if (code == code_if || code == code_elseif) {
                out += sprintf(out, " then");
            } else if (code == code_while) {
                out += sprintf(out, " do");
            }
            break;

        case code_else:
        case code_endif:
        case code_endwhile:
        case code_do:
            // nothing to do here
            break;

        case code_break:
        case code_continue:
            // if the user specified a break/continue level...
            if (bytecode < bytecode_in+length) {
                bytecode += unparse_const(0, bytecode, &out);
            }
            break;

        case code_for:
            assert(bytecode < bytecode_in+length);

            // decompile the for loop variable
            bytecode += unparse_var(false, true, 1, bytecode, &out);

            // decompile the assignment
            out += sprintf(out, " = ");

            // decompile the initial value expression
            bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);

            // decompile the "to"
            assert(*bytecode == code_comma);
            bytecode++;
            out += sprintf(out, " to ");

            // decompile the final value expression
            bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);

            // if there is a "step" keyword...
            if (bytecode_in+length > bytecode) {
                assert(*bytecode == code_comma);
                bytecode++;
                // decompile the "step" and the step value expression
                out += sprintf(out, " step ");
                bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
            }
            break;

        case code_next:
            // nothing to do here
            break;

        case code_label:
        case code_restore:
        case code_gosub:
        case code_sub:
            // decompile the label/sub name
            len = sprintf(out, "%s", bytecode);
            out += len;
            bytecode += len+1;
            n = 0;

            if ((code == code_sub) || (code == code_gosub)) {
                // decompile the parameter names separating parameter names with a comma
                while (bytecode < bytecode_in+length) {
                    if (n) {
                        assert(*bytecode == code_comma);
                        bytecode++;
                        *(out++) = ',';
                    }
                    *(out++) = ' ';
                    n = 1;

                    bytecode += unparse_expression(0, bytecode, bytecode_in+length-bytecode, &out);
                }
            }
            break;

        case code_return:
        case code_endsub:
            // nothing to do here
            break;

        case code_sleep:
            bytecode += unparse_time(bytecode, length, &out);
            break;

        case code_halt:
        case code_stop:
        case code_end:
            // nothing to do here
            break;

        default:
            assert(0);
            break;
    }

    if (bytecode < bytecode_in+length && multi) {
        goto XXX_AGAIN_XXX;
    }
    assert(bytecode == bytecode_in+length);
}

// this function unparses (de-compiles) a statement line from bytecode.
void
unparse_bytecode(IN byte *bytecode, IN int length, OUT char *text)
{
    int i;
    char *out;
    byte code;

    out = text;

    code = *bytecode;

    // find the bytecode keyword
    for (i = 0; i < LENGTHOF(keywords); i++) {
        if (keywords[i].code == code) {
            break;
        }
    }
    if (i == LENGTHOF(keywords)) {
        unparse2_bytecode(bytecode, length, out);
        return;
    }

    // decompile the bytecode keyword
    if (keywords[i].keyword[0]) {
        out += sprintf(out, keywords[i].keyword);
        out += sprintf(out, " ");
    }

    assert(length);
    unparse_bytecode_code(code, bytecode+1, length-1, out);
}

