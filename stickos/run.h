// *** run.h **********************************************************

#define MAX_TIMERS  4

#define TIMER_INTS  MAX_TIMERS

enum {
    num_watchpoints = 4, // tunable number of watchpoints.  values >8 will require minor widening of bit fields.
    all_watchpoints_mask = (1<<num_watchpoints)-1 // mask of all watchpoints.
};

extern bool run_step;
extern bool run_trace;

extern int run_line_number;
extern bool run_in_library;

extern bool run_watchpoint;
extern bool run_condition;

extern bool run_printf;

extern volatile bool run_stop;
extern volatile bool running;  // for profiler
extern volatile int run_breaks;

extern uint32 possible_watchpoints_mask;
extern bool watch_mode_smart;

extern bool run_input_const(IN OUT char **text, OUT int32 *value_out);

extern int32 run_bytecode_const(IN const byte *bytecode, IN OUT int *index);
extern int run_var(const byte *bytecode_in, int length, OUT const char **name, OUT int32 *max_index);
extern int run_expression(const byte *bytecode_in, int length, IN OUT const char **lvalue_var_name, OUT int32 *value);
extern int run_string(IN const byte *bytecode_in, IN int length, IN int size, OUT char *string, OUT int *actual_out);
extern int run_relation(const byte *bytecode_in, int length, OUT int32 *value);
extern int run_relation_or_expression(const byte *bytecode_in, int length, OUT int32 *value);

extern bool run_bytecode(bool immediate, const byte *bytecode, int length);

extern bool run_bytecode_code(uint code, bool immediate, const byte *bytecode, int length);

extern void run_clear(bool flash);

extern bool run(bool cont, int start_line_number);
extern void stop(void);

extern void run_initialize(void);
