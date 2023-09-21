// *** basic0.c *******************************************************
// this file implements private extensions to the stickos command
// interpreter.

#include "main.h"

enum cmdcode {
#if STICK_GUEST
    command_exit,
#endif
    command_connect,  // nnn
#if STICK_GUEST || SODEBUG
    command_demo,
#endif
    command_help,
    command_nodeid,  // nnn
    command_reset,
#if UPGRADE
    command_upgrade,
#endif
    command_uptime,
#if ZIGFLEA
    command_zigflea,
#endif
#if SCOPE && ! STICK_GUEST
    command_scope,
    command_wave,
#endif
    command_hostname,
    command_dummy
};

static
const char * const commands[] = {
#if STICK_GUEST
    "exit",
#endif
    "connect",
#if STICK_GUEST || SODEBUG
    "demo",
#endif
    "help",
    "nodeid",
    "reset",
#if UPGRADE
    "upgrade",
#endif
    "uptime",
#if ZIGFLEA
    "zigflea",
#endif
#if SCOPE && ! STICK_GUEST
    "scope",
    "wave",
#endif
    "hostname",
};


// *** help ***********************************************************

const char * const help_about =
#if defined(__32MX250F128B__)
"Welcome to StickOS for Microchip PIC32MX2-F128B v" VERSION "!\n"
#elif defined(__32MK0512GPK064__)
"Welcome to StickOS for Microchip PIC32MK0512GPK v" VERSION "!\n"
#elif defined(__32MK0512MCM064__)
"Welcome to StickOS for Microchip PIC32MK0512MCM v" VERSION "!\n"
#elif defined(__32MX440F512H__)
"Welcome to StickOS for Microchip PIC32MXx-F512H v" VERSION "!\n"
#elif defined(__32MX460F512L__)
"Welcome to StickOS for Microchip PIC32MXx-F512L v" VERSION "!\n"
#elif defined(__32MX795F512H__)
"Welcome to StickOS for Microchip PIC32MX7-F512H v" VERSION "!\n"
#elif defined(__32MX795F512L__)
"Welcome to StickOS for Microchip PIC32MX7-F512L v" VERSION "!\n"
#else
#error
#endif
"Copyright (c) 2008-2023; all rights reserved.  Patent U.S. 8,117,587.\n"
"https://github.com/rtestardi/StickOS2\n"
"rtestardi at live.com\n"
;


/*
to change help:

1. change the help inside the "dead" part of the #if (between GENERATE-HELP-BEGIN and GENERATE-HELP-END)
2. download the Win32 version of StickOS from https://rtestardi.github.io/StickOS/downloads.htm
3. in a shell window, cd to the cpustick directory
4. run: "StickOS.exe help <basic0.c >help.c"
4a. or in powershell run: gc basic0.c | c:\temp\StickOS.v1.82.exe help | out-file -encoding ascii help.c
5. rebuild
*/
#if 1
#include "help.c"
#else
// GENERATE_HELP_BEGIN

#if ! SODEBUG || STICK_GUEST
const char * const help_general =
"for more information:\n"
"  help about\n"
"  help commands\n"
"  help modes\n"
"  help statements\n"
"  help blocks\n"
"  help devices\n"
"  help expressions\n"
"  help strings\n"
"  help variables\n"
"  help pins\n"
"  help zigflea\n"
"\n"
"see also:\n"
"  https://rtestardi.github.io/StickOS/\n"
;

static const char * const help_commands =
"<Ctrl-C>                      -- stop program\n"
"auto <line>                   -- automatically number program lines\n"
"clear [flash]                 -- clear ram [and flash] variables\n"
"cls                           -- clear terminal screen\n"
"cont [<line>]                 -- continue program from stop\n"
"delete ([<line>][-][<line>]|<subname>) -- delete program lines\n"
"dir                           -- list saved programs\n"
"edit <line>                   -- edit program line\n"
"help [<topic>]                -- online help\n"
"list ([<line>][-][<line>]|<subname>) -- list program lines\n"
"load <name>                   -- load saved program\n"
"memory                        -- print memory usage\n"
"new                           -- erase code ram and flash memories\n"
"profile ([<line>][-][<line>]|<subname>) -- display profile info\n"
"purge <name>                  -- purge saved program\n"
"renumber [<line>]             -- renumber program lines (and save)\n"
"reset                         -- reset the MCU!\n"
"run [<line>]                  -- run program\n"
"save [<name>|library]         -- save code ram to flash memory\n"
"subs                          -- list sub names\n"
"undo                          -- undo code changes since last save\n"
#if UPGRADE
"upgrade                       -- upgrade StickOS firmware!\n"
#endif
"uptime                        -- print time since last reset\n"
"\n"
"for more information:\n"
"  help modes\n"
;

