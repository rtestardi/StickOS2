#if __unix__
#include <signal.h>
#else
#include <windows.h>
#include <stdio.h>
#endif
#include "main.h"

int serial_baudrate;
bool zb_present = true;
bool main_prompt = true;
bool terminal_echo = true;
volatile int32 terminal_getchar;
char *volatile main_command;

byte big_buffer[768];

void
flash_erase_pages(uint32 *addr, uint32 npages)
{
    memset(addr, 0xff, npages*FLASH_PAGE_SIZE);
}

void
flash_write_words(uint32 *addr, uint32 *data, uint32 nwords)
{
    memcpy(addr, data, nwords*sizeof(int));
}

void
flash_upgrade(uint32 fsys_frequency)
{
}

void
clone(bool and_run)
{
}

#define BASIC_INPUT_LINE_SIZE  72

void
terminal_command_error(int offset)
{
    int i;
    char buffer[2+BASIC_INPUT_LINE_SIZE+1];

    assert(offset < BASIC_INPUT_LINE_SIZE);

    offset += 2;

    if (offset >= 10) {
        strcpy(buffer, "error -");
        for (i = 7; i < offset; i++) {
            buffer[i] = ' ';
        }
        buffer[i++] = '^';
        assert(i < sizeof(buffer));
        buffer[i] = '\0';
    } else {
        for (i = 0; i < offset; i++) {
            buffer[i] = ' ';
        }
        buffer[i++] = '^';
        assert(i < sizeof(buffer));
        buffer[i] = '\0';
        strcat(buffer, " - error");
    }
    printf("%s\n", buffer);
}

static
#if __unix__
void
#else
BOOL
#endif
ctrlc(int event)
{
#if __unix__
    stop();
#else
    if (event) {
        return 0;
    } else {
        stop();
        return 1;
    }
#endif
}

uint total_in;
uint total_out;

int
generate_help(
    void
    )
{
    int c;
    int i;
    int n;
    bool on;
    char *line;
    char line1[1024];
    byte line2[1024];
    char line3[1024];

    on = false;
    while (line = line1, gets(line)) {
        if (strstr(line, "GENERATE_HELP_BEGIN")) {
            on = true;
        }
        if (strstr(line, "GENERATE_HELP_END")) {
            break;
        }
        if (on) {
            n = strlen(line);
            if (line[0] == '"' && line[n-1] == '"') {
                line[n-1] = '\0';
                line++;

                text_compress(line, line2);
                text_expand(line2, line3);

                total_in += strlen(line);
                total_out += strlen(line2);

                for (i = 0; line[i] || line3[i]; i++) {
                    assert(line[i] == line3[i]);
                }

                printf("\"");
                for (i = 0; (c = line2[i]); i++) {
                    if (isprint(c)) {
                        printf("%c", c);
                    } else if (c == '"') {
                        printf("\\\"");
                    } else if (c == '\\') {
                        printf("\\\\");
                    } else if (c == '\n') {
                        printf("\\n");
                    } else {
                        printf("\\%03o", c);
                    }
                }
                printf("\"\n");
            } else {
                // N.B. our printf() is limited to 80 characters
                fprintf(stdout, "%s\n", line);
                fflush(stdout);
            }
        }
    }
    return 0;
}

int
main(int argc, char **argv)
{
    int i;
    char text[2*BASIC_INPUT_LINE_SIZE];

    if (argc > 1 && ! strcmp(argv[1], "help")) {
        return generate_help();
    }

#if __unix__
    signal(SIGINT, ctrlc);
#else
    SetErrorMode(0);
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ctrlc, true);
#endif

    flash_erase_pages((uint32 *)FLASH_CODE1_PAGE, BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);
    flash_erase_pages((uint32 *)FLASH_CODE2_PAGE, BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);
    for (i = 0; i < BASIC_STORES; i++) {
        flash_erase_pages((uint32 *)FLASH_STORE_PAGE(i), BASIC_LARGE_PAGE_SIZE/FLASH_PAGE_SIZE);
    }
    flash_erase_pages((uint32 *)FLASH_CATALOG_PAGE, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);
    flash_erase_pages((uint32 *)FLASH_PARAM1_PAGE, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);
    flash_erase_pages((uint32 *)FLASH_PARAM2_PAGE, BASIC_SMALL_PAGE_SIZE/FLASH_PAGE_SIZE);

    timer_initialize();
    basic_initialize();

    if (argc == 1) {
        basic0_run("help about");
    }
    for (;;) {
        if (isatty(0) && main_prompt) {
            write(1, "> ", 2);
        }
        if (! gets(text)) {
            break;
        }
        text[BASIC_INPUT_LINE_SIZE-1] = '\0';
        basic0_run(text);
    }
    return 0;
}

