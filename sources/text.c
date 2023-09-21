// *** text.c *********************************************************
// this file implements generic text handling functions.

#include "main.h"

// compression
// 0x01 - 0x1f -> add 0x20 and follow with a space (use 0x7f in place of \n)
// 0x20 - 0x7e -> literal
// 0x80 - 0xbf -> subtract 0x40 and follow with a space
// 0xc0 - 0xff -> generate 2+(c-0xc0) spaces

#if _WIN32
void
text_compress(
    IN char *text,
    OUT byte *buffer
    )
{
    int c;

    while ((c = *text++)) {
        if (c >= 0x40 && c < 0x80 && text[0] == ' ' && text[1] != ' ') {
            c += 0x40;
            assert(c >= 0x80 && c < 0xc0);
            text++;
        } else if (c > 0x20 && c < 0x40 && text[0] == ' ' && text[1] != ' ') {
            c -= 0x20;
            if (c == '\n') {
                c = 0x7f;
            }
            assert((c != '\n' && (c > 0 && c < 0x20)) || c == 0x7f);
            text++;
        } else if (c == ' ' && text[0] == ' ') {
            text++;
            c = 0xc0;
            while (*text == ' ' && c < 0xff) {
                text++;
                c++;
            }
            assert(c >= 0xc0 && c < 0x100);
        } else {
            assert(c == '\n' || (c >= 0x20 && c < 0x7f));
        }
        *buffer++ = c;
    }
    *buffer = '\0';
}
#endif

void
text_expand(
    IN byte *buffer,
    OUT char *text
    )
{
    int c;

    while (*buffer) {
        c = *buffer++;  // CW bug
        if (c >= 0x80 && c < 0xc0) {
            *text++ = c-0x40;
            *text++ = ' ';
        } else if ((c != '\n' && (c > 0 && c < 0x20)) || c == 0x7f) {
            if (c == 0x7f) {
                c = '\n';
            }
            *text++ = c+0x20;
            *text++ = ' ';
        } else if (c >= 0xc0 && c < 0x100) {
            *text++ = ' ';
            *text++ = ' ';
            while (c > 0xc0) {
                *text++ = ' ';
                c--;
            }
        } else {
            assert(c == '\n' || (c >= 0x20 && c < 0x7f));
            *text++ = c;
        }
    }
    *text = '\0';
}

