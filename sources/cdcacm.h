// *** cdcacm.h *******************************************************

extern bool cdcacm_active;

typedef void (*cdcacm_reset_cbfn)(void);

typedef bool (*cdcacm_receive_cbfn)(byte *buffer, int length);

void
cdcacm_print(const byte *line, int length);

void
cdcacm_command_ack(void);

void
cdcacm_register(cdcacm_reset_cbfn reset, cdcacm_receive_cbfn receive);

