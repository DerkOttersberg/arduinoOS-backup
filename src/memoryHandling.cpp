#include "memoryHandeling.h"
#define MAX_VARS 16
#define MEM_SIZE 128

byte memory[MEM_SIZE];
memoryEntry memoryTable[MAX_VARS];
byte noOfVars = 0;
void setVar(byte name, int ID)
{
    // Serial.print(F("variable name: "));
    // Serial.println(name);
    if (noOfVars == MAX_VARS)
    {
        Serial.println(F("Maximum amount of variable has been reached"));
        return;
    }
    if (name == 0)
    {
        Serial.println(F("Name is NULL"));
        return;
    }
    int variableIndex = findVariable(name, ID);
    // Serial.println(variableIndex);
    if (variableIndex != -1)
    {
        //  erase variable out of memory table
        for (int i = variableIndex; i < noOfVars - 1; i++)
        {
            memoryTable[i] = memoryTable[i + 1];
        }
        noOfVars--;
    }

    // meta data
    byte type = popByte(ID);
    byte length;

    if (type == 'I')
    {
        length = 2;
    }
    else if (type == 'F')
    {
        length = 4;
    }
    else if (type == 'C')
    {
        length = 1;
    }
    else if (type == 'S')
    {
        length = popByte(ID);
    }
    else
    {
        Serial.print("Fout: onbekend type");
        Serial.println(type);
        return;
    }

    int avaliableSpaceIndex = findAvailableSpace(length);

    if (avaliableSpaceIndex == -1)
    {
        Serial.println(F("Not enough space left."));
        return;
    }

    //  new memoryEntry
    memoryEntry newVar;
    // strncpy(newVar.name, name, sizeof(newVar.name));
    newVar.name = name;
    newVar.type = type;
    newVar.procesID = ID;
    newVar.length = length;
    newVar.adres = avaliableSpaceIndex;

    memoryTable[noOfVars] = newVar;
    noOfVars++;
    for (int i = length; i > 0; i--)
    {
        byte bit = popByte(ID);
        // Serial.print(bit);
        memory[avaliableSpaceIndex - 1 + i] = bit;
    }
}
void getVar(byte name, int ID)
{
    // showMemory();
    int variableIndex = findVariable(name, ID);
    if (variableIndex == -1)
    {
        Serial.println(F("Variable not found"));
    }
    // meta data from var
    byte type = memoryTable[variableIndex].type;
    byte length = memoryTable[variableIndex].length;
    int adres = memoryTable[variableIndex].adres;

    for (int i = 0; i < length; i++)
    {
        pushByte(memory[adres + i], ID);
    }

    if (type == 'S')
    {
        pushByte(length, ID);
    }
    pushByte(type, ID);
}
int findVariable(byte name, int ID)
{
    for (size_t i = 0; i < noOfVars; i++)
    {
        if (memoryTable[i].name == name && memoryTable[i].procesID == ID)
        {
            return i;
        }
    }
    return -1;
}

int findAvailableSpace(int newVarLength)
{
    if (noOfVars == 0)
    {
        return 0;
    }
    if (noOfVars == 1)
    {
        return memoryTable[0].adres + memoryTable[0].length;
    }

    for (size_t i = 0; i < noOfVars; i++)
    {
        int difference = memoryTable[i + 1].adres - (memoryTable[i].adres + memoryTable[i].length);
        if (difference >= newVarLength)
        {
            return memoryTable[i].adres + memoryTable[i].length;
        }
    }
    // Check space after last variable
    int lastEnd = memoryTable[noOfVars - 1].adres + memoryTable[noOfVars - 1].length;
    if (MEM_SIZE - lastEnd >= newVarLength)
    {
        return lastEnd;
    }

    //  no space left
    return -1;
}

void deleteProcesVars(int procesID)
{
    for (int i = 0; i < noOfVars; i++)
    {
        if (memoryTable[i].procesID == procesID)
        {
            // Verwijder entry door alles een plek naar voren te schuiven
            for (int j = i; j < noOfVars - 1; j++)
            {
                memoryTable[j] = memoryTable[j + 1];
            }
            noOfVars--;
            i--; // index terug zodat we de nieuwe entry op positie i niet overslaan
        }
    }
}

void showMemory()
{
    for (size_t i = 0; i < noOfVars; i++)
    {
        memoryEntry var = memoryTable[i];
        Serial.print("Var: ");
        Serial.print((char)var.name);
        Serial.print(", PID: ");
        Serial.print(var.procesID);
        Serial.print(", Type: ");
        Serial.print((char)var.type);
        Serial.print(", Lengte: ");
        Serial.print(var.length);
        Serial.print(", Data: ");
        for (int j = 0; j < var.length; j++)
        {
            Serial.print(memory[var.adres + j], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
}
void showMemoryTable()
{
    for (size_t i = 0; i < noOfVars; i++)
    {
        memoryEntry var = memoryTable[i];
        Serial.print("Var: ");
        Serial.print((char)var.name);
        Serial.print(", PID: ");
        Serial.print(var.procesID);
        Serial.print(", Type: ");
        Serial.print((char)var.type);
        Serial.print(", Lengte: ");
        Serial.print(var.length);
        Serial.print(", Data: ");
    }
}