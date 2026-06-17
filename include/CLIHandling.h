#ifndef CLIHANDLING_H
#define CLIHANDLING_H
#define BUFSIZE 64

// Prototype functions
bool readToken(char *buffer);
const char **separateArgCommand(char *input);
#endif