static const char * const help_modes =
"analog [<millivolts>]             -- set/display analog voltage scale\n"
"baud [<rate>]                     -- set/display uart console baud rate\n"
"autorun [on|off]                  -- autorun mode (on reset)\n"
"echo [on|off]                     -- terminal echo mode\n"
"indent [on|off]                   -- listing indent mode\n"
"nodeid [<nodeid>|none]            -- set/display zigflea nodeid\n"
"numbers [on|off]                  -- listing line numbers mode\n"
"pins [<assign> [<pinname>|none]]  -- set/display StickOS pin assignments\n"
"prompt [on|off]                   -- terminal prompt mode\n"
"servo [<Hz>]                      -- set/display servo Hz (on reset)\n"
"step [on|off]                     -- debugger single-step mode\n"
"trace [on|off]                    -- debugger trace mode\n"
"watchsmart [on|off]               -- low-overhead watchpoint mode\n"
"\n"
"pin assignments:\n"
"  heartbeat  safemode*\n"
"  qspi_cs*  zigflea_rst*  zigflea_attn*  zigflea_rxtxen\n"
"\n"
"for more information:\n"
"  help pins\n"
;

static const char * const help_statements =
"<line>                                 -- delete program line from code ram\n"
"<line> <statement>  // comment         -- enter program line into code ram\n"
"\n"
"<variable>[$] = <expression> [, ...]   -- assign variable\n"
"? [dec|hex|raw] <expression> [, ...] [;] -- print results\n"
"assert <expression>                    -- break if expression is false\n"
"data <n> [, ...]                       -- read-only data\n"
"dim <variable>[$][[n]] [as ...] [, ...] -- dimension variables\n"
"end                                    -- end program\n"
"halt                                   -- loop forever\n"
"input [dec|hex|raw] <variable>[$] [, ...] -- input data\n"
"label <label>                          -- read/data label\n"
"let <variable>[$] = <expression> [, ...] -- assign variable\n"
"print [dec|hex|raw] <expression> [, ...] [;] -- print results\n"
"read <variable> [, ...]                -- read read-only data into variables\n"
"rem <remark>                           -- remark\n"
"restore [<label>]                      -- restore read-only data pointer\n"
"sleep <expression> (s|ms|us)           -- delay program execution\n"
"stop                                   -- insert breakpoint in code\n"
"vprint <variable>[$] = [dec|hex|raw] <expression> [, ...] -- print to variable\n"
"\n"
"for more information:\n"
"  help blocks\n"
"  help devices\n"
"  help expressions\n"
"  help strings\n"
"  help variables\n"
;

static const char * const help_blocks =
"if <expression> then\n"
"[elseif <expression> then]\n"
"[else]\n"
"endif\n"
"\n"
"for <variable> = <expression> to <expression> [step <expression>]\n"
"  [(break|continue) [n]]\n"
"next\n"
"\n"
"while <expression> do\n"
"  [(break|continue) [n]]\n"
"endwhile\n"
"\n"
"do\n"
"  [(break|continue) [n]]\n"
"until <expression>\n"
"\n"
"gosub <subname> [<expression>, ...]\n"
"\n"
"sub <subname> [<param>, ...]\n"
"  [return]\n"
"endsub\n"
;

