// *** code.c *********************************************************
// this file implements bytecode code storage and access and merge
// functionality, as well as the stickos filesystem.

// Copyright (c) CPUStick.com, 2008-2023.  All rights reserved.
// Patent U.S. 8,117,587.

#include "main.h"

bool code_indent = true;
bool code_numbers = true;

byte *code_library_page;

// the last word of each flash bank is the generation number
#define _LGENERATION(p)  *(int32 *)((p)+BASIC_LARGE_PAGE_SIZE-sizeof(uint32))
static
int32
LGENERATION(byte *p)
{
    return _LGENERATION(p);
}

#undef PAGE_SIZE
#define _PAGE_SIZE(p)  (((p) == FLASH_CODE1_PAGE || (p) == FLASH_CODE2_PAGE || (p) == code_library_page) ? BASIC_LARGE_PAGE_SIZE : sizeof(RAM_CODE_PAGE))
static
int
PAGE_SIZE(byte *p)
{
    return _PAGE_SIZE(p);
}

// we always pick the newer flash bank
#define __FLASH_CODE_PAGE  ((LGENERATION(FLASH_CODE1_PAGE)+1 > LGENERATION(FLASH_CODE2_PAGE)+1) ? FLASH_CODE1_PAGE : FLASH_CODE2_PAGE)
static
byte *
_FLASH_CODE_PAGE()
{
    return __FLASH_CODE_PAGE;
}
#define FLASH_CODE_PAGE _FLASH_CODE_PAGE()


// profiling
#define PROFILE_BUFFER  (RAM_CODE_PAGE+OFFSETOF(struct line, bytecode))
#define PROFILE_BUFFER_SIZE  (sizeof(RAM_CODE_PAGE)-OFFSETOF(struct line, bytecode))
#define PROFILE_BUCKETS  (PROFILE_BUFFER_SIZE/sizeof(struct bucket))

static uint32 profile_shift;  // right shift, line_number to bucket number

struct bucket {
    uint32 hits;
} *profile_buckets;

uint32 profile_other;
uint32 profile_library;

static
void
check_line(byte *page, struct line *line)
{
    assert(line->line_number != -1);
    if (line->line_number) {
        assert(line->length+line->comment_length > 0 && line->length+line->comment_length <= BASIC_BYTECODE_SIZE);
    } else {
        assert(line->length+line->comment_length == 0);
    }
    assert(line->size == ROUNDUP(line->length+line->comment_length, sizeof(uint32)) + LINESIZE);

    if (page) {
        assert((byte *)line >= page && (byte *)line+line->size <= page+PAGE_SIZE(page));
    }
}

#if ! SODEBUG
#define check_line(a, b)
#endif


// this function returns the first code line in a page.
static
inline
struct line *
find_first_line_in_page(byte *page)
{
    struct line *line;

    line = (struct line *)page;
    check_line(page, line);
    return line;
}

// this function returns the next code line in a page.
static
inline
struct line *
find_next_line_in_page(byte *page, struct line *line)
{
    struct line *next;

    // if we're about to go past the end...
    if (! line->line_number) {
        return NULL;
    }

    check_line(page, line);
    next = (struct line *)((byte *)line + line->size);
    check_line(page, next);
    assert(next->line_number > line->line_number || ! next->line_number);
    return next;
}

// this function finds the exact code line in the page, if it exists.
static
struct line *
find_exact_line_in_page(byte *page, int line_number)
{
    struct line *line;

    for (line = find_first_line_in_page(page); line; line = find_next_line_in_page(page, line)) {
        if (line->line_number == line_number) {
            return line;
        }
    }
    return NULL;
}

// this function finds the following code line in the page, if it
// exists.
static
struct line *
find_following_line_in_page(byte *page, int line_number)
{
    struct line *line;

    for (line = find_first_line_in_page(page); line; line = find_next_line_in_page(page, line)) {
        if (line->line_number > line_number || ! line->line_number) {
            return line;
        }
    }
    return NULL;
}

// this function deletes a code line from a page.
static
void
delete_line_in_page(byte *page, struct line *line)
{
    int shrink;
    struct line *next;

    check_line(page, line);
    assert(line->line_number);
    assert(page == RAM_CODE_PAGE);

    next = (struct line *)((byte *)line + line->size);

    memmove((byte *)line, (byte *)next, page+PAGE_SIZE(page)-(byte *)next);
    shrink = (byte *)next - (byte *)line;
    memset(page+PAGE_SIZE(page)-shrink, 0, shrink);

    // verify
    find_exact_line_in_page(page, 0);
}

