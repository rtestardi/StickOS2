// *** run2.c *********************************************************
// this file implements private extensions to the stickos bytecode
// execution engine.

#include "main.h"

// this function executes a private bytecode statement.
bool  // end
run2_bytecode_code(uint code, const byte *bytecode, int length)
{
    int n;
    bool end;
    int index;
    uint code2;

    end = false;

    index = 0;

    switch (code) {
        case code_wave:
            code2 = bytecode[index++];
            index += run_expression(bytecode+index, length-index, NULL, &n);
            if (n < 0 || n > 10000000) {
                printf("bad frequency\n");
                goto XXX_SKIP_XXX;
            }
#if ! STICK_GUEST
            if (run_condition) {
                switch (code2) {
                    case 0:
                        wave("square", n);
                        break;
                    case 1:
                        wave("ekg", n);
                        break;
                    case 2:
                        wave("sine", n);
                        break;
                    case 3:
                        wave("triangle", n);
                        break;
                    default:
                        assert(0);
                        break;
                }
            }
#endif
            break;

        default:
            assert(0);
            break;
    }

    assert(index == length);
    return end;

XXX_SKIP_XXX:
    stop();
    return false;
}

