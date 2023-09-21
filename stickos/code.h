// *** code.h *********************************************************

struct line {
    uint16 size;  // of entire struct, rounded up to 4 byte boundary (used internally)
    uint16 length;  // of bytecode (used externally)
    uint16 comment_length;  // follows length (used only for listing)
    int line_number;
    byte bytecode[VARIABLE];
};

#define LINESIZE  OFFSETOF(struct line, bytecode[0])


extern bool code_indent;
extern bool code_numbers;

extern byte *code_library_page;

// used for code execution
struct line *code_next_line(IN bool deleted_ok, IN bool in_library, IN OUT int *line_number);  // returns NULL at eop

struct line *code_line(IN enum bytecode code, IN const byte *name, IN bool print, IN bool library_ok, OUT bool *in_library);

// used for code management
void code_insert(IN int line_number, IN char *text, IN int text_offset);
void code_edit(int line_number_in);
void code_list(bool profile, int start_line_number, int end_line_number, bool in_library);
void code_delete(int start_line_number, int end_line_number);
void code_save(bool fast, int renum);  // coalesce ram/flash and save to alt flash and flip flash flag
void code_new(void);
void code_subs(void);
void code_undo(void);
void code_mem(void);

void code_store(char *name);
void code_load(char *name);
void code_dir(void);
void code_purge(char *name);

// our profiler
void code_timer_poll(void);

void code_clear2(void);

void code_initialize(void);

