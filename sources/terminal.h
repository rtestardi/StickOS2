typedef void (*terminal_command_cbfn)(char *command);
typedef void (*terminal_ctrlc_cbfn)(void);

extern bool terminal_echo;
extern int terminal_rxid;  // we send received characters to this node to be received
extern int terminal_txid;  // we send printed characters to this node to be printed
extern volatile int32 terminal_getchar;

void
terminal_print(const byte *buffer, int length);

bool
terminal_receive(byte *buffer, int length);

void
terminal_wait(void);

void
terminal_edit(char *line);

void
terminal_command_discard(bool discard);

bool
terminal_command_discarding(void);

void
terminal_command_ack(bool edit);

void
terminal_command_error(int offset);

void
terminal_poll(void);

void
terminal_register(terminal_command_cbfn command, terminal_ctrlc_cbfn ctrlc);

void
terminal_initialize(void);

