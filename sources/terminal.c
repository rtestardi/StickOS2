#include "main.h"

bool terminal_echo = true;
int terminal_rxid = -1;  // node we forward our receives to
int terminal_txid = -1;  // node we forward our prints to
volatile int32 terminal_getchar;

static terminal_command_cbfn command_cbfn;
static terminal_ctrlc_cbfn ctrlc_cbfn;

static bool discard;
static char command[BASIC_INPUT_LINE_SIZE];

// this buffer holds the extra command characters provided by the inbound
// transport beyond the end of the current comamnd.
static char extra[MAX((ZB_PAYLOAD_SIZE), 64)];

static byte hist_in;
static byte hist_out;
static bool hist_first = true;

#define NHIST  8
#define HISTWRAP(x)  (((unsigned)(x))%NHIST)
static char history[NHIST][BASIC_INPUT_LINE_SIZE];

static int ki;
static char keys[8];
static int cursor;

// this buffer holds the tail of the line that still has to be reprinted
// after we insert characters in the middle of the line
static char tail[BASIC_INPUT_LINE_SIZE];

// this buffer holds characters queued by an isr to be echoed later by
// terminal_poll().
static char echo[BASIC_INPUT_LINE_SIZE*2];

// this buffer holds characters queued by an isr to be forwarded later
// by terminal_poll().
static char forward[BASIC_INPUT_LINE_SIZE*2];

static bool ack = true;

void
terminal_print(const byte *buffer, int length)
{
    int id;
    bool printed;

    assert(gpl() == 0);

    led_unknown_progress();

    printed = false;

    // if we're connected to another node...
    id = terminal_txid;
    if (id != -1) {
        // forward packets
        zb_send(id, zb_class_print, length, buffer);
    }

    if (serial_active) {
        serial_send(buffer, length);
        printed = true;
    }

    if (cdcacm_attached && cdcacm_active) {
        cdcacm_print(buffer, length);
        printed = true;
    }
}

// *** line editor ***

enum keys {
    KEY_RIGHT,
    KEY_LEFT,
    KEY_UP,
    KEY_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_BS,
    KEY_BS_DEL,
    KEY_DEL
};

struct keycode {
    char keys[7];
    byte code;
} const keycodes[] = {
    "\033[C", KEY_RIGHT,
    "\033[D", KEY_LEFT,
    "\033[A", KEY_UP,
    "\033[B", KEY_DOWN,
    "\033[H", KEY_HOME,
    "\033[1~", KEY_HOME,
    "\033[K", KEY_END,
    "\033[4~", KEY_END,
    "\010", KEY_BS,
    "\177", KEY_BS_DEL,  // ambiguous
    "\033[3~", KEY_DEL,

    // Everything prior to this line is indexed by "enum keys" values.
    // Aliases for keys must follow this line.

    "\006", KEY_RIGHT, // ctrl-f
    "\002", KEY_LEFT, // ctrl-b
    "\020", KEY_UP, // ctrl-p
    "\016", KEY_DOWN, // ctrl-n
    "\001", KEY_HOME, // ctrl-a
    "\005", KEY_END // ctrl-e
};

#define KEYS_RIGHT  "\033[%dC"
#define KEYS_LEFT  "\033[%dD"
#define KEY_DELETE  "\033[P"
#define KEY_CLEAR  "\033[K"