static const char * const help_devices =
"timers:\n"
"  configure timer <n> for <n> (s|ms|us)\n"
"  on timer <n> do <statement>                -- on timer execute statement\n"
"  off timer <n>                              -- disable timer interrupt\n"
"  mask timer <n>                             -- mask/hold timer interrupt\n"
"  unmask timer <n>                           -- unmask timer interrupt\n"
"\n"
"uarts:\n"
"  configure uart <n> for <n> baud <n> data (even|odd|no) parity [loopback]\n"
"  on uart <n> (input|output) do <statement>  -- on uart execute statement\n"
"  off uart <n> (input|output)                -- disable uart interrupt\n"
"  mask uart <n> (input|output)               -- mask/hold uart interrupt\n"
"  unmask uart <n> (input|output)             -- unmask uart interrupt\n"
"  uart <n> (read|write) <variable> [, ...]   -- perform uart I/O\n"
"\n"
"i2c:\n"
"  i2c (start <addr>|(read|write) <variable> [, ...]|stop) -- master i2c I/O\n"
"\n"
"qspi:\n"
"  qspi <variable> [, ...]                    -- master qspi I/O\n"
"\n"
"watchpoints:\n"
"  on <expression> do <statement>             -- on expr execute statement\n"
"  off <expression>                           -- disable expr watchpoint\n"
"  mask <expression>                          -- mask/hold expr watchpoint\n"
"  unmask <expression>                        -- unmask expr watchpoint\n"
;

static const char * const help_expressions =
"the following operators are supported as in C,\n"
"in order of decreasing precedence:\n"
"  <n>                       -- decimal constant\n"
"  0x<n>                     -- hexadecimal constant\n"
"  'c'                       -- character constant\n"
"  <variable>                -- simple variable\n"
"  <variable>[<expression>]  -- array variable element\n"
"  <variable>#               -- length of array or string\n"
"  (   )                     -- grouping\n"
"  !   ~                     -- logical not, bitwise not\n"
"  *   /   %                 -- times, divide, mod\n"
"  +   -                     -- plus, minus\n"
"  >>  <<                    -- shift right, left\n"
"  <=  <  >=  >              -- inequalities\n"
"  ==  !=                    -- equal, not equal\n"
"  |   ^   &                 -- bitwise or, xor, and\n"
"  ||  ^^  &&                -- logical or, xor, and\n"
"for more information:\n"
"  help variables\n"
;

static const char * const help_strings =
"v$ is a nul-terminated view into a byte array v[]\n"
"\n"
"string statements:\n"
"  dim, input, let, print, vprint\n"
"  if <expression> <relation> <expression> then\n"
"  while <expression> <relation> <expression> do\n"
"  until <expression> <relation> <expression> do\n"
"\n"
"string expressions:\n"
"  \"literal\"                      -- literal string\n"
"  <variable>$                    -- variable string\n"
"  <variable>$[<start>:<length>]  -- variable substring\n"
"  +                              -- concatenates strings\n"
"\n"
"string relations:\n"
"  <=  <  >=  >                   -- inequalities\n"
"  ==  !=                         -- equal, not equal\n"
"  ~  !~                          -- contains, does not contain\n"
"for more information:\n"
"  help variables\n"
;

static const char * const help_variables =
"all variables must be dimensioned!\n"
"variables dimensioned in a sub are local to that sub\n"
"simple variables are passed to sub params by reference; otherwise, by value\n"
"array variable indices start at 0\n"
"v is the same as v[0], except for input/print/i2c/qspi/uart statements\n"
"\n"
"ram variables:\n"
"  dim <var>[$][[n]]\n"
"  dim <var>[[n]] as (byte|short)\n"
"\n"
"absolute variables:\n"
"  dim <var>[[n]] [as (byte|short)] at address <addr>\n"
"\n"
"flash parameter variables:\n"
"  dim <varflash>[[n]] as flash\n"
"\n"
"pin alias variables:\n"
"  dim <varpin> as pin <pinname> for (digital|analog|servo|frequency|uart) \\\n"
"                                      (input|output) \\\n"
"                                      [debounced] [inverted] [open_drain]\n"
"\n"
"system variables (read-only):\n"
"  analog  getchar"
"  msecs  nodeid\n"
"  random  seconds  ticks  ticks_per_msec\n"
"\n"
"for more information:\n"
"  help pins\n"
;

