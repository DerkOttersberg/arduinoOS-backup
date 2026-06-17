#ifndef MEMORYHANDELING_H
#define MEMORYHANDELING_H

#include <Arduino.h>
#include "stack.h"

// Struct voor geheugeninvoer
struct memoryEntry
{
    byte name;    // 1 karakter naam
    int procesID; // bijhorend proces-ID
    byte type;    // type van de variabele: 'C', 'i', 'F', 'S'
    byte length;  // lengte in bytes
    int adres;    // startadres in het geheugen
};

// Functies
void setVar(byte name, int ID);
void getVar(byte name, int ID);
int findVariable(byte name, int ID);
int findAvailableSpace(int newVarLength);
void deleteProcesVars(int ID);
void showMemoryTable();
void showMemory();
#endif