// this function implements the command line editing functionality
// of the console, by accumulating one character at a time from the
// console.
static
void
accumulate(char c)
{
    int i;
    int n;
    char *p;
    int orig;
    int again;

    assert(gpl() >= MIN(SPL_USB, SPL_IRQ4));

    if (c == '\003' || c == '\025') { // Clear line on ctrl-c or ctrl-u.
        if (cursor) {
            sprintf(echo+strlen(echo), KEYS_LEFT, cursor);
            assert(strlen(echo) < sizeof(echo));
            cursor = 0;
        }
        strcat(echo, KEY_CLEAR);
        assert(strlen(echo) < sizeof(echo));
        command[0] = '\0';
        ki = 0;
        return;
    }

    do {
        again = false;

        keys[ki++] = c;
        keys[ki] = '\0';
        assert(ki < sizeof(keys));

        for (i = 0; i < LENGTHOF(keycodes); i++) {
            if (! strncmp(keycodes[i].keys, keys, ki)) {
                // potential match
                if (keycodes[i].keys[ki]) {
                    // partial match
                    return;
                }

                // full match

                switch (keycodes[i].code) {
                    case KEY_RIGHT:
                        if (cursor < strlen(command)) {
                            strcat(echo, keycodes[KEY_RIGHT].keys);
                            assert(strlen(echo) < sizeof(echo));
                            cursor++;
                        }
                        break;
                    case KEY_LEFT:
                        if (cursor) {
                            strcat(echo, keycodes[KEY_LEFT].keys);
                            assert(strlen(echo) < sizeof(echo));
                            cursor--;
                        }
                        break;

                    case KEY_UP:
                    case KEY_DOWN:
                        if (keycodes[i].code == KEY_UP) {
                            if (hist_first) {
                                hist_out = HISTWRAP(hist_in-1);
                            } else {
                                hist_out = HISTWRAP(hist_out-1);
                            }
                        } else {
                            hist_out = HISTWRAP(hist_out+1);
                        }
                        hist_first = false;
                        for (n = 0; n < NHIST; n++) {
                            if (history[hist_out][0]) {
                                break;
                            }
                            if (keycodes[i].code == KEY_UP) {
                                hist_out = HISTWRAP(hist_out-1);
                            } else {
                                hist_out = HISTWRAP(hist_out+1);
                            }
                        }
                        if (n != NHIST) {
                            if (cursor) {
                                sprintf(echo+strlen(echo), KEYS_LEFT, cursor);
                                assert(strlen(echo) < sizeof(echo));
                                cursor = 0;
                            }
                            strcat(echo, KEY_CLEAR);
                            assert(strlen(echo) < sizeof(echo));
                            strcpy(command, history[hist_out]);

                            // reprint the line
                            strcat(echo, command);
                            assert(strlen(echo) < sizeof(echo));
                            cursor = strlen(command);
                        }
                        break;
                    case KEY_HOME:
                        if (cursor) {
                            sprintf(echo+strlen(echo), KEYS_LEFT, cursor);
                            assert(strlen(echo) < sizeof(echo));
                            cursor = 0;
                        }
                        break;
                    case KEY_END:
                        if (strlen(command)-cursor) {
                            sprintf(echo+strlen(echo), KEYS_RIGHT, strlen(command)-cursor);
                            assert(strlen(echo) < sizeof(echo));
                            cursor = strlen(command);
                        }
                        break;
                    case KEY_BS_DEL:
                    case KEY_BS:
                        if (cursor) {
                            strcat(echo, keycodes[KEY_LEFT].keys);
                            assert(strlen(echo) < sizeof(echo));
                            strcat(echo, KEY_DELETE);
                            assert(strlen(echo) < sizeof(echo));
                            cursor--;
                            memmove(command+cursor, command+cursor+1, sizeof(command)-cursor-1);
                        }
                        break;
                    case KEY_DEL:
                        if (command[cursor]) {
                            strcat(echo, KEY_DELETE);
                            assert(strlen(echo) < sizeof(echo));
                            memmove(command+cursor, command+cursor+1, sizeof(command)-cursor-1);
                        }
                        break;
                    default:
                        assert(0);
                        break;
                }
                ki = 0;
                return;
            }
        }

        // no match

        // if we had already accumulated characters...
        if (ki > 1) {
            // we'll have to go around again
            ki--;
            again = true;
        }

        // process printable characters
        orig = cursor;
        for (i = 0; i < ki; i++) {
            if (isprint(keys[i])) {
                n = strlen(command);
                if (n < sizeof(command)-1) {
                    memmove(command+cursor+1, command+cursor, n+1-cursor);
                    command[cursor] = keys[i];
                    cursor++;
                    assert(cursor <= sizeof(command)-1);
                }
            }
        }

        if (cursor > orig) {
            // print the new characters
            p = strchr(echo, '\0');
            strncpy(p, command+orig, cursor-orig);
            p[cursor-orig] = '\0';

            // N.B. the remainder of the line still needs to be reprinted!
            strcpy(tail, command+cursor);
        }

        ki = 0;
    } while (again);
}

