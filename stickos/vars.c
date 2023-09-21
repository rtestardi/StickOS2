// *** vars.c *********************************************************
// this file implements the variable access module, including ram,
// pin, and flash variables, as well as the external pin control and
// access module.

// Copyright (c) CPUStick.com, 2008-2023.  All rights reserved.
// Patent U.S. 8,117,587.

#include "main.h"

// the last word of each flash bank is the generation number
#define _SGENERATION(p)  *(int32 *)((p)+BASIC_SMALL_PAGE_SIZE-sizeof(uint32))
static
int32
SGENERATION(byte *p)
{
    return _SGENERATION(p);
}

// we always pick the newer flash bank
#define __FLASH_PARAM_PAGE  ((SGENERATION(FLASH_PARAM1_PAGE)+1 > SGENERATION(FLASH_PARAM2_PAGE)+1) ? FLASH_PARAM1_PAGE : FLASH_PARAM2_PAGE)
static
byte *
_FLASH_PARAM_PAGE()
{
    return __FLASH_PARAM_PAGE;
}
#define FLASH_PARAM_PAGE _FLASH_PARAM_PAGE()

static byte *alternate_flash_param_page;

// *** pin variables ***

static struct system_var {
    char *name;
    volatile int32 *integer;  // try this
    int32 constant;  // then this
} const systems[] = {
    "getchar", &terminal_getchar, 0,
    "msecs", &msecs, 0,
    "analog", &pin_analog, 0,
#if ! STICK_GUEST
    "nodeid", &zb_nodeid, 0,
#endif
    "seconds", &seconds, 0,
    "ticks", &ticks, 0,
    "ticks_per_msec", NULL, ticks_per_msec,
};

#define VAR_NAME_SIZE  14

static
struct var {
    char name[VAR_NAME_SIZE];  // 13 char max variable name
    byte gosubs;
    byte type;
    byte string;
    byte size;  // 4 bytes per integer, 2 bytes per short, 1 byte per byte
    uint16 max_index;  // for type == code_pin, this is 1
    union {
        struct {
            int page_offset;
            uint8 watchpoints_mask;  // mask indicating the watchpoints whose conditions depend on this var
            uint16 nodeid;  // for remote variable sets
        } var;
        struct {
            uint16 only_index;  // this is the allowed array index
            byte number;
            uint8 type;
            uint8 qual;
        } pin;  // type=code_pin
        struct {
            struct var *target_var;
        } ref;  // type=code_var_reference
        struct {
            uintptr addr;
        } abs;  // type=code_absolute
    } u;
} vars[BASIC_VARS];

static int max_vars;  // allocated

static int ram_offset;  // allocated in RAM_VARIABLE_PAGE
static int param_offset;  // allocated in FLASH_PARAM_PAGE

// *** system variable access routines ***

// *** pin/watchpoint associations ***
// each array element is a bitmask of watchpoints associated with the
// indexed pin.  Bits are set in var_get() while evaluating a
// watchpoint condition.
static uint8 pin_watchpoint_masks[ROUNDUP(PIN_MAX * num_watchpoints, 8) / 8];

static
int
pin_watchpoint_mask_index(enum pin_number pin)
{
    assert(pin < PIN_MAX);
    return pin / (8 / num_watchpoints);
}

static
int
pin_watchpoint_mask_offset(enum pin_number pin)
{
    assert(pin < PIN_MAX);
    return (pin % (8 / num_watchpoints)) * num_watchpoints;
}

static
void
pin_watchpoint_set_mask(enum pin_number pin, uint32 watchpoint_mask)
{
    assert(pin < PIN_MAX);
    assert((watchpoint_mask & all_watchpoints_mask) == watchpoint_mask);
    assert(pin_watchpoint_mask_index(pin) < LENGTHOF(pin_watchpoint_masks));
    pin_watchpoint_masks[pin_watchpoint_mask_index(pin)] |= watchpoint_mask << pin_watchpoint_mask_offset(pin);
}

static
uint8
pin_watchpoint_get_mask(enum pin_number pin)
{
    assert(pin < PIN_MAX);
    assert(pin_watchpoint_mask_index(pin) < LENGTHOF(pin_watchpoint_masks));
    return (pin_watchpoint_masks[pin_watchpoint_mask_index(pin)] >> pin_watchpoint_mask_offset(pin)) & all_watchpoints_mask;
}