// this function inserts a code line in a page.
static
bool
insert_line_in_page(byte *page, int line_number, int length, int comment_length, byte *bytecode)
{
    int room;
    int grow;
    struct line *eop;
    struct line *line;

    assert(line_number);
    assert(length);
    assert(page == RAM_CODE_PAGE);

    // delete this line if it exists
    line = find_exact_line_in_page(page, line_number);
    if (line) {
        delete_line_in_page(page, line);
    }

    // find the eop
    eop = find_exact_line_in_page(page, 0);
    assert(eop && ! eop->line_number && eop->length == 0 && eop->size == LINESIZE);
    room = page+sizeof(RAM_CODE_PAGE) - ((byte *)eop+eop->size);

    // check for available memory
    grow = LINESIZE+ROUNDUP(length+comment_length, sizeof(uint32));
    if (grow > room) {
        return false;
    }

    // shift the following lines down
    line = find_following_line_in_page(page, line_number);
    assert(line);
    assert(((byte *)line+grow) + (((byte *)eop+eop->size) - (byte *)line) <= page+sizeof(RAM_CODE_PAGE));
    memmove((byte *)line+grow, (byte *)line, ((byte *)eop+eop->size) - (byte *)line);

    // insert the new line
    line->line_number = line_number;
    line->size = grow;
    line->length = length;
    line->comment_length = comment_length;
    assert((unsigned)length+comment_length <= BASIC_BYTECODE_SIZE);
    memcpy(line->bytecode, bytecode, length+comment_length);

    // verify
    find_exact_line_in_page(page, 0);

    return true;
}

// this function finds the next logical code line, from either the
// ram or flash code page.
static
struct line *
code_next_line_internal(IN bool deleted_ok, IN bool in_library, IN OUT int *line_number)
{
    struct line *line;
    struct line *ram_line;
    struct line *flash_line;

    // if we're looking in primary code space...
    if (! in_library) {
        do {
            ram_line = find_following_line_in_page(RAM_CODE_PAGE, *line_number);
            assert(ram_line);
            flash_line = find_following_line_in_page(FLASH_CODE_PAGE, *line_number);
            assert(flash_line);

            if (! ram_line->line_number && ! flash_line->line_number) {
                return NULL;
            }

            if (! ram_line->line_number) {
                line = flash_line;
            } else if (! flash_line->line_number) {
                line = ram_line;
            } else if (flash_line->line_number < ram_line->line_number) {
                line = flash_line;
            } else {
                line = ram_line;
            }

            *line_number = line->line_number;
        } while (! deleted_ok && line->length == 1 && line->bytecode[0] == code_deleted);
    } else {
        // look in the library code space
        line = find_following_line_in_page(code_library_page, *line_number);
        assert(line);

        if (! line->line_number) {
            return NULL;
        }

        *line_number = line->line_number;
    }

    return line;
}

// LRU cache
static struct line *last_seq_line;
static int last_seq_line_number;
static bool last_seq_in_library;
static struct line *last_nonseq_line;
static int last_nonseq_line_number;
static bool last_nonseq_in_library;

// this function finds the next logical code line, hopefully from
// the lru cache
struct line *
code_next_line(IN bool deleted_ok, IN bool in_library, IN OUT int *line_number)
{
    byte *code_page;
    struct line *line;

    // performance path
    // N.B. this works if the code is all in the same page -- i.e., saved
    if (! ((struct line *)RAM_CODE_PAGE)->line_number && *line_number) {
        code_page = in_library ? code_library_page : FLASH_CODE_PAGE;

        // if we've got a sequential LRU cache hit...
        if (*line_number == last_seq_line_number && in_library == last_seq_in_library && last_seq_line) {
            line = find_next_line_in_page(code_page, last_seq_line);
            check_line(code_page, line);
            if (! line->line_number) {
                return NULL;
            }
            *line_number = line->line_number;
            goto XXX_DONE_XXX;
        }

        // if we've got a non-sequential cache hit...
        if (*line_number == last_nonseq_line_number && in_library == last_nonseq_in_library && last_nonseq_line) {
            line = last_nonseq_line;
            *line_number = line->line_number;
            goto XXX_DONE_XXX;
        }
    }

    last_nonseq_line_number = *line_number;
    last_nonseq_in_library = in_library;

    line = code_next_line_internal(deleted_ok, in_library, line_number);

    last_nonseq_line = line;  // remember this for subsequent non-sequential accesses

XXX_DONE_XXX:
    last_seq_line_number = *line_number;
    last_seq_in_library = in_library;
    last_seq_line = line;  // remember this for subsequent sequential accesses

    assert(line != (struct line *)-1);
    return line;
}