static const char * const help_pins =
"pin names:\n"
#if defined(__32MX250F128B__)
"  0/8     1/9     2/10    3/11    4/12    5/13    6/14    7/15\n"
"  ------- ------- ------- ------- ------- ------- ------- --------+\n"
"  ra0     ra1                     ra4                             | PORT A\n"
"                                                                  |      A+8\n"
"  rb0     rb1     rb2     rb3     rb4     rb5             rb7     | PORT B\n"
"  rb8     rb9                             rb13    rb14    rb15    |      B+8\n"
"\n"
"all pins support general purpose digital input/output\n"
"ra[0-1],rb[0-3,13-15] = potential analog input pins (mV)\n"
"rb[2,5,7,9,13] = potential analog output (PWM) pins (mV)\n"
"rb[2,5,7,9,13] = potential servo output (PWM) pins (us)\n"
"rb[2,5,7,9,13] = potential frequency output pins (Hz)\n"
"rb8 (u2) = potential uart input pins (received byte)\n"
"rb14 (u2) = potential uart output pins (transmit byte)\n"
"\n"
"i2c: rb8=SCL1, rb9=SDA1\n"  // XXX -- why don't other MCUs show these?
"qspi: ra4=SDI2, rb13=SDO2, rb14=SS2, rb15=SCK2\n"  // XXX -- why don't other MCUs show these?
#elif defined(__32MK0512GPK064__) || defined(__32MK0512MCM064__)
"     ICSP        GND 3V3 5V  a0  a1  a2  a3  a4  a5  a6  a7  a8\n"
"  U                                                          e2\n"
"  S                                                   GND SCOPE\n"
"  B                                                              B\n"
"                                                                 N\n"
"                                                                 C\n"
"                                                      GND  WAVE\n"
"                                                             e3\n"
"     s1  e1      GND 3V3 5V  b0  b1  b2  b3  b4  b5  b6  b7  b8\n"
"\n"
"[ab][0-8],e[1-3],s1 support general purpose digital input/output\n"
"(digital inputs b[0-8] are 5V tolerant)\n"
"a[1-8] = potential analog input pins (mV)\n"
"a[3-8] = potential analog output (PWM) pins (mV)\n"
"a[3-8] = potential servo output (PWM) pins (us)\n"
"a[3-8] = potential frequency output pins (Hz)\n"
"b6 (u1), b4 (u2) = potential uart input pins (received byte)\n"
"b8 (u1), b7 (u2) = potential uart output pins (transmit byte)\n"
"i2c: a6=SDA, a7=SCL\n"  // XXX -- why don't other MCUs show these?
"qspi: b0=SDO, b1=SDI, b2=SCK, b3=SS\n"  // XXX -- why don't other MCUs show these?
#else
"  0/8     1/9     2/10    3/11    4/12    5/13    6/14    7/15\n"
"  ------- ------- ------- ------- ------- ------- ------- --------+\n"
"  an0     an1     an2     an3     an4     an5     an6     an7     | PORT B\n"
"  an8     an9     an10    an11    an12    an13    an14    an15    |      B+8\n"
"          rc1     rc2     rc3     rc4                             | PORT C\n"
"                                  rc12    rc13    rc14    rc15    |      C+8\n"
"  rd0     rd1     rd2     rd3     rd4     rd5     rd6     rd7     | PORT D\n"
"  rd8     rd9     rd10    rd11    rd12    rd13    rd14    rd15    |      D+8\n"
"  re0     re1     re2     re3     re4     re5     re6     re7     | PORT E\n"
"  re8     re9                                                     |      E+8\n"
"  rf0     rf1     rf2     rf3     rf4     rf5                     | PORT F\n"
"  rf8                             rf12    rf13                    |      F+8\n"
"  rg0     rg1     rg2     rg3                     rg6     rg7     | PORT G\n"
"  rg8     rg9                     rg12    rg13    rg14    rg15    |      G+8\n"
"\n"
"all pins support general purpose digital input/output\n"
"an? = potential analog input pins (mV)\n"
"rd[0-4] = potential analog output (PWM) pins (mV)\n"
"rd[0-4] = potential servo output (PWM) pins (us)\n"
"rd[0-4] = potential frequency output pins (Hz)\n"
"rf4 (u2) = potential uart input pins (received byte)\n"
"rf5 (u2) = potential uart output pins (transmit byte)\n"
#endif
;

static const char * const help_zigflea =
"connect <nodeid>              -- connect to MCU <nodeid> via zigflea\n"
"<Ctrl-D>                      -- disconnect from zigflea\n"
"\n"
"remote node variables:\n"
"  dim <varremote>[[n]] as remote on nodeid <nodeid>\n"
"\n"
"zigflea cable:\n"
"  MCU                  MC1320X\n"
"  -------------        -----------\n"
// REVISIT -- implement zigflea on MRF24J40
"  sck1                 spiclk\n"
"  sdi1                 miso\n"
"  sdo1                 mosi\n"
"  int1                 irq*\n"
"  pins qspi_cs*        ce*\n"
"  pins zigflea_rst*    rst*\n"
"  pins zigflea_rxtxen  rxtxen\n"
"  vss                  vss\n"
"  vdd                  vdd\n"
;
#endif

