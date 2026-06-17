#include "stack.h"
#include "procesHandeling.h"

void pushByte(byte b, int procesID)
{
    // Serial.println(b);
    int procesIndex = findID(procesID);
    procesEntry &proces = procesTable[procesIndex];

    if (proces.sp >= STACKSIZE)
    {
        Serial.println("Stack overflow!");
        return;
    }

    proces.stack[proces.sp++] = b;
}

byte popByte(int procesID)
{
    int procesIndex = findID(procesID);
    procesEntry &proces = procesTable[procesIndex];
    // Serial.println(proces.stack[proces.sp]);
    if (proces.sp == 0)
    {
        Serial.println("Stack underflow!");
        return 0;
    }
    return proces.stack[--proces.sp];
}

// push values to stack
void pushInt(int data, int procesID)
{
    // Serial.println(d/ata);
    byte MSB = highByte(data);
    byte LSB = lowByte(data);
    // value
    pushByte(MSB, procesID);
    pushByte(LSB, procesID);
    pushByte('I', procesID);
}

void pushFloat(float data, int procesID)
{
    byte *b = (byte *)&data;

    // value
    pushByte(b[3], procesID);
    pushByte(b[2], procesID);
    pushByte(b[1], procesID);
    pushByte(b[0], procesID);

    //  meta data
    // pushByte(4);
    pushByte('F', procesID);
}

void pushChar(char data, int procesID)
{
    pushByte(data, procesID);
    pushByte('C', procesID);
}

void pushString(char *data, int procesID)
{
    // value
    // Serial.println("pushstring");
    // Serial.println(data);
    size_t length = strlen(data);
    for (size_t i = 0; i <= length; i++)
    {
        pushByte(data[i], procesID);
        // Serial.println(data[i]);
    }

    //  meta data
    pushByte(strlen(data) + 1, procesID);
    pushByte('S', procesID);
}



int popInt(int procesID)
{
    // value
    byte LSB = popByte(procesID);
    byte MSB = popByte(procesID);

    return word(MSB, LSB);
}

float popFloat(int procesID)
{
    float value;
    ((byte *)&value)[3] = popByte(procesID);
    ((byte *)&value)[2] = popByte(procesID);
    ((byte *)&value)[1] = popByte(procesID);
    ((byte *)&value)[0] = popByte(procesID);
    // float value;
    return value;
}

char popChar(int procesID)
{
    return popByte(procesID);
}

char *popString(int procesID)
{
    int length = (int)popByte(procesID); // includes terminating zero

    if (length <= 0 || length > STACKSIZE)
    {
        char *empty = (char *)malloc(1);
        if (empty != NULL)
        {
            empty[0] = '\0';
        }
        return empty;
    }

    char *result = (char *)malloc(length + 1);
    if (result == NULL)
    {
        return NULL;
    }

    for (int i = length - 1; i >= 0; i--)
    {
        result[i] = (char)popByte(procesID);
    }

    result[length] = '\0';
    result[length - 1] = '\0';

    return result;
}
int popNumber(int procesID)
{
    char popType = popByte(procesID);
    if (popType == 'F')
    {
        return (int)popFloat(procesID);
    }
    else if (popType == 'I')
    {
        return popInt(procesID);
    }
    else if (popType == 'C')
    {
        return (int)(byte)popChar(procesID);
    }

    return 0;
}
float popVal(int procesID)
{
    char popType = popByte(procesID);
    if (popType == 'F')
    {
        return popFloat(procesID);
    }
    else if (popType == 'I')
    {
        return (float)popInt(procesID);
    }
    else if (popType == 'C')
    {
        return (float)(byte)popChar(procesID);
    }

    return 0.0f;
}

void showStack(int procesID)
{
    int procesIndex = findID(procesID);
    procesEntry &proces = procesTable[procesIndex];
    Serial.print(F("Stack contents for process "));
    Serial.println(procesID);
    Serial.println(F("Index |  Hex  |  Dec  | ASCII"));
    //  show everything that is in it.
    for (int i = 0; i < proces.sp; i++)
    {
        byte value = proces.stack[i];
        Serial.print(i < 10 ? "  " : " "); // Pad index
        Serial.print(i);
        Serial.print(F("   |  0x"));
        if (value < 0x10)
            Serial.print('0'); // Pad hex
        Serial.print(value, HEX);
        Serial.print(F("  |  "));
        if (value < 10)
            Serial.print("  "); // Pad decimal
        else if (value < 100)
            Serial.print(" ");
        Serial.print(value, DEC);
        Serial.print(F("  |  "));

        // ASCII printable check
        if (value >= 32 && value <= 126)
            Serial.write(value);
        else
            Serial.print(".");

        Serial.println();
    }
}