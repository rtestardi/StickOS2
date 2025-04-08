// *** basic0.h *******************************************************

enum bytecode2 {
    code_private = code_max,
    code_wave,
    code_max_max
};

// revisit -- move to var2.h???
enum flash_var2 {
    FLASH_UNUSED = FLASH_NEXT,
    FLASH_LAST_LAST
};

extern const char * const help_about;
extern const char * const help_general;

void basic0_help(IN char *text_in);

void basic0_run(char *text_in);

void basic0_poll(void);

