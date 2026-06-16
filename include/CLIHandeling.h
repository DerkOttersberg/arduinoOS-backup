#ifndef CLIHANDELING_H
#define CLIHANDELING_H
#define BUFSIZE 64

// Prototype functions
bool readToken(char *buffer);
const char **seperateArgCommand(char *input);
#endif