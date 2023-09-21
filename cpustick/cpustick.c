// *** cpustick.c *****************************************************
// this file implements the main program loop of stickos, where we
// wait for and then process (elsewhere) stickos commands.

#include "main.h"

// this is the command line we just received from the FTDI transport
// user console
char *volatile main_command;

bool main_edit;
bool main_prompt = true;
uint32 main_auto;

// this function is called by the FTDI transport when a new command
// command line has been received from the user.
static void
main_command_cbfn(char *command)
{
    // pass the command to the run loop
    assert(! main_command);
    main_command = command;
}

// this function is called by the FTDI transport when the user presses
// Ctrl-C.
static void
main_ctrlc_cbfn(void)
{
    stop();
}

// this function is called by the FTDI transport when the USB device
// is reset.
static void
main_reset_cbfn(void)
{
}

static int autoran;

// this function implements the main program loop of stickos, where we
// wait for and then process (elsewhere) stickos commands.
void
main_run(void)
{
    bool ready;
    bool autoend;
    static int first;
    char buffer[16];

    // we just poll here waiting for commands
    for (;;) {
        basic0_poll();

        ready = 0;
        autoend = false;
        if (! autoran && ! disable_autorun && var_get_flash(FLASH_AUTORUN) == 1) {
            basic0_run("run");
            autoend = true;
        } else if (main_command) {
            basic0_poll();
            if (terminal_echo) {
                printf("\n");
            }
            basic0_run(main_command);
            ready = 1;

            if (main_auto) {
                sprintf(buffer, "%ld ", main_auto);
                terminal_edit(buffer);
                main_edit = true;
                main_auto += 10;
            }
        }
        autoran = true;

        if (autoend || ready) {
            if (! first) {
                basic0_run("help about");
                if (disable_autorun) {
                    printf("AUTORUN DISABLED!\n");
                }
                first = 1;
            }
            if (main_prompt) {
                printf("> ");
            } else {
                printf("done\n");
            }
        }

        if (ready) {
            ready = 0;
            if (main_command) {
                main_command = NULL;
                terminal_command_ack(main_edit);
                main_edit = false;
            }
        }
    }
}

int
main_nodeid()
{
    return var_get_flash(FLASH_NODEID);
}

// this function is called by upper level code to register callback
// functions.
void
main_initialize(void)
{
    // register device mode callbacks
    terminal_register(main_command_cbfn, main_ctrlc_cbfn);

    cdcacm_register(main_reset_cbfn, terminal_receive);

    basic_initialize();
}

