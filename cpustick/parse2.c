// *** parse2.c *******************************************************
// this file implements private extensions to the stickos bytecode
// compiler and de-compiler.

#include "main.h"

static
const struct keyword {
    char *keyword;
    enum bytecode2 code;
} keywords[] = {
    "private", code_private,
};


// this function parses (compiles) a private statement line to bytecode.
bool
parse2_line(IN char *text_in, OUT int *length_out, OUT byte *bytecode, OUT int *syntax_error_in)
{
    int i;
    int len;
    char *text;
    int length;

    text = text_in;
    parse_trim(&text);

    for (i = 0; i < LENGTHOF(keywords); i++) {
        len = strlen(keywords[i].keyword);
        if (! strncmp(text, keywords[i].keyword, len)) {
            text += len;
            break;
        }
    }
    if (i == LENGTHOF(keywords) || i == code_private) {
        goto XXX_ERROR_XXX;
    }

    parse_trim(&text);

    length = 0;
    bytecode[length++] = keywords[i].code;

    switch (keywords[i].code) {
        default:
            assert(0);
            break;
    }

    if (*text) {
        goto XXX_ERROR_XXX;
    }

    *length_out = length;
    return true;

XXX_ERROR_XXX:
    *syntax_error_in = text - text_in;
    assert(*syntax_error_in >= 0 && *syntax_error_in < BASIC_OUTPUT_LINE_SIZE);
    return false;
}

// this function unparses (de-compiles) a private statement line from bytecode.
void
unparse2_bytecode(IN byte *bytecode_in, IN int length, OUT char *text)
{
    int i;
    byte code;
    char *out;
    byte *bytecode;

    bytecode = bytecode_in;
    out = text;

    code = *bytecode++;

    // find the bytecode keyword
    for (i = 0; i < LENGTHOF(keywords); i++) {
        if (keywords[i].code == code) {
            break;
        }
    }
    assert(i != LENGTHOF(keywords));

    // decompile the bytecode keyword
    out += sprintf(out, keywords[i].keyword);
    out += sprintf(out, " ");

    switch (code) {
        default:
            assert(0);
            break;
    }

    assert(bytecode == bytecode_in+length);
}