// this function returns the line where the specified sub_name
// exists.
struct line *
code_line(IN enum bytecode code, IN const byte *name, IN bool print, IN bool library_ok, OUT bool *in_library)
{
    bool library;
    int line_number;
    struct line *line;

    assert(code == code_label || code == code_sub);

    // first check the primary code space, then check the library code space...
    for (library = false; library <= true; library++) {
        if (! library || (library_ok && code_library_page)) {
            line_number = 0;
            for (;;) {
                line = code_next_line(false, library, &line_number);
                if (line) {
                    if (line->bytecode[0] == code) {
                        if (! strcmp((char *)line->bytecode+1, (const char *)name)) {
                            if (in_library) {
                                *in_library = library;
                            }
                            return line;
                        }
                        if (print) {
                            printf("  %s\n", (char *)line->bytecode+1);
                        }
                    }
                } else {
                    break;
                }
            }
            if (print && ! library && library_ok && code_library_page) {
                printf("library:\n");
            }
        }
    }
    return NULL;
}

// *** RAM control and access ***

// this function inserts a line into the ram code page, possibly
// performing an auto save to the flash code page if needed.
void
code_insert(int line_number, char *text_in, IN int text_offset)
{
    char *c;
    int length;
    int comment_length;
    int syntax_error;
    char text[BASIC_OUTPUT_LINE_SIZE];
    byte bytecode[BASIC_BYTECODE_SIZE];

    syntax_error = -1;

    if (text_in) {
        // if there is a comment...
        // XXX -- can't put comment in quotes!
        c = strstr(text_in, "//");
        if (c) {
            strncpy(text, text_in, c-text_in);
            text[c-text_in] = '\0';
            c += 2;  // skip the "//"
            comment_length = strlen(c)+1;
        } else {
            strcpy(text, text_in);
            comment_length = 0;
        }
        if (! parse_line(text, &length, bytecode, &syntax_error)) {
            terminal_command_error(text_offset + syntax_error);
#if ! STICK_GUEST
            main_auto = 0;
#endif
            return;
        }
        if (c) {
            memcpy(bytecode+length, c, comment_length);
        }
    } else {
        bytecode[0] = code_deleted;
        length = 1;
        comment_length = 0;
    }

    if (! insert_line_in_page(RAM_CODE_PAGE, line_number, length, comment_length, bytecode)) {
        printf("auto save\n");
        code_save(true, 0);
        if (! insert_line_in_page(RAM_CODE_PAGE, line_number, length, comment_length, bytecode)) {
            printf("out of code ram\n");
        }
    }
}

// this function deletes lines into the ram code page (note that
// deleted lines are actually *added* to the ram code page, to
// override existing lines in the flash code page!).
void
code_delete(int start_line_number, int end_line_number)
{
    int code;
    int line_number;
    struct line *line;

    if (! start_line_number && ! end_line_number) {
        return;
    }

    line_number = start_line_number?start_line_number-1:0;
    for (;;) {
        line = code_next_line(false, false, &line_number);
        if (line) {
            code = line->bytecode[0];
            if (end_line_number && line_number > end_line_number) {
                break;
            }
            code_insert(line_number, NULL, 0);
            if (line_number > start_line_number && end_line_number == 0x7fffffff && code == code_endsub) {
                break;
            }
        } else {
            break;
        }
    }
}