// GENERATE_HELP_END
#endif

void
basic0_help(IN char *text_in)
{
    char *p;
    char *text;
    byte line[BASIC_OUTPUT_LINE_SIZE];
    char line2[BASIC_OUTPUT_LINE_SIZE];

    text = text_in;

    // while there is more help to print...
    while (*text) {
        // print the next line of help
        p = strchr(text, '\n');
        assert(p);
        assert(p-text < BASIC_OUTPUT_LINE_SIZE);
        memcpy(line, text, p-text);
        line[p-text] = '\0';
        text_expand(line, line2);
        printf("%s\n", line2);
        text = p+1;
    }

#if ! STICK_GUEST
    if (text_in == help_about) {
        printf("(checksum 0x%x)\n", flash_checksum);
    }
#endif
}


// *** demo ***********************************************************
#if STICK_GUEST || SODEBUG
static const char * const demos[] = {
  "rem ### blinky ###\n"
#if STICK_GUEST
  "dim i\n"
#endif
  "dim led as pin e2 for digital output inverted\n"
#if STICK_GUEST
  "while 1 do\n"
  "  for i = 1 to 16\n"
  "    let led = !led\n"
  "    sleep 50 ms\n"
  "  next\n"
  "  sleep 800 ms\n"
  "endwhile\n"
  "end\n"
#endif
,
  "rem ### uart isr ###\n"
#if STICK_GUEST
  "dim data\n"
  "data 1, 1, 2, 3, 5, 8, 13, 21, 0\n"
#endif
  "configure uart 2 for 300 baud 8 data no parity loopback\n"
  "dim tx as pin b7 for uart output\n"
  "dim rx as pin b4 for uart input\n"
  "on uart 2 input do gosub receive\n"
  "on uart 2 output do gosub transmit\n"
#if STICK_GUEST
  "sleep 1000 ms\n"
  "end\n"
  "sub receive\n"
  "  print \"received\", rx\n"
  "endsub\n"
  "sub transmit\n"
  "  read data\n"
  "  if ! data then\n"
  "    return\n"
  "  endif\n"
  "  assert !tx\n"
  "  print \"sending\", data\n"
  "  let tx = data\n"
  "endsub\n"
#endif
,
  "rem ### uart pio ###\n"
#if STICK_GUEST
  "configure uart 1 for 9600 baud 7 data even parity loopback\n"
#endif
  "dim tx as pin b8 for uart output\n"
  "dim rx as pin b6 for uart input\n"
#if STICK_GUEST
  "let tx = 3\n"
  "let tx = 4\n"
  "while tx do\n"
  "endwhile\n"
  "print rx\n"
  "print rx\n"
  "print rx\n"
  "end\n"
#endif
,
  "rem ### toaster ###\n"
#if STICK_GUEST
  "dim target, secs\n"
#endif
  "dim thermocouple as pin a8 for analog input\n"
  "dim relay as pin a0 for digital output\n"
  "dim buzzer as pin a6 for frequency output\n"
#if STICK_GUEST
  "data 512, 90, 746, 105, 894, 20, -1, -1\n"
  "configure timer 0 for 1 s\n"
  "configure timer 1 for 1 s\n"
  "on timer 0 do gosub adjust\n"
  "on timer 1 do gosub beep\n"
  "while target != -1 do\n"
  "  sleep secs s\n"
  "  read target, secs\n"
  "endwhile\n"
  "off timer 0\n"
  "off timer 1\n"
  "let relay = 0\n"
  "let buzzer = 100\n"
  "sleep 1 s\n"
  "let buzzer = 0\n"
  "end\n"
  "sub adjust\n"
  "  if thermocouple >= target then\n"
  "    let relay = 0\n"
  "  else\n"
  "    let relay = 1\n"
  "  endif\n"
  "endsub\n"
  "sub beep\n"
  "  let buzzer = thermocouple\n"
  "  sleep 100 ms\n"
  "  let buzzer = target\n"
  "  sleep 100 ms\n"
  "  let buzzer = 0\n"
  "endsub\n"
#endif
};
#endif


