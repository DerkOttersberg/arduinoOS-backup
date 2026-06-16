#include "procesHandeling.h"
#include "memoryHandeling.h"
#include "EEPROMHandeling.h"
#include "commands.h"
#include "instruction_set.h"
#include <math.h>
int noOfProces = 0;
int procesID = 0;
procesEntry procesTable[MAX_PROCES];

void resetProcesses()
{
    for (int i = 0; i < noOfProces; i++)
    {
        deleteProcesVars(procesTable[i].procesID);
    }

    memset(procesTable, 0, sizeof(procesTable));
    noOfProces = 0;
    procesID = 0;
}

void run(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    if (noOfProces >= MAX_PROCES)
    {
        Serial.println(F("Not enough space for another proces to be started."));
        return;
    }
    int fileIndex = findName(arg);
    if (fileIndex == -1)
    {
        Serial.println(F("No file exists with that name"));
        return;
    }

    fileInfo file = readFATEntry(fileIndex);

    strncpy(procesTable[noOfProces].name, arg, sizeof(procesTable[noOfProces].name) - 1);
    procesTable[noOfProces].name[sizeof(procesTable[noOfProces].name) - 1] = '\0';
    int assignedProcessID = procesID++;
    procesTable[noOfProces].procesID = assignedProcessID;
    procesTable[noOfProces].state = 'r';
    procesTable[noOfProces].programCounter = file.position;
    procesTable[noOfProces].programLength = file.length;
    procesTable[noOfProces].beginningAdres = file.position;
    procesTable[noOfProces].loopAdres = file.position;

    // Initialize stack to zero for safety
    memset(procesTable[noOfProces].stack, 0, STACKSIZE);
    procesTable[noOfProces].sp = 0;
    noOfProces++;

    //  hele stack in proces table
    //  stack grote 32

    Serial.println(F("Process succesfully started. whith proces ID"));
    Serial.println(assignedProcessID);
}

void suspend(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    if (!isDigit(arg[0]))
    {
        Serial.println(F("Given argument is not a valid integer."));
        return;
    }
    Serial.println(arg);
    int procesIndex = findID(atoi(arg));
    if (procesIndex == -1)
    {
        Serial.println(F("Given process ID does not exist"));
        return;
    }

    if (procesTable[procesIndex].state == '0')
    {
        Serial.println(F("Proces already terminated"));
        return;
    }

    changeState(procesIndex, 'p');
}
void resume(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    if (!isDigit(arg[0]))
    {
        Serial.println(F("Given argument is not a valid integer."));
        return;
    }
    int procesIndex = findID(atoi(arg));
    if (procesIndex == -1)
    {
        Serial.println(F("Given process ID does not exist"));
        return;
    }

    if (procesTable[procesIndex].state == '0')
    {
        Serial.println(F("Proces already terminated"));
        return;
    }

    changeState(procesIndex, 'r');
}

void kill(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    if (!isDigit(arg[0]))
    {
        Serial.println(F("Given argument is not a valid integer."));
        return;
    }
    int processIdToKill = atoi(arg);
    int procesIndex = findID(processIdToKill);
    if (procesIndex == -1)
    {
        Serial.println(F("Given process ID does not exist"));
        return;
    }
    // Serial.println(procesIndex);
    changeState(procesIndex, '0');
    deleteProcesVars(processIdToKill);
    removeProcesFromList(procesIndex);

    Serial.println(F("Process terminated"));
}
void list(const char *arg)
{
    Serial.println(noOfProces);
    for (int i = 0; i < noOfProces; i++)
    {
        // if (procesTable[i].state == '0')
        // {
        //     continue;
        // }
        Serial.print(F("ID: "));
        Serial.println(procesTable[i].procesID);
        Serial.print(F("Name: "));
        Serial.println(procesTable[i].name);
        Serial.print(F("State: "));
        Serial.println((char)procesTable[i].state);
    }
}
// helpers functions
void changeState(int ID, byte desiredState)
{
    if (procesTable[ID].state == desiredState)
    {
        Serial.println(F("Proces already in desired state "));
        return;
    }

    procesTable[ID].state = desiredState;
    Serial.println(F("Changed state of proces "));
}