// N.B. if this routine returns false, cdcacm will drop the ball and we'll
// call cdcacm_command_ack() later to pick it up again.
static bool
terminal_receive_internal(byte *buffer, int length)
{
    int i;
    int j;
    int x;
    int id;
    char c;
    bool boo;
    static byte term;

    led_unknown_progress();

    // allow \n or \r to terminate lines, but not both
    j = 0;
    for (i = 0; i < length; i++) {
        c = buffer[i];
        if (c == '\n' || c == '\r') {
            if (! term || term == c) {
                buffer[j] = '\r';
                j++;
            }
            if (! term) {
                term = c;
            } else {
                term = 0;
            }
        } else {
            buffer[j] = c;
            j++;
            term = 0;
        }
    }
    // N.B. we just rewrote buffer and may have shortened length!
    length = j;

    // if we're connected to another node...
    id = terminal_rxid;
    if (id != -1) {
        // forward packets
        x = splx(5);
        strncat(forward, (char *)buffer, length);
        assert(strlen(forward) < sizeof(forward));
        boo = !!strchr((char *)forward, '\r');
        splx(x);

        // if we're forwarding a return...
        if (boo) {
            // slow us down
            ack = false;
            return false;
        }
        return true;
    }

    // accumulate commands
    for (j = 0; j < length; j++) {
        if (buffer[j] == '\003') {
            assert(ctrlc_cbfn);
            ctrlc_cbfn();
        }

        // if the other node just disconnected from us...
        if (buffer[j] == '\004') {
            terminal_txid = -1;
        }

        if (! discard) {
            if (buffer[j] == '\r') {
                // this is a delayed reprint
                strcat(echo, tail);
                assert(strlen(echo) < sizeof(echo));
                tail[0] = '\0';

                if (strcmp(history[HISTWRAP(hist_in-1)], command)) {
                    strcpy(history[hist_in], command);
                    hist_in = HISTWRAP(hist_in+1);
                }

                // save extra for later
                assert(length >= j+1);
                strncpy(extra, (char *)buffer+j+1, length-(j+1));
                extra[length-(j+1)] = '\0';

                ack = false;
                zb_drop(true);

                assert(command_cbfn);
                command_cbfn(command);

                // wait for terminal_command_ack();
                return false;
            } else {
                accumulate(buffer[j]);
            }
        } else {
            terminal_getchar = buffer[j];
        }
    }

    // this is a delayed reprint
    strcat(echo, tail);
    assert(strlen(echo) < sizeof(echo));

    // and back the cursor up
    if (strlen(tail)) {
        sprintf(echo+strlen(echo), KEYS_LEFT, strlen(tail));
        assert(strlen(echo) < sizeof(echo));
    }

    tail[0] = '\0';

    return true;
}

bool
terminal_receive(byte *buffer, int length)
{
    if (length) {
        // reply to local node
        terminal_txid = -1;
    }

    return terminal_receive_internal(buffer, length);
}

void
terminal_wait(void)
{
    while (! ack) {
        os_yield();
    }
}

// this function allows the user to edit a recalled history line.
void
terminal_edit(char *line)
{
    // put an unmodified copy of the line in history
    strncpy(history[hist_in], line, sizeof(history[hist_in])-1);
    hist_in = HISTWRAP(hist_in+1);

    // and then allow the user to edit it
    strncpy(command, line, sizeof(command)-1);
}

