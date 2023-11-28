// *** basic.c ********************************************************
// this file implements the stickos command interpreter.

// Copyright (c) CPUStick.com, 2008-2023.  All rights reserved.
// Patent U.S. 8,117,587.

#include "main.h"

#if STICK_GUEST
byte FLASH_CODE1_PAGE[BASIC_LARGE_PAGE_SIZE];
byte FLASH_CODE2_PAGE[BASIC_LARGE_PAGE_SIZE];
byte FLASH_STORE_PAGES[BASIC_STORES][BASIC_LARGE_PAGE_SIZE];
byte FLASH_CATALOG_PAGE[BASIC_SMALL_PAGE_SIZE];
byte FLASH_PARAM1_PAGE[BASIC_SMALL_PAGE_SIZE];
byte FLASH_PARAM2_PAGE[BASIC_SMALL_PAGE_SIZE];
#endif

byte RAM_CODE_PAGE[BASIC_RAM_PAGE_SIZE];
byte RAM_VARIABLE_PAGE[BASIC_RAM_PAGE_SIZE];

byte *start_of_dynamic;
byte *end_of_dynamic;

struct timer_unit const timer_units[] = {
    "us", -1000/ticks_per_msec,
    "ms", ticks_per_msec,
    "s", ticks_per_msec*1000,
};

enum cmdcode {
    command_analog,  // nnn
    command_auto,  // [nnn]
    command_baud,  // nnn
    command_clear,  // [flash]
    command_cls,
    command_cont,  // [nnn]
    command_delete,  // ([nnn] [-] [nnn]|<subname>)
    command_dir,
    command_edit, // nnn
    command_list,  // ([nnn] [-] [nnn]|<subname>)
    command_load,  // <name>
    command_memory,
    command_new,
    command_pins,  // <pinname> <pinnumber>
    command_profile,  // ([nnn] [-] [nnn]|<subname>)
    command_purge, // <name>
    command_renumber,
    command_run,  // [nnn]
    command_save,  // [<name>]
    command_servo,  // [nnn]
    command_subs,
    command_undo,
    num_commands
};

static
const char * const commands[] = {
    "analog",
    "auto",
    "baud",
    "clear",
    "cls",
    "cont",
    "delete",
    "dir",
    "edit",
    "list",
    "load",
    "memory",
    "new",
    "pins",
    "profile",
    "purge",
    "renumber",
    "run",
    "save",
    "servo",
    "subs",
    "undo",
};

// revisit -- merge this with run.c/basic.c/parse.c???
bool
basic_const(IN OUT char **text, OUT int *value_out)
{
    int value;

    if (! isdigit((*text)[0])) {
        return false;
    }

    // parse constant value and advance *text past constant
    value = 0;
    while (isdigit((*text)[0])) {
        value = value*10 + (*text)[0] - '0';
        (*text)++;
    }

    *value_out = value;
    parse_trim(text);
    return true;
}

struct mode {
    const char * const   name;
    bool * const         var;
    const enum flash_var flash_var;
};

// define command controllable variables each is backed by a global variable and/or a flash variable.
static const struct mode modes[] = {
    // name         var                flash_var
    { "autorun",    NULL,              FLASH_AUTORUN      },
    { "echo",       &terminal_echo,    FLASH_LAST         },
    { "indent",     &code_indent,      FLASH_LAST         },
    { "numbers",    &code_numbers,     FLASH_LAST         },
    { "prompt",     &main_prompt,      FLASH_LAST         },
    { "step",       &run_step,         FLASH_LAST         },
    { "trace",      &run_trace,        FLASH_LAST         },
    { "watchsmart", &watch_mode_smart, FLASH_WATCH_SMART  },
    { NULL,         NULL,              FLASH_LAST         }  // terminator
};