// this function lists lines.
void
code_list(bool profile, int start_line_number, int end_line_number, bool in_library)
{
    byte code;
    int p;
    int32 phits;
    int bucket;
    int indent;
    int line_number;
    int profiled_buckets;
    struct line *line;
    char *text;

    text = (char *)big_buffer;

    if (profile) {
        assert(! in_library);
        if (((struct line *)RAM_CODE_PAGE)->line_number) {
            printf("save code to profile\n");
            return;
        }
        printf("%7ldms other\n", profile_other);
        if (code_library_page) {
            printf("%7ldms library\n", profile_library);
        }
    }

    if (in_library) {
        assert(! profile);
        assert(code_library_page);
        printf("library:\n");
    }

    indent = 0;
    line_number = 0;
    profiled_buckets = start_line_number>>profile_shift;
    for (;;) {
        line = code_next_line(false, in_library, &line_number);
        if (line) {
            code = line->bytecode[0];
            if (code == code_endif || code == code_else || code == code_elseif || code == code_endwhile || code == code_until || code == code_next || code == code_endsub) {
                if (indent) {
                    indent--;
                } else {
                    printf("missing block begins?\n");
                }
            }
            if (line_number >= start_line_number && (! end_line_number || line_number <= end_line_number)) {
                memset(text, ' ', indent*2);
                unparse_bytecode(line->bytecode, line->length, text+code_indent*indent*2);
                if (line->comment_length) {
                    if (*line->bytecode != code_norem) {
                        strcat(text, "  ");
                    }
                    strcat(text, "//");
                    strcat(text, (char *)line->bytecode+line->length);
                }
                if (profile) {
                    bucket = line_number>>profile_shift;
                    if (bucket >= profiled_buckets) {
                        phits = 0;
                        for (p = profiled_buckets; p <= bucket; p++) {
                            phits += profile_buckets[p].hits;
                        }
                        profiled_buckets = bucket+1;
                        printf("%7ldms %4d %s\n", phits, line_number, text);
                    } else {
                        printf("          %4d %s\n", line_number, text);
                    }
                } else {
                    if (code_numbers) {
                        printf("%4d %s\n", line_number, text);
                    } else {
                        printf("%s\n", text);
                    }
                }
            }
            if (end_line_number && line_number > end_line_number) {
                break;
            }
            if (line_number > start_line_number && end_line_number == 0x7fffffff && code == code_endsub) {
                break;
            }
            if (code == code_if || code == code_else || code == code_elseif || code == code_while || code == code_do ||code == code_for || code == code_sub) {
                indent++;
            }
        } else {
            if (! start_line_number || start_line_number != end_line_number) {
                if (indent) {
                    printf("missing block ends?\n");
                }
                printf("end\n");
            }
            break;
        }
    }
}

// this function recalls a line to be edited by teh FTDI transport
// command line editor.
void
code_edit(int line_number_in)
{
    int n;
    int line_number;
    struct line *line;
    char text[BASIC_OUTPUT_LINE_SIZE+10];  // REVISIT -- line number size?

    line_number = line_number_in-1;
    line = code_next_line(false, false, &line_number);
    if (! line || line_number != line_number_in) {
        return;
    }

    n = sprintf(text, "%d ", line_number);
    unparse_bytecode(line->bytecode, line->length, text+n);
    if (line->comment_length) {
        if (*line->bytecode != code_norem) {
            strcat(text, "  ");
        }
        strcat(text, "//");
        strcat(text, (char *)line->bytecode+line->length);
    }
#if ! STICK_GUEST
    terminal_edit(text);
    main_edit = true;
#endif
}

// *** flash control and access ***

static byte *alternate_flash_code_page;
static int alternate_flash_code_page_offset;

// this function erases the alternate code page in flash memory.
static
void
code_erase_alternate(void)
{
    // determine the alternate flash page
    assert(FLASH_CODE_PAGE == FLASH_CODE1_PAGE || FLASH_CODE_PAGE == FLASH_CODE2_PAGE);
    alternate_flash_code_page = (FLASH_CODE_PAGE == FLASH_CODE1_PAGE) ? FLASH_CODE2_PAGE : FLASH_CODE1_PAGE;
    alternate_flash_code_page_offset = 0;

    // erase the alternate flash page
    assert(BASIC_LARGE_PAGE_SIZE >= FLASH_PAGE_SIZE && ! (BASIC_LARGE_PAGE_SIZE%FLASH_PAGE_SIZE));
    flash_erase_pages((uint32 *)alternate_flash_code_page, BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);
}