// this function causes us to discard typed characters while the
// BASIC program is running, except Ctrl-C.
void
terminal_command_discard(bool discard_in)
{
    discard = discard_in;
    terminal_getchar = 0;
}

bool
terminal_command_discarding(void)
{
    return discard;
}

// this function acknowledges receipt of an FTDI command from upper
// level code.
void
terminal_command_ack(bool edit)
{
    int x;
    bool boo;

    ki = 0;
    hist_first = true;

    if (! edit) {
        memset(command, 0, sizeof(command));
        cursor = 0;
    } else {
        printf("%s", command);
        cursor = strlen(command);
    }

    // if we had extra left over from last time...
    if (extra[0]) {
        // process it now
        x = splx(5);
        boo = terminal_receive_internal((byte *)extra, strlen(extra));
        splx(x);
        if (! boo) {
            // we just issued another command
            return;
        }
    }

    if (terminal_txid == -1) {
        cdcacm_command_ack();
        serial_command_ack();
    }

    ack = true;
    zb_drop(false);
}

// this function is called by upper level code in response to an
// FTDI command error.
void
terminal_command_error(int offset)
{
    int i;
    char buffer[2+BASIC_INPUT_LINE_SIZE+1];

    assert(offset < BASIC_INPUT_LINE_SIZE);

    offset += 2;  // prompt -- revisit, this is decided elsewhere!

    if (offset >= 10) {
        strcpy(buffer, "error -");
        for (i = 7; i < offset; i++) {
            buffer[i] = ' ';
        }
        buffer[i++] = '^';
        assert(i < sizeof(buffer));
        buffer[i] = '\0';
    } else {
        for (i = 0; i < offset; i++) {
            buffer[i] = ' ';
        }
        buffer[i++] = '^';
        assert(i < sizeof(buffer));
        buffer[i] = '\0';
        strcat(buffer, " - error");
    }
    printf("%s\n", buffer);
}

// deliver any pending echo characters the isr stack accumulated
// or forward characters from terminal_receive_internal().
void
terminal_poll(void)
{
    int x;
    static int last;
    char copy[BASIC_INPUT_LINE_SIZE*2];

    assert(gpl() == 0);

    x = splx(5);
    strcpy(copy, echo);
    assert(strlen(copy) < sizeof(copy));
    echo[0] = '\0';
    splx(x);

    if (terminal_echo && copy[0]) {
        terminal_print((byte *)copy, strlen(copy));
        last = msecs;
    }

    x = splx(5);
    strcpy(copy, forward);
    assert(strlen(copy) < sizeof(copy));
    forward[0] = '\0';
    splx(x);

    if (copy[0]) {
        zb_send(terminal_rxid, zb_class_receive, strlen(copy), (byte *)copy);
        if (copy[0] == '\004') {
            // stop forwarding packets on Ctrl-D
            terminal_rxid = -1;
        }

        if (strchr((char *)copy, '\r')) {
            if (terminal_txid == -1) {
                cdcacm_command_ack();
                serial_command_ack();
            }

            ack = true;
        }
    }

    zb_poll();

    os_yield();
}

void
terminal_register(terminal_command_cbfn command, terminal_ctrlc_cbfn ctrlc)
{
    command_cbfn = command;
    ctrlc_cbfn = ctrlc;
}

static void
class_receive(int nodeid, int length, byte *buffer)
{
    if (length) {
        // reply to remote node
        terminal_txid = nodeid;
    }

    (void)terminal_receive_internal(buffer, length);
}

static void
class_print(int nodeid, int length, byte *buffer)
{
    int x;

    x = splx(5);
    strncat(echo, (char *)buffer, length);
    assert(strlen(echo) < sizeof(echo));
    splx(x);
}

void
terminal_initialize(void)
{
    zb_register(zb_class_receive, class_receive);
    zb_register(zb_class_print, class_print);
}