// *** flash control and access ***

// this function erases the alternate parameter page in flash memory.
static
void
flash_erase_alternate(void)
{
    // determine the alternate flash page
    assert(FLASH_PARAM_PAGE == FLASH_PARAM1_PAGE || FLASH_PARAM_PAGE == FLASH_PARAM2_PAGE);
    alternate_flash_param_page = (FLASH_PARAM_PAGE == FLASH_PARAM1_PAGE) ? FLASH_PARAM2_PAGE : FLASH_PARAM1_PAGE;

    // erase the alternate flash page
    assert(BASIC_SMALL_PAGE_SIZE >= FLASH_PAGE_SIZE && ! (BASIC_SMALL_PAGE_SIZE%FLASH_PAGE_SIZE));
    flash_erase_pages((uint32 *)alternate_flash_param_page, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);
}

// this function updates the alternate parameter page in flash memory.
static
void
flash_update_alternate(IN uint32 offset, IN int32 value)
{
    assert(! (offset & (sizeof(uint32)-1)));

    assert(FLASH_PARAM_PAGE != alternate_flash_param_page);

    // copy the initial words from the primary page to the alternate page
    flash_write_words((uint32 *)alternate_flash_param_page, (uint32 *)FLASH_PARAM_PAGE, offset/sizeof(uint32));

    // copy the updated word value at the specified offset to the alternate page
    flash_write_words((uint32 *)(alternate_flash_param_page+offset), (uint32 *)&value, 1);

    // copy the final words from the primary page to the alternate page
    flash_write_words((uint32 *)(alternate_flash_param_page+offset+sizeof(uint32)), (uint32 *)(FLASH_PARAM_PAGE+offset+sizeof(uint32)), (BASIC_SMALL_PAGE_SIZE-sizeof(uint32)-(offset+sizeof(uint32)))/sizeof(uint32));
}

static
void
flash_update_alternate_name(IN char *name_in)
{
    uint32 offset0;
    uint32 offset7;
    char name[32];

    strncpy(name, name_in, sizeof(name));
    name[sizeof(name)-1] = '\0';

    offset0 = FLASH_OFFSET(FLASH_NAME_0);
    offset7 = FLASH_OFFSET(FLASH_NAME_7);

    assert(FLASH_PARAM_PAGE != alternate_flash_param_page);

    // copy the initial words from the primary page to the alternate page
    flash_write_words((uint32 *)alternate_flash_param_page, (uint32 *)FLASH_PARAM_PAGE, offset0/sizeof(uint32));

    // copy the updated word value at the specified offset to the alternate page
    flash_write_words((uint32 *)(alternate_flash_param_page+offset0), (uint32 *)&name, 8);

    // copy the final words from the primary page to the alternate page
    flash_write_words((uint32 *)(alternate_flash_param_page+offset7+sizeof(uint32)), (uint32 *)(FLASH_PARAM_PAGE+offset7+sizeof(uint32)), (BASIC_SMALL_PAGE_SIZE-sizeof(uint32)-(offset7+sizeof(uint32)))/sizeof(uint32));
}

// this function clears the alternate parameter page in flash memory.
static
void
flash_clear_alternate(void)
{
    int32 value;
    uint32 offset;

    assert(FLASH_PARAM_PAGE != alternate_flash_param_page);

    value = 0;

    // for all user variable offsets...
    for (offset = 0; offset < FLASH_OFFSET(0); offset += sizeof(uint32)) {
        assert(! (offset & (sizeof(uint32)-1)));

        // copy the zero word to the alternate page
        flash_write_words((uint32 *)(alternate_flash_param_page+offset), (uint32 *)&value, 1);
    }

    // copy system variables at end of flash page
    flash_write_words((uint32 *)(alternate_flash_param_page+FLASH_OFFSET(0)), (uint32 *)(FLASH_PARAM_PAGE+FLASH_OFFSET(0)), FLASH_LAST);
}

