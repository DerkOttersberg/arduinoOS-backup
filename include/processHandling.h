#ifndef PROCESHANDELING_H
#define PROCESHANDELING_H
#include "stack.h"
#include <Arduino.h>
#define MAX_PROCES 10

struct procesEntry
{
    char name[12];
    int procesID;
    byte state;
    int programCounter;
    int programLength;
    int beginningAdres;
    int filePointer;
    int loopAdres;
    byte stack[STACKSIZE];
    int sp;
};
extern procesEntry procesTable[MAX_PROCES]; //process id en welk nummer het heeft
void run(const char *arg);
void suspend(const char *arg);
void resume(const char *arg);
void kill(const char *arg);
void list(const char *arg);
void removeProcesFromList(int procesIndex);
void changeState(int ID, byte desiredState);
int findID(int ID);
int findProces(char *name);
void handleDataTypes(char type, int procesIndex);
void runProcesses();
void execute(int i);
void resetProcesses();
#endif