int findID(int ID)
{
    for (int i = 0; i < noOfProces; i++)
    {
        if (procesTable[i].procesID == ID)
        {
            return i;
        }
    }
    return -1;
}
int findProces(char *name)
{
    for (int i = 0; i < noOfProces; i++)
    {
        if (strcmp(name, procesTable[i].name) == 0)
        {
            return i;
        }
    }
    return -1;
}
void removeProcesFromList(int procesIndex)
{

    for (int i = procesIndex; i < noOfProces - 1; i++)
    {
        // move everything back  by 1from the point of hte deleted proces
        procesTable[i] = procesTable[i + 1];
    }
    procesTable[noOfProces - 1] = procesEntry(); // Reset the last entry
    noOfProces--;
}
void handleDataTypes(char type, int procesIndex)
{
    int procesCounter = procesTable[procesIndex].programCounter;
    int procesID = procesTable[procesIndex].procesID;
    int i = 0;
    if (type == 'C')
    {
        pushChar(EEPROM.read(procesCounter + 1), procesID);
        i = 1;
    }
    else if (type == 'I')
    {

        byte MSB = EEPROM.read(procesCounter + 1);
        byte LSB = EEPROM.read(procesCounter + 2);
        int result = word(MSB, LSB);
        pushInt(result, procesID);
        i = 2;
    }
    else if (type == 'F')
    {
        // value
        float value;
        ((byte *)&value)[3] = EEPROM.read(procesCounter + 1);
        ((byte *)&value)[2] = EEPROM.read(procesCounter + 2);
        ((byte *)&value)[1] = EEPROM.read(procesCounter + 3);
        ((byte *)&value)[0] = EEPROM.read(procesCounter + 4);
        pushFloat(value, procesID);
        i = 4;
    }
    else if (type == 'S')
    {
        int length = 0;
        while (EEPROM.read(procesCounter + 1 + length) != '\0')
        {
            length++;
        }

        char result[length + 1];
        for (int j = 0; j < length; j++)
        {
            result[j] = (char)EEPROM.read(procesCounter + 1 + j);
        }
        result[length] = '\0';

        i = length + 1; // include terminating zero; outer PC++ advances past opcode
        pushString(result, procesID);

        // free(result);

        // Serial.println((EEPROM.read(procesCounter +)));
    }
    procesTable[procesIndex].programCounter += i;
}