// this function updates the alternate code page in flash memory.
static
bool
code_append_line_to_alternate(const struct line *line)
{
    int size;
    int nwords;

    size = line->size;

    // if this is not the special end line...
    if (line->line_number) {
        // we always leave room for the special end line
        if ((int)(alternate_flash_code_page_offset+size) > (int)(BASIC_LARGE_PAGE_SIZE-sizeof(uint32)-LINESIZE)) {
            return false;
        }
    }

    assert(! (size%sizeof(uint32)));
    nwords = size/sizeof(uint32);

    // copy ram or flash to flash
    assert(FLASH_CODE_PAGE != alternate_flash_code_page);
    assert(nwords && ! (alternate_flash_code_page_offset%sizeof(uint32)));
    flash_write_words((uint32 *)(alternate_flash_code_page+alternate_flash_code_page_offset), (uint32 *)line, nwords);

    alternate_flash_code_page_offset += size;
    assert(! (alternate_flash_code_page_offset%sizeof(uint32)));

    return true;
}

const static struct line empty = { LINESIZE, 0, 0, 0 };

// this function promotes the alternate code page in flash memory
// to become current.
static
void
code_promote_alternate(bool fast)
{
    bool boo;
    int32 generation;

    assert(FLASH_CODE_PAGE != alternate_flash_code_page);

    // append an end line to the alternate flash bank
    boo = code_append_line_to_alternate(&empty);
    assert(boo);

    // update the generation of the alternate page, to make it primary!
    generation = LGENERATION(FLASH_CODE_PAGE)+1;
    flash_write_words((uint32 *)(alternate_flash_code_page+BASIC_LARGE_PAGE_SIZE-sizeof(uint32)), (uint32 *)&generation, 1);

    assert(FLASH_CODE_PAGE == alternate_flash_code_page);
    assert(LGENERATION(FLASH_CODE1_PAGE) != LGENERATION(FLASH_CODE2_PAGE));

    if (! fast) {
        delay(500);  // this always takes a while!
    }
}

// this function saves/merges the current program from ram to flash.
void
code_save(bool fast, int renum)
{
    bool boo;
    int line_number;
    int line_renumber;
    struct line *line;
    struct line *ram_line;
    byte ram_line_buffer[sizeof(struct line)-VARIABLE+BASIC_BYTECODE_SIZE];

    // blow our LRU cache
    last_seq_line = NULL;
    last_seq_line_number = 0;
    last_nonseq_line = NULL;
    last_nonseq_line_number = 0;

    // erase the alternate flash bank
    code_erase_alternate();

    // we'll build lines in ram
    ram_line = (struct line *)ram_line_buffer;

    // for all lines...
    boo = true;
    line_number = 0;
    line_renumber = 0;
    for (;;) {
        line = code_next_line(true, false, &line_number);
        if (! line) {
            break;
        }
        line_number = line->line_number;

        // if the line is not deleted...
        if (line->length != 1 || line->bytecode[0] != code_deleted) {
            // copy the line to ram
            ASSERT((unsigned)line->size <= sizeof(ram_line_buffer));
            memcpy(ram_line, line, line->size);

            // if we're renumbering lines...
            if (renum) {
                ram_line->line_number = renum+line_renumber;
                line_renumber += 10;
            }

            // append the line to the alternate flash bank
            boo = code_append_line_to_alternate(ram_line);
            if (! boo) {
                break;
            }
        }
    }

    // if we saved all lines successfully...
    if (boo) {
        // promote the alternate flash bank
        code_promote_alternate(fast);

        // clear ram
        memset(RAM_CODE_PAGE, 0, sizeof(RAM_CODE_PAGE));
        *(struct line *)RAM_CODE_PAGE = empty;
    } else {
        printf("out of code flash\n");
    }

    // prepare for profiling
    code_clear2();
}

// this function erases the current program in flash.
void
code_new(void)
{
    // erase the alternate flash bank
    code_erase_alternate();

    // promote the alternate flash bank
    code_promote_alternate(false);

    // clear ram
    memset(RAM_CODE_PAGE, 0, sizeof(RAM_CODE_PAGE));
    *(struct line *)RAM_CODE_PAGE = empty;

    // clear variables
    run_clear(true);
}

// this function erases code ram, reverting to the code in flash.
void
code_undo()
{
    // clear ram
    memset(RAM_CODE_PAGE, 0, sizeof(RAM_CODE_PAGE));
    *(struct line *)RAM_CODE_PAGE = empty;
}

