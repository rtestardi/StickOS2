// *** parse2.h *******************************************************

bool parse2_line(IN char *text, OUT int *length, OUT byte *bytecode, OUT int *syntax_error);
void unparse2_bytecode(IN byte *bytecode, IN int length, OUT char *text);

