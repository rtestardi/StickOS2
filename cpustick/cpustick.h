// *** cpustick.h *****************************************************

extern char *volatile main_command;

extern bool main_edit;
extern bool main_prompt;
extern uint32 main_auto;

extern void
main_run(void);

extern int
main_ip_address();

extern int
main_nodeid(void);

extern void
main_initialize(void);