// *** basic0_run() ***************************************************

// this function implements the stickos command interpreter.
void
basic0_run(char *text_in)
{
    int i;
    int d;
    int h;
    int m;
    int t;
    int cmd;
    int len;
#if ZIGFLEA
    bool reset;
    bool init;
#endif
    const char *p;
#if STICK_GUEST || SODEBUG
    const char *np;
#endif
    char *text;
    int number1;
    int number2;
#if SCOPE && ! STICK_GUEST
    int number3;
    int number4;
    bool magnify;
    bool rise;
    bool fall;
    bool level;
    bool automatic;
    bool digital;
    char *pattern;
#endif

    text = text_in;

    parse_trim(&text);

    // parse private commands
    for (cmd = 0; cmd < LENGTHOF(commands); cmd++) {
        len = strlen(commands[cmd]);
        if (! strncmp(text, commands[cmd], len)) {
            break;
        }
    }

    if (cmd != LENGTHOF(commands)) {
        text += len;
    }
    parse_trim(&text);

    number1 = 0;
    number2 = 0;

    switch (cmd) {
#if STICK_GUEST
        case command_exit:
            exit(0);
#endif

        case command_connect:
            if (! zb_present) {
                printf("zigflea not present\n");
#if ! STICK_GUEST
            } else if (zb_nodeid == -1) {
                printf("zigflea nodeid not set\n");
#endif
            } else {
                if (! basic_const(&text, &number1) || number1 == -1) {
                    goto XXX_ERROR_XXX;
                }
                if (*text) {
                    goto XXX_ERROR_XXX;
                }

                printf("press Ctrl-D to disconnect...\n");

#if ! STICK_GUEST
                assert(main_command);
                main_command = NULL;
                terminal_command_ack(false);

                terminal_rxid = number1;

                while (terminal_rxid != -1) {
                    basic0_poll();
                }
#endif

                printf("...disconnected\n");
            }
            break;

#if STICK_GUEST || SODEBUG
        case command_demo:
            number1 = 0;
            if (*text) {
                if (! basic_const(&text, &number1) || number1 < 0 || number1 >= LENGTHOF(demos)) {
                    goto XXX_ERROR_XXX;
                }
            }

            number2 = 0;
            if (*text) {
                if (! basic_const(&text, &number2) || ! number2) {
                    goto XXX_ERROR_XXX;
                }
                if (*text) {
                    goto XXX_ERROR_XXX;
                }
            }

            if (! number2) {
                number2 = 10;
                code_new();
            }

            i = 0;
            for (p = (char *)demos[number1]; *p; p = np+1) {
                np = strchr(p, '\n');
                assert(np);
                strncpy((char *)big_buffer, p, np-p);
                big_buffer[np-p] = '\0';
                code_insert(number2+i*10, (char *)big_buffer, 0);
                i++;
            }
            break;
#endif

        case command_help:
#if ! SODEBUG || STICK_GUEST
            if (! *text) {
                p = help_general;
            } else
#endif
            if (parse_word(&text, "about")) {
                p = help_about;
            } else if (parse_word(&text, "reset")) {
#if ! STICK_GUEST
#if ! defined(__32MX250F128B__)
                printf("RCON = 0x%x; RNMICON = 0x%x\n", RCON, RNMICON);
                RCONCLR = 0xffffff;
#endif
#endif
                break;
#if ! SODEBUG || STICK_GUEST
            } else if (parse_word(&text, "commands")) {
                p = help_commands;
            } else if (parse_word(&text, "modes")) {
                p = help_modes;
            } else if (parse_word(&text, "statements")) {
                p = help_statements;
            } else if (parse_word(&text, "devices")) {
                p = help_devices;
            } else if (parse_word(&text, "blocks")) {
                p = help_blocks;
            } else if (parse_word(&text, "expressions")) {
                p = help_expressions;
            } else if (parse_word(&text, "strings")) {
                p = help_strings;
            } else if (parse_word(&text, "variables")) {
                p = help_variables;
            } else if (parse_word(&text, "pins")) {
                p = help_pins;
            } else if (parse_word(&text, "zigflea")) {
                p = help_zigflea;
#endif
            } else {
                goto XXX_ERROR_XXX;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }
            basic0_help((char *)p);
            break;

        case command_nodeid:
            if (*text) {
                if (parse_word(&text, "none")) {
                    number1 = -1;
                } else {
                    if (! basic_const(&text, &number1) || number1 == -1) {
                        goto XXX_ERROR_XXX;
                    }
                }
                if (*text) {
                    goto XXX_ERROR_XXX;
                }
                var_set_flash(FLASH_NODEID, number1);
#if ! STICK_GUEST
                zb_nodeid = number1;
#endif
            } else {
                i = var_get_flash(FLASH_NODEID);
                if (i == -1) {
                    printf("none\n");
                } else {
                    printf("%u\n", i);
                }
            }
            break;

        case command_reset:
            if (*text) {
                goto XXX_ERROR_XXX;
            }

#if ! STICK_GUEST
            (void)splx(7);
            SYSKEY = 0;
            SYSKEY = 0xAA996655;
            SYSKEY = 0x556699AA;
            RSWRSTSET = _RSWRST_SWRST_MASK;
            while (RSWRST, true) {
                // NULL
            }
            ASSERT(0);
#endif
            break;

#if UPGRADE
        case command_upgrade:  // upgrade StickOS S19 file
            flash_upgrade();
            break;
#endif

        case command_uptime:
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            t = seconds;
            d = t/(24*60*60);
            t = t%(24*60*60);
            h = t/(60*60);
            t = t%(60*60);
            m = t/(60);
            printf("%dd %dh %dm\n", d, h, m);
            break;

#if ZIGFLEA
        case command_zigflea:
            reset = parse_word(&text, "reset");
            init = parse_word(&text, "init");
            if (*text) {
                goto XXX_ERROR_XXX;
            }
#if ! STICK_GUEST
            zb_diag(reset, init);
#endif
            break;
#endif

#if SCOPE && ! STICK_GUEST
        case command_scope:  // <qus> [+-=](<voltage>|0x<bits>) [<slew>|0x<mask>] [<delayus>]
            number1 = 10;
            number2 = -1;
            number3 = -1;
            number4 = 0;
            magnify = false;
            if (*text == '-') {
                magnify = true;
                text++;
            }
            if (*text && (! basic_const(&text, &number1) || number1 == 0)) {
                goto XXX_ERROR_XXX;
            }
            if (magnify) {
                number1 = -number1;
            }
            rise = false;
            fall = false;
            level = false;
            automatic = false;
            if (*text == '+') {
                rise = true;
                text++;
            } else if (*text == '-') {
                fall = true;
                text++;
            } else if (*text == '~') {
                level = true;
                automatic = true;
                text++;
            } else {
                level = true;
            }
            digital = *text == '0' && *(text+1) == 'x';
            if (*text && (! run_input_const(&text, &number2) || number2 &~ 1023)) {
                goto XXX_ERROR_XXX;
            }
            if (*text && (! run_input_const(&text, &number3) || number3 &~ 1023)) {
                goto XXX_ERROR_XXX;
            }
            if (*text && (! run_input_const(&text, &number4) || number4 < 0 || number4 > 1000000)) {
                goto XXX_ERROR_XXX;
            }
            if (*text) {
                goto XXX_ERROR_XXX;
            }

            scope(number1, rise, fall, level, automatic, digital, number2, number3, number4);
            break;

        case command_wave:
            // save the pattern (and frequency following)
            pattern = text;

            // skip the pattern
            while (*text && ! isspace(*text)) {
                text++;
            }

            // skip any spaces
            parse_trim(&text);

            // get the frequency
            if (*text && (! basic_const(&text, &number2) || number2 < 0 || number2 > 10000000)) {
                goto XXX_ERROR_XXX;
            }

            wave(pattern, number2);
            break;
#endif

        case command_hostname:
            if (*text) {
                var_set_flash_name(text);
            } else {
                printf("%s\n", var_get_flash_name());
            }
            break;

        case LENGTHOF(commands):
            // this is not a private command; process a public command...
            basic_run(text_in);
            break;

        default:
            assert(0);
            break;
    }

    return;

XXX_ERROR_XXX:
    terminal_command_error(text-text_in);
}

#if ! STICK_GUEST
void
basic0_poll(void)
{
    terminal_poll();
    var_poll();
}
#endif