// this function promotes the alternate parameter page in flash memory
// to become current.
static
void
flash_promote_alternate(void)
{
    int32 generation;

    assert(FLASH_PARAM_PAGE != alternate_flash_param_page);

    // update the generation of the alternate page, to make it primary!
    generation = SGENERATION(FLASH_PARAM_PAGE)+1;
    flash_write_words((uint32 *)(alternate_flash_param_page+BASIC_SMALL_PAGE_SIZE-sizeof(uint32)), (uint32 *)&generation, 1);

    assert(FLASH_PARAM_PAGE == alternate_flash_param_page);
    assert(SGENERATION(FLASH_PARAM1_PAGE) != SGENERATION(FLASH_PARAM2_PAGE));

    delay(500);  // this always takes a while!
}

// this function finds the specified variable in our variable table.
//
// If index != -1, then it is a pin array index and it used to locate the pin array element name[index].
// If index == -1, then the first variable with name is returned regardless of its index.
//
// *gosubs is set to the gosub level in which the variable or reference is found.  This may be different than the
// gosub level of the returned variable if a reference was used to locate the returned variable.
static
struct var *
var_find(IN const char *name, IN int index, OUT int *gosubs)
{
    struct var *var;

    // for all declared variables...
    for (var = vars+(max_vars-1); var >= vars; var--) {
        assert(var->type);

        // if the variable name matches...
        // N.B. we use strncmp so the user can use a longer variable name and still match
        if (var->name[0] == name[0] && ! strncmp(var->name, name, sizeof(var->name)-1)) {
            // if var is a reference, return the referred to variable, making note of the gosub level.
            *gosubs = var->gosubs;
            if (var->type != code_ram) {
                if (var->type == code_var_reference) {
                    // assert that the referent is in an outer scope.
                    assert(var > var->u.ref.target_var);

                    var = var->u.ref.target_var;

                    // references do not refer to other references.  every reference reaches its final destination var.
                    assert(var->type != code_var_reference);
                }

                // if an index is specified and we're considering a pin array element, then ensure the index matches
                if ((index != -1) && (var->type == code_pin) && (var->u.pin.only_index != index)) {
                    continue;
                }
            }

            return var;
        }
    }
    return NULL;
}

static
const struct system_var *
system_find(const char *name)
{
    int i;

    for (i = 0; i < LENGTHOF(systems); i++) {
        if (! strcmp(name, systems[i].name)) {
            return systems+i;
        }
    }
    return NULL;
}

// this function opens a new gosub variable scope; variables declared
// in the gosub will be automatically undeclared when the gosub
// returns.
int
var_open_scope(void)
{
    return max_vars;
}

// this function closes a gosub scope when the gosub returns,
// automatically undeclaring any variables declared in the gosub.
void
var_close_scope(IN int scope)
{
    int i;

    assert(scope >= 0);

    // reclaim ram space, if any was allocated by this scope.
    if (max_vars > scope) {

        // Because variables are going out of scope, pin->watchpoint_condition relationships may be changing:
        // - invalidate all pin->watchpoint_condition relationships.
        // - schedule all watchpoints to re-execute asap, which will re-establish pin->watchpoint_condition relationships.
        memset(pin_watchpoint_masks, 0, sizeof(pin_watchpoint_masks));
        possible_watchpoints_mask = all_watchpoints_mask;

        // find the first (if any exist) non-reference variable in this scope by skipping any references.
        for (i = scope; (i < max_vars) && (vars[i].type == code_var_reference); i++) {
        }

        // release any ram allocate by this scope's variables.
        if (i < max_vars) {
            assert(vars[i].type == code_ram || vars[i].type == code_nodeid);
            ram_offset = vars[i].u.var.page_offset;
            memset(RAM_VARIABLE_PAGE+ram_offset, 0, sizeof(RAM_VARIABLE_PAGE)-ram_offset);
        }

        max_vars = scope;
    }
}

