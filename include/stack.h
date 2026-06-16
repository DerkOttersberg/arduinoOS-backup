#ifndef STACK_H
#define STACK_H

#include <Arduino.h>

#define STACKSIZE 64

// Basic byte operations
void pushByte(byte b, int procesID);
byte popByte(int procesID);

// Push functions for data types
void pushInt(int data, int procesID);
void pushFloat(float data, int procesID);
void pushChar(char data, int procesID);
void pushString(char *data, int procesID);

// Pop functions for data types
int popInt(int procesID);
float popFloat(int procesID);
char popChar(int procesID);
char *popString(int procesID);
int popNumber(int procesID);
float popVal(int procesID);

void showStack(int procesID);
#endif