// this function implements the stickos command interpreter.
void
basic_run(char *text_in)
{
    int i;
    int cmd;
    int len;
    bool boo;
    char *text;
    int pin;
    int length;
    int number1;
    int number2;
    bool in_library;
    struct line *line;
    int syntax_error;
    byte bytecode[BASIC_BYTECODE_SIZE];
    static uint8 empty;
    const struct mode *mode;

    if (run_step && ! *text_in) {
        text = "cont";
    } else {
        text = text_in;
    }

    parse_trim(&text);

    // find mode by name
    for (mode = modes; mode->name; mode++) {
        if (parse_word(&text, mode->name)) {
            if (*text) {
                // parse the new value: <on|off>
                if (parse_word(&text, "on")) {
                    boo = true;
                } else if (parse_word(&text, "off")) {
                    boo = false;
                } else {
                    goto XXX_ERROR_XXX;
                }
                if (*text) {
                    goto XXX_ERROR_XXX;
                }
                if (mode->flash_var != FLASH_LAST) {
                    var_set_flash(mode->flash_var, boo);
                }
                if (mode->var) {
                    *mode->var = boo;
                }
            } else {
                if (mode->var) {
                    boo = *mode->var;
                } else {
                    boo = var_get_flash(mode->flash_var) == 1;
                }
                printf("%s\n", boo ? "on" : "off");
            }
            return;
        }
    }

    for (cmd = 0; cmd < LENGTHOF(commands); cmd++) {
        len = strlen(commands[cmd]);
        if (! strncmp(text, commands[cmd], len)) {
            break;
        }
    }
    // N.B. cmd == LENGTHOF(commands) OK below!!!

    if (cmd != LENGTHOF(commands)) {
        text += len;
        parse_trim(&text);
    }

#if STICK_GUEST
    // let timer ticks arrive; align ticks on run
    timer_ticks(cmd == command_run);
#endif

    number1 = 0;
    number2 = 0;

    switch (cmd) {
        case command_analog:
        case command_baud:
        case command_servo:
            if (*text) {
                if (! basic_const(&text, &number1) || number1 == -1) {
                    goto XXX_ERROR_XXX;
                }
                if (*text) {
                    goto XXX_ERROR_XXX;
                }
                if (cmd == command_analog) {
                    if (number1 < 1000 || number1 > 5000) {
                        goto XXX_ERROR_XXX;
                    }
                    var_set_flash(FLASH_ANALOG, number1);
                    pin_analog = number1;
                } else if (cmd == command_baud) {
                    var_set_flash(FLASH_BAUD, number1);
                    // N.B. used on next reboot
                } else {
                    if (number1 < 20 || number1 > 500) {
                        goto XXX_ERROR_XXX;
                    }
                    var_set_flash(FLASH_SERVO, number1);
                    // N.B. used on next reboot
                }
            } else {
                printf("%d\n", (int)(cmd == command_analog ? pin_analog : (cmd == command_baud ? serial_baudrate : servo_hz)));
            }
            break;

        case command_pins:
            if (*text) {
                // find the assignment name
                for (i = 0; i < pin_assignment_max; i++) {
                    if (parse_wordn(&text, pin_assignment_names[i])) {
                        break;
                    }
                }

                if (*text) {
                    // find the pin name
                    for (pin = 0; pin < pin_last; pin++) {
                        if (parse_wordn(&text, pins[pin].name)) {
                            break;
                        }
                    }
                    if (pin == pin_last || *text) {
                        goto XXX_ERROR_XXX;
                    }
                    var_set_flash(FLASH_ASSIGNMENTS_BEGIN+i, pin);
                    pin_assign(i, pin);
                } else {
                    assert(pin_assignments[i] < pin_last);
                    printf("%s\n", pins[pin_assignments[i]].name);
                }

            } else {
                for (i = 0; i < pin_assignment_max; i++) {
                    assert(pin_assignments[i] < pin_last);
                    printf("%s %s\n", pin_assignment_names[i], pins[pin_assignments[i]].name);
                }
            }
            break;

        case command_auto:
            if (! basic_const(&text, &number1)) {
                number1 = 10;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }
#if ! STICK_GUEST
            main_auto = number1;
#endif
            break;

        case command_clear:
            boo = false;
            if (parse_word(&text, "flash")) {
                boo = true;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            run_clear(boo);
            break;

        case command_cls:
            if (*text) {
                goto XXX_ERROR_XXX;
            }
            printf("%c[2J\n", '\033');
            break;

        case command_cont:
        case command_run:
            if (*text) {
                (void)basic_const(&text, &number1);
                if (*text) {
                    goto XXX_ERROR_XXX;
                }
            }

#if ! STICK_GUEST
            terminal_command_discard(true);
            if (main_command) {
                main_command = NULL;
                terminal_command_ack(false);
            }
#endif

            run(cmd == command_cont, number1);

#if ! STICK_GUEST
            terminal_command_discard(false);
#endif
            break;

        case command_delete:
        case command_list:
        case command_profile:
            in_library = false;
            if (*text) {
                boo = basic_const(&text, &number1);
                number2 = number1;
                if (*text) {
                    boo |= parse_char(&text, '-');
                    number2 = 0;
                    boo |= basic_const(&text, &number2);
                }
                if (! boo) {
                    line = code_line(code_sub, (byte *)text, false, cmd==command_list, &in_library);
                    if (line == NULL) {
                        goto XXX_ERROR_XXX;
                    }
                    number1 = line->line_number;
                    text += strlen(text);
                    number2 = 0x7fffffff;  // endsub
                }
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            if (cmd == command_delete) {
                code_delete(number1, number2);
            } else {
                code_list(cmd == command_profile, number1, number2, in_library);
            }
            break;

        case command_dir:
            if (*text) {
                goto XXX_ERROR_XXX;
            }
            code_dir();
            break;

        case command_edit:
            (void)basic_const(&text, &number1);
            if (! number1 || *text) {
                goto XXX_ERROR_XXX;
            }
            code_edit(number1);
            break;

        case command_load:
        case command_purge:
        case command_save:
            if (cmd != command_save && ! *text) {
                goto XXX_ERROR_XXX;
            }
            if (cmd == command_load) {
                code_load(text);
            } else if (cmd == command_purge) {
                code_purge(text);
            } else if (cmd == command_save) {
                code_store(text);
            } else {
                assert(0);
            }
            break;

        case command_memory:
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            code_mem();
            var_mem();
            break;

        case command_new:
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            code_new();
            break;

        case command_renumber:
            number1 = 10;
            (void)basic_const(&text, &number1);
            if (! number1 || *text) {
                goto XXX_ERROR_XXX;
            }

            code_save(false, number1);
            break;

        case command_subs:
            if (*text) {
                goto XXX_ERROR_XXX;
            }
            line = code_line(code_sub, (const byte *)"", true, true, NULL);
            assert(! line);
            break;

        case command_undo:
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            code_undo();
            break;

        case LENGTHOF(commands):
            // if the line begins with a line number
            if (isdigit(*text)) {
                (void)basic_const(&text, &number1);
                if (! number1) {
#if ! STICK_GUEST
                    main_auto = 0;
#endif
                    goto XXX_ERROR_XXX;
                }
                if (*text) {
                    code_insert(number1, text, text-text_in);
                    empty = 0;
                } else {
                    code_insert(number1, NULL, text-text_in);
#if ! STICK_GUEST
                    if (++empty > 1) {
                        main_auto = 0;
                    }
#endif
                }
            } else {
#if ! STICK_GUEST
                main_auto = 0;
#endif
                if (*text) {
                    // *** interactive debugger ***
                    // see if this might be a basic line executing directly
                    if (parse_line(text, &length, bytecode, &syntax_error)) {
                        // if this line is not allowed in immediate mode...
                        if (*bytecode == code_input || *bytecode == code_i2c || *bytecode == code_uart || *bytecode == code_qspi) {
                            printf("not allowed\n");
                        } else {
                            // run the bytecode
                            run_bytecode(true, bytecode, length);
                        }
                    } else {
                        terminal_command_error(text-text_in + syntax_error);
                    }
                }
            }
            break;

        default:
            assert(0);
            break;
    }
    return;

XXX_ERROR_XXX:
    terminal_command_error(text-text_in);
}

// this function initializes the basic module.
void
basic_initialize(void)
{
#if ! STICK_GUEST
    start_of_dynamic = FLASH_CODE1_PAGE;
    ASSERT(end_of_static < start_of_dynamic);

    end_of_dynamic = FLASH_PARAM2_PAGE+BASIC_SMALL_PAGE_SIZE;
    ASSERT((uint32)end_of_dynamic <= FLASH_START+FLASH_BYTES);

#endif

    assert(LENGTHOF(commands) == num_commands);
    code_initialize();
    var_initialize();
    run_initialize();
}