// this function prints code memory usage.
void
code_mem(void)
{
    int n;
    struct line *eop;

    eop = find_exact_line_in_page(RAM_CODE_PAGE, 0);
    assert(eop);
    n = (byte *)eop-RAM_CODE_PAGE;
    printf("%3d%% ram code bytes used%s\n", n*100/(sizeof(RAM_CODE_PAGE)-LINESIZE), n?" (unsaved changes!)":"");

    eop = find_exact_line_in_page(FLASH_CODE_PAGE, 0);
    assert(eop);
    n = (byte *)eop-FLASH_CODE_PAGE;
    printf("%3d%% flash code bytes used\n", n*100/(BASIC_LARGE_PAGE_SIZE-sizeof(uint32)-LINESIZE));
}

// *** filesystem ***

struct catalog {
    char name[BASIC_STORES][15];  // 14 character name
    int dummy;
};

// find the library if it exists
static
void
find_library(void)
{
    int i;
    struct catalog *catalog;

    code_library_page = NULL;

    catalog = (struct catalog *)FLASH_CATALOG_PAGE;

    // look up the library name in the catalog
    for (i = 0; i < BASIC_STORES; i++) {
        if (! strncmp(catalog->name[i], "library", sizeof(catalog->name[i])-1)) {
            break;
        }
    }
    if (i < BASIC_STORES) {
        // we'll run with a library code space
        code_library_page = FLASH_STORE_PAGE(i);
    }
}

// this function stores the current program to the filesystem.
void
code_store(char *name)
{
    int i;
    int e;
    int n;
    struct catalog temp;
    struct catalog *catalog;

    // update the primary flash
    code_save(false, 0);

    if (! *name) {
        return;
    }

    memcpy(&temp, FLASH_CATALOG_PAGE, sizeof(temp));
    catalog = (struct catalog *)&temp;

    // look up the name in the catalog
    e = -1;
    for (i = 0; i < BASIC_STORES; i++) {
        if (catalog->name[i][0] == (char)0xff || catalog->name[i][0] == (char)0) {
            e = i;
        }
        if (! strncmp(catalog->name[i], name, sizeof(catalog->name[i])-1)) {
            break;
        }
    }

    // if we didn't find a match...
    if (i == BASIC_STORES) {
        if (e == -1) {
            printf("out of storage\n");
            return;
        }
        n = e;
    } else {
        n = i;
    }

    // erase the store flash
    flash_erase_pages((uint32 *)FLASH_STORE_PAGE(n), BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);

    // if the name doesn't yet exist...
    if (i == BASIC_STORES) {
        // update the catalog
        // revisit -- this is not crash-update-safe
        strncpy(catalog->name[n], name, sizeof(catalog->name[n])-1);
        catalog->name[n][sizeof(catalog->name[n])-1] = '\0';
        flash_erase_pages((uint32 *)FLASH_CATALOG_PAGE, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);
        flash_write_words((uint32 *)FLASH_CATALOG_PAGE, (uint32 *)&temp, sizeof(temp)/sizeof(uint32));
    }

    // copy the primary flash to the store flash
    flash_write_words((uint32 *)FLASH_STORE_PAGE(n), (uint32 *)FLASH_CODE_PAGE, BASIC_LARGE_PAGE_SIZE/sizeof(uint32));

    find_library();
}

// this function loads the current program from the filesystem.
void
code_load(char *name)
{
    int i;
    int32 generation;
    byte *code_page;
    struct catalog *catalog;

    // update the primary flash and clear ram
    code_save(false, 0);

    catalog = (struct catalog *)FLASH_CATALOG_PAGE;

    // look up the name in the catalog
    for (i = 0; i < BASIC_STORES; i++) {
        if (! strncmp(catalog->name[i], name, sizeof(catalog->name[i])-1)) {
            break;
        }
    }

    // if we didn't find a match...
    if (i == BASIC_STORES) {
        printf("program '%s' not found\n", name);
        return;
    }

    // revisit -- this is not crash-update-safe
    code_page = FLASH_CODE_PAGE;
    generation = LGENERATION(code_page);

    // erase the primary flash
    flash_erase_pages((uint32 *)code_page, BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);

    // copy the store flash to the primary flash (except the generation)
    flash_write_words((uint32 *)code_page, (uint32 *)FLASH_STORE_PAGE(i), BASIC_LARGE_PAGE_SIZE/sizeof(uint32)-1);

    // then restore the generation
    flash_write_words((uint32 *)(code_page+BASIC_LARGE_PAGE_SIZE-sizeof(uint32)), (uint32 *)&generation, 1);
}