//
// running processes and executing instructions
//
void runProcesses()
{
    // Serial.println((char)procesTable[0].state);
    // Serial.println(noOfProces);
    for (int i = 0; i < noOfProces; i++)
    {

        execute(i);
    }
}
void execute(int i)
{

    if (procesTable[i].state == 'r')
    {

        int procesCounter = procesTable[i].programCounter;
        int procesID = procesTable[i].procesID;

        byte instruction = EEPROM.read(procesCounter);

        int x;
        int y;
        int z;
        int a;
        int b;

        char charX;

        float floatX;
        float floatY;
        float floatZ;
        float floatA;
        float floatB;

        char popType;
        // Serial.println(procesCounter);
        // Serial.println(beginAdres);
        // Serial.println(instruction);

        // execute 1 byte code per count
        switch (instruction)
        {
        case CHAR:
            // Serial.println(F("Executing CHAR"));

            handleDataTypes('C', i);
            break;

        case INT:
            // Serial.println(F("Executing INT"));
            handleDataTypes('I', i);
            break;

        case STRING:
            // Serial.println(F("Executing STRING"));

            handleDataTypes('S', i);
            break;

        case FLOAT:
            // Serial.println(F("Executing FLOAT"));
            handleDataTypes('F', i);
            break;

        case SET:
            // Serial.println(F("Executing SET"));
            // showStack(procesID);
            setVar(EEPROM.read(++procesTable[i].programCounter), procesID);
            // showMemory();
            break;

        case GET:
            // Serial.println(F("Executing GET"));
            // showMemory();
            getVar(EEPROM.read(++procesTable[i].programCounter), procesID);
            // showStack(procesID);
            break;

        case INCREMENT:
            // Serial.println(F("Executing INCREMENT"));
            popType = popByte(procesID); // get type;
            if (popType == 'I')
            {
                int resultInt = popInt(procesID);
                // Serial.println(resultInt);
                pushInt(resultInt + 1, procesID);
            }
            else if (popType == 'F')
            {
                float result = popFloat(procesID);
                pushFloat(result + 1.0, procesID);
            }
            else if (popType == 'C')
            {
                byte result = (byte)popChar(procesID);
                pushChar(result + 1, procesID);
            }
            // Serial.println(EEPROM.read(procesTable[i].programCounter));
            // Serial.println(EEPROM.read(procesTable[i].programCounter + 1));
            // Serial.println(EEPROM.read(procesTable[i].programCounter + 2));
            // Serial.println(EEPROM.read(procesTable[i].programCounter + 3));
            // Serial.println(EEPROM.read(procesTable[i].programCounter + 4));
            break;

        case DECREMENT:
            // Serial.println(F("Executing DECREMENT"));
            popType = popByte(procesID); // get type;

            if (popType == 'I')
            {
                int resultINT = popInt(procesID);
                pushInt(resultINT - 1, procesID);
            }
            else if (popType == 'F')
            {
                float result = popFloat(procesID);

                pushFloat(result - 1.00f, procesID);
            }
            else if (popType == 'C')
            {
                char result = (char)popChar(procesID);
                pushChar(result - 0x01, procesID);
            }
            break;

        case PLUS:
            Serial.println(F("Executing PLUS"));
            // showStack(procesID);
            popType = popByte(procesID); // get type;

            if (popType == 'I')
            {

                int x = popInt(procesID);
                popByte(procesID); // pop type for y
                int y = popInt(procesID);
                pushInt(x + y, procesID);
            }
            else if (popType == 'F')
            {
                float floatX = popFloat(procesID);
                popByte(procesID); // pop type for y
                float floatY = popFloat(procesID);

                pushFloat(floatX + floatY, procesID);
            }
            else if (popType == 'C')
            {
                char charX = (char)popChar(procesID);
                popByte(procesID); // pop type for y
                char charY = (char)popChar(procesID);
                pushChar(charX + charY, procesID);
            }
            // showStack(procesID);
            break;

        case MINUS:
            Serial.println(F("Executing MINUS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);

            pushInt(x - y, procesID);
            break;

        case TIMES:
            Serial.println(F("Executing TIMES"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);

            pushInt(x * y, procesID);
            break;

        case DIVIDEDBY:
            Serial.println(F("Executing DIVIDEDBY"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);

            pushInt(x / y, procesID);
            break;

        case MODULUS:
            Serial.println(F("Executing MODULUS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);

            pushInt(x % y, procesID);
            break;

        case UNARYMINUS:
            Serial.println(F("Executing UNARYMINUS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            pushInt(-x, procesID);
            break;

        case EQUALS:
            Serial.println(F("Executing EQUALS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x == y) ? '1' : '0', procesID);
            break;

        case NOTEQUALS:
            Serial.println(F("Executing NOTEQUALS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x != y) ? '1' : '0', procesID);

            break;

        case LESSTHAN:
            Serial.println(F("Executing LESSTHAN"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x < y) ? '1' : '0', procesID);
            break;

        case LESSTHANOREQUALS:
            Serial.println(F("Executing LESSTHANOREQUALS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x <= y) ? '1' : '0', procesID);
            break;

        case GREATERTHAN:
            Serial.println(F("Executing GREATERTHAN"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x > y) ? '1' : '0', procesID);
            break;

        case GREATERTHANOREQUALS:
            Serial.println(F("Executing GREATERTHANOREQUALS"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x >= y) ? '1' : '0', procesID);
            break;

        case LOGICALAND:
            Serial.println(F("Executing LOGICALAND"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x && y) ? '1' : '0', procesID);
            break;

        case LOGICALOR:
            Serial.println(F("Executing LOGICALOR"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar((x || y) ? '1' : '0', procesID);
            break;

        case LOGICALXOR:
            Serial.println(F("Executing LOGICALXOR"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            popByte(procesID); // get type;
            y = popInt(procesID);
            pushChar(((x && !y) || (!x && y)) ? '1' : '0', procesID);
            break;

        case LOGICALNOT:
            Serial.println(F("Executing LOGICALNOT"));
            popByte(procesID); // get type;
            x = popInt(procesID);

            pushChar((!x) ? '1' : '0', procesID);
            break;

        case BITWISEAND:
            Serial.println(F("Executing BITWISEAND"));
            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                x = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
            }

            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                y = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                y = popInt(procesID);
            }
            pushInt(x & y, procesID);
            break;

        case BITWISEOR:
            Serial.println(F("Executing BITWISEOR"));
            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                x = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
            }

            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                y = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                y = popInt(procesID);
            }

            pushInt(x | y, procesID);
            break;

        case BITWISEXOR:
            Serial.println(F("Executing BITWISEXOR"));
            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                x = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
            }

            // get type;
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                y = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                y = popInt(procesID);
            }

            pushInt(x ^ y, procesID);
            break;

        case BITWISENOT:
            Serial.println(F("Executing BITWISENOT"));
            popType = popByte(procesID);
            if (popType == 'C')
            {
                char charBuf[2];
                charBuf[0] = popChar(procesID);
                charBuf[1] = '\0';
                x = atoi(charBuf);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
            }

            pushInt(~x, procesID);
            break;

        case TOCHAR:
            Serial.println(F("Executing TOCHAR"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                charX = (char)(int)popFloat(procesID);
            }
            else if (popType == 'I')
            {
                charX = (char)popInt(procesID);
            }
            pushChar(charX, procesID);
            break;

        case TOINT:
            Serial.println(F("Executing TOINT"));
            popType = popByte(procesID);
            if (popType == 'C')
            {
                x = (int)popChar(procesID);
            }
            else if (popType == 'F')
            {
                x = (int)popFloat(procesID);
            }
            pushInt(x, procesID);
            break;

        case TOFLOAT:
            Serial.println(F("Executing TOFLOAT"));
            popType = popByte(procesID);
            if (popType == 'C')
            {
                floatX = (float)(byte)popChar(procesID);
            }
            else if (popType == 'I')
            {
                floatX = (float)popInt(procesID);
            }
            pushFloat(floatX, procesID);
            break;

        case ROUND:
            Serial.println(F("Executing ROUND"));
            x = popNumber(procesID);
            pushInt(x, procesID);
            break;

        case FLOOR:
            Serial.println(F("Executing FLOOR"));
            x = popNumber(procesID);
            pushInt(x, procesID);
            break;

        case CEIL:
            Serial.println(F("Executing CEIL"));
            popType = popByte(procesID);
            x = popNumber(procesID);

            pushInt(x, procesID);
            break;

        case MIN:
            Serial.println(F("Executing MIN"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);

                popByte(procesID); // get type;
                floatY = popFloat(procesID);

                pushFloat(min(floatX, floatY), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);

                popByte(procesID); // get type;
                y = popInt(procesID);
                pushInt(min(x, y), procesID);
            }

            break;

        case MAX:
            Serial.println(F("Executing MAX"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);

                popByte(procesID); // get type;
                floatY = popFloat(procesID);

                pushFloat(max(floatX, floatY), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);

                popByte(procesID); // get type;
                y = popInt(procesID);
                pushInt(max(x, y), procesID);
            }
            break;

        case ABS:
            Serial.println(F("Executing ABS"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);
                pushFloat(abs(floatX), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
                pushInt(abs(x), procesID);
            }

            break;

        case CONSTRAIN:
            Serial.println(F("Executing CONSTRAIN"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);

                popByte(procesID); // get type;
                floatY = popFloat(procesID);

                popByte(procesID); // get type;
                floatZ = popFloat(procesID);

                pushFloat(constrain(floatX, floatY, floatZ), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);

                popByte(procesID); // get type;
                y = popInt(procesID);

                popByte(procesID); // get type;
                z = popInt(procesID);

                pushInt(constrain(x, y, z), procesID);
            }

            break;

        case MAP:
            Serial.println(F("Executing MAP"));
            // get type;
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);

                popByte(procesID); // get type;
                floatY = popFloat(procesID);

                popByte(procesID); // get type;
                floatZ = popFloat(procesID);

                popByte(procesID); // get type;
                floatA = popFloat(procesID);

                popByte(procesID); // get type;
                floatB = popFloat(procesID);

                pushFloat((floatX - floatY) * (floatB - floatA) / (floatZ - floatY) + floatB, procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);

                popByte(procesID); // get type;
                y = popInt(procesID);

                popByte(procesID); // get type;
                z = popInt(procesID);

                popByte(procesID); // get type;
                a = popInt(procesID);

                popByte(procesID); // get type;
                b = popInt(procesID);

                pushInt((x - y) * (b - a) / (z - y) + b, procesID);
            }

            break;

        case POW:
            Serial.println(F("Executing POW"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);

                popByte(procesID); // get type;
                floatY = popFloat(procesID);

                pushFloat(pow(floatX, floatY), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);

                popByte(procesID); // get type;
                y = popInt(procesID);

                pushInt(pow(x, y), procesID);
            }
            break;

        case SQ:
            Serial.println(F("Executing SQ"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);
                pushFloat(pow(floatX, 2), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
                pushInt(pow(x, 2), procesID);
            }
            break;

        case SQRT:
            Serial.println(F("Executing SQRT"));
            popType = popByte(procesID);
            if (popType == 'F')
            {
                floatX = popFloat(procesID);
                pushFloat(sqrt(floatX), procesID);
            }
            else if (popType == 'I')
            {
                x = popInt(procesID);
                pushInt(sqrt(x), procesID);
            }
            break;

        case DELAY:
            floatX = popVal(procesID);
            delay(floatX);
            break;

        case MILLIS:
            // Serial.println(F("Executing MILLIS"));
            pushInt((int)millis(), procesID);
            break;

        case PINMODE:
            Serial.println(F("Executing PINMODE"));
            floatX = popVal(procesID);
            floatY = popVal(procesID);
            pinMode(floatX, floatY);
            break;

        case ANALOGREAD:
            Serial.println(F("Executing ANALOGREAD"));
            floatX = popVal(procesID);
            analogRead(floatX);
            break;

        case ANALOGWRITE:
            Serial.println(F("Executing ANALOGWRITE"));
            floatX = popVal(procesID);
            floatY = popVal(procesID);
            analogWrite(floatX, floatY);
            break;

        case DIGITALREAD:
            Serial.println(F("Executing DIGITALREAD"));
            floatX = popVal(procesID);
            digitalRead(floatX);
            break;

        case DIGITALWRITE:
            Serial.println(F("Executing DIGITALWRITE"));
            floatX = popVal(procesID);
            floatY = popVal(procesID);
            digitalWrite(floatX, floatY);
            break;

        case PRINT:
            // Serial.println(F("Executing PRINT"));
            popType = (char)popByte(procesID);
            if (popType == 'F')
            {
                Serial.print(popFloat(procesID));
            }
            else if (popType == 'I')
            {
                Serial.print((int)popInt(procesID));
            }
            else if (popType == 'C')
            {
                Serial.print((char)popChar(procesID));
            }
            else if (popType == 'S')
            {
                char *printString = popString(procesID);
                Serial.print(printString);
                free(printString);
            }
            break;

        case PRINTLN:
            // Serial.println(F("Executing PRINTLN"));
            // showStack(procesID);
            popType = (char)popByte(procesID);
            if (popType == 'F')
            {
                Serial.print(F("Float: "));
                Serial.println(popFloat(procesID));
            }
            else if (popType == 'I')
            {
                int printInt = popInt(procesID);
                Serial.print(F("Integer: "));
                Serial.println(printInt);
            }
            else if (popType == 'C')
            {
                Serial.print(F("Char: "));
                Serial.println((char)popChar(procesID));
            }
            else if (popType == 'S')
            {
                char *stringresult = popString(procesID);
                Serial.print(F("String: "));
                Serial.println(stringresult);
                free(stringresult);
            }
            break;

        case OPEN:
        {
            // Serial.println(F("Executing OPEN"));
            popByte(procesID); // pop type
            int size = popInt(procesID);
            // showStack(procesID);
            popByte(procesID); // pop type
            char *name = popString(procesID);

            int fileIndex = findName(name);
            if (fileIndex == -1)
            {
                int availableSpaceIndex = findAvailableSpaceFAT(size);
                if (availableSpaceIndex == -1)
                {
                    Serial.print("ERROR: file not found and no space left for a file sized: ");
                    Serial.println(size);
                    free(name);
                    break;
                }
                // Prepare file info
                fileInfo file = {0};
                strncpy(file.name, name, sizeof(file.name) - 1);
                file.name[sizeof(file.name) - 1] = '\0';
                file.position = availableSpaceIndex;
                file.length = size;

                writeFATEntry(noOfFiles * 16 + 1, file);
                procesTable[i].filePointer = file.position;
                free(name);
                break;
            }
            fileInfo file = readFATEntry(fileIndex);
            procesTable[i].filePointer = file.position;
            free(name);
            break;
        }

        case CLOSE:
            Serial.println(F("Executing CLOSE"));
            break;

        case WRITE:
            // Serial.println(12);
            Serial.println(F("Executing WRITE IN SWITCH"));

            break;

        case IF:
            Serial.println(F("Executing IF"));
            break;

        case ELSE:
            Serial.println(F("Executing ELSE"));
            break;

        case ENDIF:
            Serial.println(F("Executing ENDIF"));
            break;

        case WHILE:
            Serial.println(F("Executing WHILE"));
            break;

        case ENDWHILE:
            Serial.println(F("Executing ENDWHILE"));
            break;

        case LOOP:
            // Handled below for PC/loop register behavior.
            break;

        case ENDLOOP:
            // Handled below for PC/loop register behavior.
            break;

        case FORK:
            // Handled below to keep consistent stack/file side effects.
            break;

        case WAITUNTILDONE:
            // Handled below to keep PC behavior consistent with DELAYUNTIL.
            break;

        case STOP:
            // Handled below to terminate and clean process state.
            break;

        default:
            Serial.print(F("Unknown opcode: "));
            Serial.println(instruction);
            break;
        }
        // kill process when at the end of its life :(
        // Serial.println(name);
        // Serial.println(length);
        // Serial.println(procesCodaer);

        // write is in an if case because for some reason it doesnt execute in the  switch case
        if (instruction == WRITE)
        {
            Serial.println(F("Executing WRITE"));
            popType = popByte(procesID);
            int addr = procesTable[i].filePointer;
            if (popType == 'F')
            {
                float val = popFloat(procesID);
                byte *b = (byte *)&val;

                // value
                EEPROM.write(addr, b[3]);
                EEPROM.write(addr + 1, b[2]);
                EEPROM.write(addr + 2, b[1]);
                EEPROM.write(addr + 3, b[0]);

                procesTable[i].filePointer += 4;
            }
            else if (popType == 'I')
            {
                int val = popInt(procesID);
                byte MSB = highByte(val);
                byte LSB = lowByte(val);
                EEPROM.write(addr, MSB);
                EEPROM.write(addr + 1, LSB);

                procesTable[i].filePointer += 2;
            }
            else if (popType == 'C')
            {
                char val = popChar(procesID);
                EEPROM.write(addr, val);
                procesTable[i].filePointer += 1;
            }
            else if (popType == 'S')
            {
                showStack(procesID);
                char *str = popString(procesID);
                int len = strlen(str);
                Serial.println(str);
                for (int j = 0; j < len; j++)
                {
                    EEPROM.write(addr + j, str[j]);
                    Serial.println(str[j]);
                }
                EEPROM.write(addr + len, '\0');
                procesTable[i].filePointer += len + 1;
            }
        }
        else if (instruction == READINT)
        {
            // Serial.println(F("Executing READINT"));
            byte MSB = EEPROM.read(procesTable[i].filePointer);
            byte LSB = EEPROM.read(procesTable[i].filePointer + 1);
            // EEPROM.get(procesTable[i].filePointer, resultInt);
            int resultInt = word(MSB, LSB);
            procesTable[i].filePointer += sizeof(int);
            pushInt(resultInt, procesID);
        }
        else if (instruction == READCHAR)
        {
            // Serial.println(F("Executing READCHAR"));
            char resultChar;
            EEPROM.get(procesTable[i].filePointer, resultChar);
            procesTable[i].filePointer += sizeof(char);
            pushChar(resultChar, procesID);
        }
        else if (instruction == READFLOAT)
        {
            // Serial.println(F("Executing READFLOAT"));

            float resultFloat;
            ((byte *)&resultFloat)[3] = EEPROM.read(procesTable[i].filePointer);
            ((byte *)&resultFloat)[2] = EEPROM.read(procesTable[i].filePointer + 1);
            ((byte *)&resultFloat)[1] = EEPROM.read(procesTable[i].filePointer + 2);
            ((byte *)&resultFloat)[0] = EEPROM.read(procesTable[i].filePointer + 3);
            // EEPROM.get(procesTable[i].filePointer, resultInt);
            procesTable[i].filePointer += sizeof(float);
            pushFloat(resultFloat, procesID);
        }
        else if (instruction == READSTRING)
        {
            // Serial.println(F("Executing READSTRING"));
            int addrString = procesTable[i].filePointer;
            int j = 0;
            char singleChar;

            // First, find the length (including null terminator)
            do
            {
                singleChar = EEPROM.read(addrString + j);
                j++;
            } while (singleChar != '\0');

            // Allocate memory for the string
            char *resultString = (char *)malloc(j);
            if (!resultString)
            {
                Serial.println(F("Memory allocation failed!"));
                return;
            }

            // Copy the string from EEPROM
            for (int k = 0; k < j; k++)
            {
                resultString[k] = EEPROM.read(addrString + k);
            }

            resultString[j - 1] = '\0'; // Ensure null-termination
            // Serial.println(resultString);
            procesTable[i].filePointer += j;
            pushString(resultString, procesID);
            free(resultString);
        }
        else if (instruction == MILLIS)
        {
            // Serial.println(F("Executing MILLIS"));
            // Serial.println(millis());
            pushInt((int)millis(), procesID);
        }
        else if (instruction == DELAYUNTIL)
        {
            // Serial.println(F("Executing DELAYUNTIL"));
            popByte(procesID); // get data type;
            x = popInt(procesID);
            // showStack(procesID);
            if ((unsigned long)x > millis())
            {

                pushInt(x, procesID);
                // procesTable[i].programCounter--;
                return;
            }

            // popByte(procesID); // get data type;
            // x = popInt(procesID);
            // showStack(procesID);
            // floatX = popVal(procesID);
            // if ((int)floatX > (int)millis())
            // {
            //     Serial.println(F("bigger then MILLIS"));
            //     procesTable[i].programCounter--;
            //     pushFloat(floatX, procesID);
            // }
        }
        else if (instruction == LOOP)
        {
            // Serial.println(F("Executing LOOP"));
            procesTable[i].loopAdres = procesCounter;
        }
        else if (instruction == ENDLOOP)
        {
            // Serial.println(F("Executing ENDLOOP"                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          ));
            procesTable[i].programCounter = procesTable[i].loopAdres;
        }
        else if (instruction == FORK)
        {
            popByte(procesID); // pop type
            char *forkName = popString(procesID);

            int processCountBefore = noOfProces;
            int newProcessId = ::procesID;
            run(forkName);

            if (noOfProces > processCountBefore)
            {
                pushInt(newProcessId, procesID);
            }

            free(forkName);
        }
        else if (instruction == WAITUNTILDONE)
        {
            char waitType = popByte(procesID);
            int waitProcessId = 0;

            if (waitType == 'I')
            {
                waitProcessId = popInt(procesID);
            }
            else if (waitType == 'F')
            {
                waitProcessId = (int)popFloat(procesID);
            }
            else if (waitType == 'C')
            {
                waitProcessId = (int)(byte)popChar(procesID);
            }
            else
            {
                Serial.println(F("Invalid type on stack for WAITUNTILDONE"));
                return;
            }

            int waitProcessIndex = findID(waitProcessId);
            if (waitProcessIndex != -1 && procesTable[waitProcessIndex].state != '0')
            {
                pushInt(waitProcessId, procesID);
                return;
            }
        }

        else if (instruction == STOP)
        {
            deleteProcesVars(procesID);
            removeProcesFromList(i);
            Serial.println(F("Process terminated"));
            return;
        }
        procesTable[i].programCounter++;
    }
}