static void
var_declare_internal(IN const char *name, IN int gosubs, IN int type, IN bool string, IN int size, IN int max_index, IN int pin_number, IN int pin_type, IN int pin_qual, IN int nodeid, IN struct var *target, IN uintptr abs_addr)
{
    struct var *var;
    int var_gosubs;

    assert(name);
    assert(type >= code_deleted && type < code_max);

    if (type == code_var_reference) {
        assert(pin_number == -1);
        assert(pin_type == -1);
        assert(pin_qual == -1);
        assert(nodeid == -1);

        assert(gosubs > 0); // cannot currently create reference outside of a gosub

        // otherwise, we're declaring a reference to a non-system variable
        assert(target != NULL);
    }

    if (! run_condition) {
        return;
    }

    if ((type == code_flash || type == code_pin) && gosubs) {
        printf("declared flash or pin variable in sub\n");
        stop();
        return;
    }

    // catch declaration of zero length array.  allow pin array element 0.
    if ((max_index == 0) && (type != code_pin)) {
        printf("declared 0 length array\n");
        stop();
        return;
    }

    // see if the variable name (and index if a pin array element) is already in-use.
    var = var_find(name, type==code_pin ? max_index : -1, &var_gosubs);
    // if the var already exists...
    if (var) {
        // if the var exists at the same scope...
        if (var_gosubs == gosubs) {
            // this is a repeat dimension
            printf("var '%s' already declared at this scope\n", name);
            stop();
            return;
        } else {
            assert(gosubs > var_gosubs);
        }
    }

    // if we're out of vars...
    if (max_vars >= BASIC_VARS) {
        printf("out of variables\n");
        stop();
        return;
    }

    // declare the var
    var = &vars[max_vars];
    strncpy(var->name, name, sizeof(var->name)-1);
    assert(var->name[sizeof(var->name)-1] == '\0');
    var->gosubs = gosubs;
    var->type = type;
    var->string = string;
    assert(size == sizeof(byte) || size == sizeof(short) || size == sizeof(uint32));
    var->size = size;
    if (type != code_pin) {  // allow pin array element 0.
        assert(max_index > 0);
    }

    switch (type) {
        case code_nodeid:
            assert(size == 4);  // integer only
        case code_ram:
            assert(! pin_number);
            // if we're out of ram space...
            if (ram_offset+max_index*var->size > sizeof(RAM_VARIABLE_PAGE)) {
                var->max_index = 0;
                printf("out of variable ram\n");
                stop();
                return;
            }
            // *** RAM control and access ***
            // allocate the ram var
            var->max_index = max_index;
            var->u.var.page_offset = ram_offset;
            var->u.var.watchpoints_mask = 0;
            var->u.var.nodeid = nodeid;
            ram_offset += max_index*var->size;
            break;

        case code_flash:
            assert(! pin_number);
            assert(size == 4);  // integer only
            // if we're out of flash space...
            if (param_offset+max_index*var->size > BASIC_SMALL_PAGE_SIZE-(FLASH_LAST+1)*sizeof(uint32)) {
                var->max_index = 0;
                printf("out of parameter flash\n");
                stop();
                return;
            }
            // *** flash control and access ***
            // allocate the flash var
            assert(! (param_offset & (sizeof(uint32)-1)));  // integer only
            var->max_index = max_index;
            var->u.var.page_offset = param_offset;
            var->u.var.watchpoints_mask = 0;
            assert(var->size == sizeof(uint32));  // integer only
            param_offset += max_index*var->size;
            break;

        case code_pin:
            // *** external pin control and access ***
            // N.B. this was checked when we parsed
            assert(pins[pin_number].pin_type_mask & (1<<pin_type));
            var->max_index = 1;
            var->u.pin.only_index = max_index;
            var->u.pin.number = pin_number;
            var->u.pin.type = pin_type;
            var->u.pin.qual = pin_qual;

#if ! STICK_GUEST
            pin_declare(pin_number, pin_type, pin_qual);
#endif
            break;

        case code_var_reference:
            var->max_index = 0;  // unused
            var->u.ref.target_var = target;
            break;

        case code_absolute:
            var->max_index = max_index;
            var->u.abs.addr = abs_addr;
            break;

        default:
            assert(0);
            break;
    }

    max_vars++;
}

// this function declares a ram, flash, pin, or abs variable!
void
var_declare(IN const char *name, IN int gosubs, IN int type, IN bool string, IN int size, IN int max_index, IN int pin_number, IN int pin_type, IN int pin_qual, IN int nodeid, IN uintptr abs_addr)
{
    assert(type != code_var_reference);
    assert(string ? type == code_ram && size == 1 : true);

    var_declare_internal(name, gosubs, type, string, size, max_index, pin_number, pin_type, pin_qual, nodeid, NULL, abs_addr);
}

