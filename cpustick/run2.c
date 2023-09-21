// *** run2.c *********************************************************
// this file implements private extensions to the stickos bytecode
// execution engine.

#include "main.h"

// this function executes a private bytecode statement.
bool  // end
run2_bytecode_code(uint code, const byte *bytecode, int length)
{
    bool end;
    int index;

    end = false;

    index = 0;

    switch (code) {

        default:
            assert(0);
            break;
    }

    assert(index == length);
    return end;

//XXX_SKIP_XXX:
    stop();
    return false;
}

