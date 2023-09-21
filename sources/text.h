// compress a nul-terminated line of text
void
text_compress(
    IN char *text,
    OUT byte *buffer
    );

// expand a nul-terminated line of text
void
text_expand(
    IN byte *buffer,
    OUT char *text
    );
