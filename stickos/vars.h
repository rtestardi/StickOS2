// *** vars.h *********************************************************

// variables at end of flash page
enum flash_var {
    FLASH_AUTORUN,
    FLASH_BAUD,
    FLASH_ANALOG,
    FLASH_SERVO,
    FLASH_NODEID,
    FLASH_ASSIGNMENTS_BEGIN,  // pin assignments begin here
    FLASH_ASSIGNMENTS_END = FLASH_ASSIGNMENTS_BEGIN+pin_assignment_max-1,
    FLASH_WATCH_SMART,
    FLASH_NAME_0,
    FLASH_NAME_1,
    FLASH_NAME_2,
    FLASH_NAME_3,
    FLASH_NAME_4,
    FLASH_NAME_5,
    FLASH_NAME_6,
    FLASH_NAME_7,
    FLASH_NEXT,
    // private variables here
    FLASH_LAST = FLASH_PAGE_SIZE/sizeof(uint32)/2
};

// followed by 32 bytes of flash name

#define FLASH_OFFSET(var)  (BASIC_SMALL_PAGE_SIZE-sizeof(uint32)-(FLASH_LAST-(var))*sizeof(uint32))
#define FLASH_NAME_OFFSET  

int var_open_scope(void);
void var_close_scope(int scope);

// called at runtime to manipulate variables
void var_declare(const char *name, int gosubs, int type, bool string, int size, int max_index, int pin_number, int pin_type, int pin_qual, int nodeid, uintptr abs_addr);
void var_declare_reference(const char *name, int gosubs, const char *target_name);
void var_set(const char *name, int index, int32 value);
int32 var_get(const char *name, int index, uint32 running_watchpoint_mask);
int var_get_size(const char *name, int *max_index);
int var_get_length(const char *name);

void var_set_flash(int var, int32 value);
int32 var_get_flash(int var);

// resets volatile variables (and flash params) prior to a run
void var_clear(bool flash);

void var_mem(void);

void var_poll(void);

void var_initialize(void);

void
var_set_flash_name(IN char *name);

char *
var_get_flash_name();