void
var_declare_reference(const char *name, int gosubs, const char *target_name)
{
    struct var *target;
    int target_gosubs;

    // see if the referent is a normal variable or another reference...
    target = var_find(target_name, 0, &target_gosubs);
    if (! target) {
        printf("referent '%s' undefined\n", target_name);
        stop();
        return;
    }

    var_declare_internal(name, gosubs, code_var_reference, false, target->size, target->max_index, -1, -1, -1, -1, target, -1);
}

typedef struct remote_set {
    char name[VAR_NAME_SIZE];  // 14 char max variable name
    char pad;  // XXX -- figure out how to get pragma pack working
    int32 index;
    int32 value;
} remote_set_t;

static remote_set_t set;

static bool remote;

#if ! STICK_GUEST
static void
class_remote_set(int nodeid, int length, byte *buffer)
{
    // remember to set the variable as requested by the remote node
    assert(length == sizeof(set));
    set = *(remote_set_t *)buffer;

    // byteswap in place
    set.index = TF_BIG(set.index);
    set.value = TF_BIG(set.value);
}

void
var_poll(void)
{
    bool condition;

    if (set.name[0]) {
        assert(! remote);
        remote = true;
        condition = run_condition;
        run_condition = true;

        // set the variable as requested by the remote node
        var_set(set.name, set.index, set.value);
        set.name[0] = '\0';

        run_condition = condition;
        assert(remote);
        remote = false;
    }
}
#endif

// this function sets the value of a ram, flash, or pin variable!
void
var_set(IN const char *name, IN int index, IN int32 value)
{
#if 0  // unused for now
    int i;
#endif
    uint type;
    int var_gosubs;
    struct var *var;
#if ! STICK_GUEST
    remote_set_t set;
#endif

    if (! run_condition) {
        return;
    }

    var = var_find(name, index, &var_gosubs);
    if (! var) {
        printf("var '%s' undefined\n", name);
        stop();
    } else {
        if (var->type == code_ram && index < var->max_index) {
            goto XXX_PERF_XXX;
        }
        type = var->type;
        if ((type != code_pin && index >= var->max_index) || (type == code_pin && index != var->u.pin.only_index)) {
            printf("var '%s' index %d out of range\n", name, index);
            stop();
        } else {
            switch (type) {
                case code_nodeid:
                    if (! zb_present) {
                        printf("zigflea not present\n");
                        stop();
                        break;
#if ! STICK_GUEST
                    } else if (zb_nodeid == -1) {
                        printf("zigflea nodeid not set\n");
                        stop();
                        break;
#endif
                    }

#if ! STICK_GUEST
                    // if we're not being set from a remote node...
                    if (! remote) {
                        // forward the variable set request to the remote node
                        strcpy(set.name, var->name);
                        set.index = TF_BIG((int32)index);
                        set.value = TF_BIG((int32)value);
                        if (! zb_send(var->u.var.nodeid, zb_class_remote_set, sizeof(set), (byte *)&set)) {
                            value = -1;
                        }
                    }
                    // fall thru
#endif

                case code_ram:
XXX_PERF_XXX:
                    // *** RAM control and access ***
                    // set the ram variable to value
                    if (var->size == sizeof(uint32)) {
                        write32(RAM_VARIABLE_PAGE+var->u.var.page_offset+index*sizeof(uint32), value);
                    } else if (var->size == sizeof(short)) {
                        write16(RAM_VARIABLE_PAGE+var->u.var.page_offset+index*sizeof(short), value);
                    } else {
                        assert(var->size == sizeof(byte));
                        *(byte *)(RAM_VARIABLE_PAGE+var->u.var.page_offset+index) = (byte)value;
                    }

                    // wakeup any watchpoints watching this var
                    possible_watchpoints_mask |= var->u.var.watchpoints_mask;
                    break;

                case code_flash:
                    assert(var->size == sizeof(uint32));

                    // *** flash control and access ***
                    // if the flash variable is not already equal to value
                    if (*(int32 *)(FLASH_PARAM_PAGE+var->u.var.page_offset+index*sizeof(uint32)) != value) {
                        // set the flash variable to value
                        flash_erase_alternate();
                        flash_update_alternate(var->u.var.page_offset+index*sizeof(uint32), value);
                        flash_promote_alternate();

                        // wakeup any watchpoints watching this var
                        possible_watchpoints_mask |= var->u.var.watchpoints_mask;
                    }
                    break;

                case code_pin:
                    // *** external pin control and access ***
                    if (var->u.pin.type == pin_type_digital_input || var->u.pin.type == pin_type_analog_input || var->u.pin.type == pin_type_uart_input) {
                        printf("var '%s' readonly\n", name);
                        stop();
                        break;
                    }

#if ! STICK_GUEST
                    pin_set(var->u.pin.number, var->u.pin.type, var->u.pin.qual, value);
#endif
                    break;

                case code_absolute:
#if ! STICK_GUEST
                    write_n_bytes(var->size, (volatile void *)(var->u.abs.addr + (index * var->size)), value);
#endif
                    break;

                default:
                    assert(0);
                    break;
            }

            if (run_trace) {
                // *** interactive debugger ***
                // if debug tracing is enabled...
                if (var->max_index > 1) {
                    printf("    let %s[%d] = %ld\n", name, index, value);
                } else {
                    printf("    let %s = %ld\n", name, value);
                }
            }
        }
    }
}

