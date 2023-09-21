// *** parse.h ********************************************************

#define COMMENT  "//"
#define COMMENTLEN  2

void parse_trim(IN char **p);
bool parse_char(IN OUT char **text, IN char c);
bool parse_word(IN OUT char **text, IN const char *word);
bool parse_wordn(IN OUT char **text, IN const char *word);
bool parse_words(IN OUT char **text_in, IN OUT char **text_err, IN const char *words);
char *parse_find_keyword(IN OUT char *text, IN char* word);
bool parse_find_tail(IN OUT char *text, IN char *tail);
char *parse_match_paren(char *p);
char *parse_match_quote(char *p);

bool parse_const(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
bool parse_word_const_word(IN char *prefix, IN char *suffix, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_const(byte code OPTIONAL, byte *bytecode_in, char **out);
int unparse_word_const_word(char *prefix, char *suffix, byte *bytecode_in, char **out);

bool parse_var(IN bool string, IN bool lvalue, IN int indices, IN int obase, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_var(IN bool string, IN bool lvalue, IN int indices, byte *bytecode_in, char **out);

bool parse_expression(IN int obase, IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_expression(int tbase, byte *bytecode_in, int length, char **out);

bool parse_string(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_string(byte *bytecode_in, int length, char **out);

bool parse_relation(IN OUT char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_relation(byte *bytecode_in, int length, char **out);

bool parse_class(IN char *text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_class(byte *bytecode, OUT bool *string);

bool parse_string_or_expression(IN bool string, IN char **text, IN OUT int *length, IN OUT byte *bytecode);
int unparse_string_or_expression(IN bool string, byte *bytecode, int length, char **out);


bool parse_line(IN char *text, OUT int *length, OUT byte *bytecode, OUT int *syntax_error);
bool parse_line_code(IN byte code, IN char *text, OUT int *length, OUT byte *bytecode, OUT int *syntax_error);
void unparse_bytecode(IN byte *bytecode, IN int length, OUT char *text);
void unparse_bytecode_code(IN byte code, IN byte *bytecode, IN int length, OUT char *text);

