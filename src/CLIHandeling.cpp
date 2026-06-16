#include <Arduino.h>
#include <CLIHandeling.h>
#include <string.h>
static byte inputIndex = 0;

// Functie om een token (woord) in te lezen
bool readToken(char *buffer)
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == ' ')
        {
            if (inputIndex == 0)
                continue;
            Serial.write(c);
            buffer[inputIndex++] = ' ';
        }
        else if (c == '\n' || c == '\r')
        {
            if (inputIndex == 0)
                continue; // negeer dubbele spaties
            Serial.println();
            buffer[inputIndex] = '\0';
            inputIndex = 0;
            return true;
        }
        else if (c == '\b' || c == 127)
        {
            if (inputIndex > 0)
            {
                inputIndex--;
                Serial.print("\b \b");
            }
        }
        else
        {
            if (inputIndex < BUFSIZE - 1)
            {
                Serial.write(c);
                buffer[inputIndex++] = c;
            }
        }
    }
    return false;
}

const char *parts[2];
const char **seperateArgCommand(char *input)
{
    parts[0] = NULL;
    parts[1] = NULL;

    if (input == NULL)
    {
        return parts;
    }

    while (*input == ' ')
    {
        input++;
    }

    if (*input == '\0')
    {
        return parts;
    }

    parts[0] = input;

    char *space = strchr(input, ' ');
    if (space != NULL)
    {
        *space = '\0';
        char *args = space + 1;
        while (*args == ' ')
        {
            args++;
        }
        if (*args != '\0')
        {
            parts[1] = args;
        }
    }

    return parts;
}