// this function gets the value of a ram, flash, or pin variable!
int32
var_get(IN const char *name, IN int index, IN uint32 running_watchpoint_mask)
{
    uint type;
    int32 value;
    int var_gosubs;
    struct var *var;
    const struct system_var *system;

    if (! run_condition) {
        return 0;
    }

    value = 0;

    var = var_find(name, index, &var_gosubs);
    if (! var) {
        if (! index) {
            // see if this could be a special system variable
            system = system_find(name);
            if (system) {
                value = system->integer ? *system->integer : system->constant;
                if (system->integer == &terminal_getchar && ! run_watchpoint && ! running_watchpoint_mask) {
                    terminal_getchar = 0;
                }
                return value;
            } else if (! strcmp(name, "random")) {
                return random_32();
            }
        }
        printf("var '%s' undefined\n", name);
        stop();
    } else {
        if (var->type == code_ram && index < var->max_index) {
            goto XXX_PERF_XXX;
        }
        type = var->type;
        if ((type != code_pin && index >= var->max_index) || (type == code_pin && index != var->u.pin.only_index)) {
            printf("var '%s' index %d out of range\n", name, index);
            stop();
        } else {
            switch (type) {
                case code_nodeid:
                case code_ram:
XXX_PERF_XXX:
                    // *** RAM control and access ***
                    // get the value of the ram variable
                    if (var->size == sizeof(uint32)) {
                        value = read32(RAM_VARIABLE_PAGE+var->u.var.page_offset+index*sizeof(uint32));
                    } else if (var->size == sizeof(short)) {
                        value = read16(RAM_VARIABLE_PAGE+var->u.var.page_offset+index*sizeof(short));
                    } else {
                        assert(var->size == sizeof(byte));
                        value = *(byte *)(RAM_VARIABLE_PAGE+var->u.var.page_offset+index);
                    }
                    var->u.var.watchpoints_mask |= running_watchpoint_mask;
                    break;

                case code_flash:
                    assert(var->size == sizeof(uint32));

                    // *** flash control and access ***
                    // get the value of the flash variable
                    value = *(int32 *)(FLASH_PARAM_PAGE+var->u.var.page_offset+index*sizeof(uint32));

                    var->u.var.watchpoints_mask |= running_watchpoint_mask;
                    break;

                case code_pin:
                    // *** external pin control and access ***
#if ! STICK_GUEST
                    value = pin_get(var->u.pin.number, var->u.pin.type, var->u.pin.qual);

                    pin_watchpoint_set_mask((enum pin_number)var->u.pin.number, running_watchpoint_mask);

                    // if this pin is part of any watchpoints, then mark the watchpoint(s) as possible.
                    possible_watchpoints_mask |= pin_watchpoint_get_mask((enum pin_number)var->u.pin.number);
#endif
                    break;

                case code_absolute:
#if ! STICK_GUEST
                    value = read_n_bytes(var->size, (const volatile void *)(var->u.abs.addr + (index * var->size)));
#endif
                    break;

                default:
                    assert(0);
                    break;
            }
        }
    }

    return value;
}