// this function displays the programs in the filesystem.
void
code_dir()
{
    int i;
    struct catalog *catalog;

    catalog = (struct catalog *)FLASH_CATALOG_PAGE;

    // look up all names in the catalog
    for (i = 0; i < BASIC_STORES; i++) {
        if (catalog->name[i][0] == (char)0xff || catalog->name[i][0] == (char)0) {
            continue;
        }
        printf("%s\n", catalog->name[i]);
    }
}

// this function purges programs from the filesystem.
void
code_purge(char *name)
{
    int i;
    struct catalog temp;
    struct catalog *catalog;

    memcpy(&temp, FLASH_CATALOG_PAGE, sizeof(temp));
    catalog = (struct catalog *)&temp;

    // look up the name in the catalog
    for (i = 0; i < BASIC_STORES; i++) {
        if (! strncmp(catalog->name[i], name, sizeof(catalog->name[i])-1)) {
            break;
        }
    }

    // if we didn't find a match...
    if (i == BASIC_STORES) {
        printf("program '%s' not found\n", name);
        return;
    }

    // update the catalog
    // revisit -- this is not crash-update-safe
    strncpy(catalog->name[i], "", sizeof(catalog->name[i])-1);
    flash_erase_pages((uint32 *)FLASH_CATALOG_PAGE, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);
    flash_write_words((uint32 *)FLASH_CATALOG_PAGE, (uint32 *)&temp, sizeof(temp)/sizeof(uint32));

    delay(500);  // this always takes a while!

    find_library();
}

void
code_timer_poll(void)
{
    int bucket;

    // if we can profile...
    // N.B. this works if the code is all in the same page -- i.e., saved
    if (! ((struct line *)RAM_CODE_PAGE)->line_number) {
        if (run_in_library) {
            profile_library++;
        } else if (run_line_number == -1) {
            profile_other++;
        } else {
            bucket = run_line_number >> profile_shift;
            assert(bucket < PROFILE_BUCKETS);
            profile_buckets[bucket].hits++;
        }
    }
}

void code_clear2(void)
{
    int div;
    int shift;
    struct line *line;
    struct line *last_line;

    // if we can profile...
    // N.B. this works if the code is all in the same page -- i.e., saved
    if (! ((struct line *)RAM_CODE_PAGE)->line_number) {
        // clear the profile buffer
        memset(PROFILE_BUFFER, 0, PROFILE_BUFFER_SIZE);
        profile_other = 0;
        profile_library = 0;

        // find the last line of code
        last_line = NULL;
        for (line = find_first_line_in_page(FLASH_CODE_PAGE); line; line = find_next_line_in_page(FLASH_CODE_PAGE, line)) {
            if (! line->line_number) {
                break;
            }
            last_line = line;
        }

        if (last_line) {
            // compute the shift from line number to bucket number
            div = (last_line->line_number+PROFILE_BUCKETS-1)/PROFILE_BUCKETS;
            for (shift = 0; 1<<shift < div; shift++) {
                // NULL
            }
            profile_shift = shift;
        } else {
            profile_shift = 0;
        }
    }
}

// this function initializes the code module.
void
code_initialize(void)
{
    assert(! (FLASH_PAGE_SIZE%sizeof(uint32)));
    assert(! (BASIC_SMALL_PAGE_SIZE%FLASH_PAGE_SIZE));
    assert(! (BASIC_LARGE_PAGE_SIZE%FLASH_PAGE_SIZE));

    // if this is our first boot since reflash...
    if (*(int *)FLASH_CODE_PAGE == -1) {
        code_new();
        assert(find_first_line_in_page(FLASH_CODE_PAGE));
    } else {
        // clear ram
        memset(RAM_CODE_PAGE, 0, sizeof(RAM_CODE_PAGE));
        *(struct line *)RAM_CODE_PAGE = empty;
    }

    profile_buckets = (struct bucket *)PROFILE_BUFFER;

    find_library();
}