// this function gets the size of a ram, flash, or pin variable!
int
var_get_size(IN const char *name, OUT int *max_index)
{
    int size;
    int var_gosubs;
    struct var *var;

    if (! run_condition) {
        *max_index = 1;
        return 1;
    }

    size = 1;

    var = var_find(name, -1, &var_gosubs);
    if (! var) {
        // see if this could be a special system variable
        if (system_find(name)) {
            *max_index = 1;
            return sizeof(int32);
        } else if (! strcmp(name, "random")) {
            *max_index = 1;
            return sizeof(int32);
        }
        printf("var '%s' undefined\n", name);
        stop();
        *max_index = 0;
    } else {
        size = var->size;
        *max_index = var->max_index;
    }

    return size;
}

// this function gets the number of elements in an array or string
int
var_get_length(IN const char *name)
{
    int length;
    int var_gosubs;
    struct var *var;

    if (! run_condition) {
        return 0;
    }

    length = 0;

    var = var_find(name, -1, &var_gosubs);
    if (! var) {
        printf("var '%s' undefined\n", name);
        stop();
    } else {
        // if this is a string...
        if (var->string) {
            while (var_get(name, length, 0)) {
                length++;
                if (length == var->max_index) {
                    break;
                }
            }
        } else {
            // this is an array
            length = var->max_index;
        }
    }

    return length;
}

// *** flash control and access ***

// this function sets the value of a flash mode parameter
void
var_set_flash(IN int var, IN int32 value)
{
    assert(var >= 0 && var < FLASH_LAST);

    if (*(int *)(FLASH_PARAM_PAGE+FLASH_OFFSET(var)) != value) {
        flash_erase_alternate();
        flash_update_alternate(FLASH_OFFSET(var), value);
        flash_promote_alternate();
    }
}

void
var_set_flash_name(IN char *name)
{
    flash_erase_alternate();
    flash_update_alternate_name(name);
    flash_promote_alternate();
}

// this function gets the value of a flash mode parameter.
int32
var_get_flash(IN int var)
{
    assert(var >= 0 && var < FLASH_LAST);

    return *(int32 *)(FLASH_PARAM_PAGE+FLASH_OFFSET(var));
}

char *
var_get_flash_name()
{
    if (*(int32 *)(FLASH_PARAM_PAGE+FLASH_OFFSET(FLASH_NAME_0)) == -1 || ! *(byte *)(FLASH_PARAM_PAGE+FLASH_OFFSET(FLASH_NAME_0))) {
        return "Flea-Scope";
    } else {
        return (char *)(FLASH_PARAM_PAGE+FLASH_OFFSET(FLASH_NAME_0));
    }
}

// this function clears variables before a BASIC program run.
void
var_clear(IN bool flash)
{
    max_vars = 0;
    ram_offset = 0;
    param_offset = 0;
    memset(RAM_VARIABLE_PAGE, 0, sizeof(RAM_VARIABLE_PAGE));

    if (flash) {
        flash_erase_alternate();
        flash_clear_alternate();
        flash_promote_alternate();
    }

    pin_clear();
}

// this function prints variable memory usage.
void
var_mem(void)
{
    printf("%3d%% ram variable bytes used\n", ram_offset*100/sizeof(RAM_CODE_PAGE));
    printf("%3d%% flash parameter bytes used\n", param_offset*100/(BASIC_SMALL_PAGE_SIZE-(FLASH_LAST+1)*sizeof(uint32)));
    printf("%3d%% variables used\n", max_vars*100/BASIC_VARS);
}

void
var_initialize(void)
{
    // because struct var uses a byte for pin.number, assert that a byte is large enough to represented all pin numbers.
    assert(PIN_MAX <= 255);

    // because pin_watchpoint_*() routines depend on a power of 2 number of watchpoints.
    assert((num_watchpoints == 1) || (num_watchpoints == 2) || (num_watchpoints == 4) || (num_watchpoints == 8));

#if ! STICK_GUEST
    zb_register(zb_class_remote_set, class_remote_set);
#endif
}
