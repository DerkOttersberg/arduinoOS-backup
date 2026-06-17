#include <Arduino.h>
#include "EEPROM.h"
#include "commands.h"
#include "EEPROMHandeling.h"
#include "procesHandeling.h"
commandType commands[] = {
    {"store", store},
    {"erase", erase},
    {"retrieve", retrieve},
    {"files", files},
    {"freespace", freespace},
    {"showEEPROM", showEEPROM},
    {"run", run},
    {"suspend", suspend},
    {"resume", resume},
    {"kill", kill},
    {"list", list},
    {"clearEEPROM", clearEEPROM},
    {"showByte", showProcesByteCode},
};

// Loopt door de array van alle 13 commands, en wanneer het overeenkomt roept hij de bijbehorende functie aan
const int nCommands = sizeof(commands) / sizeof(commandType);
void commandHandler(const char **userInput)
{
    if (userInput == NULL || userInput[0] == NULL)
    {
        unknownCommand(NULL);
        return;
    }

    const char *command = userInput[0];
    const char *args = userInput[1];
    for (int i = 0; i < nCommands; i++)
    {

        if (strcmp(command, commands[i].name) == 0)
        {
            commands[i].func(args);
            return;
        }
    }
    unknownCommand(args);
}

//
// @param
// char[12} name, byte size, data

//schrijft de files naar eeprom maar checkt eerst op groote, limiet van files
void store(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments for store."));
        return;
    }

    // Parse: <name> <size> [data...]
    const char *cursor = arg;
    while (*cursor == ' ')
    {
        cursor++;
    }

    const char *nameStart = cursor;
    while (*cursor != '\0' && *cursor != ' ')
    {
        cursor++;
    }

    int nameLen = (int)(cursor - nameStart);
    if (nameLen <= 0 || nameLen >= 12)
    {
        Serial.println(F("Invalid file name."));
        return;
    }

    char nameToken[12] = {0};
    memcpy(nameToken, nameStart, nameLen);
    nameToken[nameLen] = '\0';

    while (*cursor == ' ')
    {
        cursor++;
    }

    if (*cursor == '\0')
    {
        Serial.println(F("Not enough arguments for store."));
        return;
    }

    char *sizeEnd = NULL;
    long parsedSize = strtol(cursor, &sizeEnd, 10);
    if (sizeEnd == cursor)
    {
        Serial.println(F("Invalid file size."));
        return;
    }

    int size = (int)parsedSize;
    if (size <= 0)
    {
        Serial.println(F("Invalid file size."));
        return;
    }

    cursor = sizeEnd;
    while (*cursor == ' ')
    {
        cursor++;
    }

    const char *data = (*cursor == '\0') ? NULL : cursor;

    if (noOfFiles == 10)
    {
        Serial.println(F("No more then 10 files can be saved at a time."));
        return;
    }
    if (findName(nameToken) != -1)
    {
        Serial.println(F("File name already exists. Please enter a different file name."));
        return;
    }
    int availableSpaceIndex = findAvailableSpaceFAT(size);
    if (availableSpaceIndex == -1)
    {
        Serial.println(F("There is no available space left for this file size."));
        return;
    }

    fileInfo file = {0};
    strncpy(file.name, nameToken, sizeof(file.name));
    file.name[sizeof(file.name) - 1] = '\0';
    file.position = availableSpaceIndex;
    file.length = size;

    int bytesWritten = 0;
    if (data != NULL)
    {
        while (bytesWritten < size && data[bytesWritten] != '\0')
        {
            EEPROM.write(availableSpaceIndex + bytesWritten, (byte)data[bytesWritten]);
            bytesWritten++;
        }
        while (bytesWritten < size)
        {
            EEPROM.write(availableSpaceIndex + bytesWritten, 0);
            bytesWritten++;
        }
    }
    else
    {
        unsigned long start = millis();
        while (bytesWritten < size)
        {
            if (Serial.available())
            {
                EEPROM.write(availableSpaceIndex + bytesWritten, (byte)Serial.read());
                bytesWritten++;
                start = millis();
            }
            else if ((millis() - start) > 4000)
            {
                Serial.println(F("Did not receive all data bytes."));
                for (int i = 0; i < bytesWritten; i++)
                {
                    EEPROM.write(availableSpaceIndex + i, 0);
                }
                return;
            }
        }
    }

    writeFATEntry(noOfFiles * 16 + 1, file);
    Serial.println(F("Succesfully inserted file into FAT"));
}
struct TempFile
{
    fileInfo info;
    byte *data;
};

void erase(const char *arg)
{

    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    int nameIndex = findName(arg);
    if (nameIndex == -1)
    {
        Serial.println(F("No file exists with that name"));
        return;
    }
    fileInfo deletedFile = readFATEntry(nameIndex);

    // Gather all following files' data BEFORE erasing anything else
    const int lastEntryPosition = noOfFiles * 16 + 1;
    int shiftStart = nameIndex + 16;
    int shiftCount = (lastEntryPosition - shiftStart) / 16;

    TempFile *shiftedFiles = new TempFile[shiftCount];
    int fileIdx = 0;

    for (int i = shiftStart; i < lastEntryPosition; i += 16)
    {
        fileInfo file = readFATEntry(i);
        if (file.length == 0)
            continue;

        byte *fileData = retrieveFromEEPROM(file);
        shiftedFiles[fileIdx++] = {file, fileData};
    }

    // erase the FAT and EEPROM content of the deleted file
    eraseFATEntry(nameIndex);
    eraseFromEEPROM(deletedFile.position, deletedFile.length);
    int totalFilesBeforeErasing = noOfFiles;
    // Now re-insert the files at their new positions
    for (int j = 0; j < fileIdx; j++)
    {
        // erase old fileData and insert it into its new position
        eraseFromEEPROM(shiftedFiles[j].info.position, shiftedFiles[j].info.length);
        shiftedFiles[j].info.position -= deletedFile.length;
        putIntoEEPROM(shiftedFiles[j].data, shiftedFiles[j].info.length, shiftedFiles[j].info.position);

        // shift the fat up
        shiftedFiles[j].info.position = deletedFile.position + j * shiftedFiles[j].info.length;
        writeFATEntry(nameIndex + (j * 16), shiftedFiles[j].info);
        eraseFATEntry(nameIndex + 16 + (j * 16)); // clean up old FAT
        delete[] shiftedFiles[j].data;
    }

    delete[] shiftedFiles;

    noOfFiles = totalFilesBeforeErasing - 1;
    Serial.println(F("File erased and remaining files moved."));
}

void retrieve(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    int nameIndex = findName(arg);
    if (nameIndex == -1)
    {
        Serial.println(F("No file exists with that name"));
        return;
    }

    fileInfo file = readFATEntry(nameIndex);
    if (file.length <= 0 || file.position < 161)
    {
        Serial.println(F("Corrupted file entry."));
        return;
    }

    int eepromLength = EEPROM.length();
    if ((file.position + file.length) > eepromLength)
    {
        Serial.println(F("Corrupted file entry."));
        return;
    }

    if (file.length > 128)
    {
        Serial.println(F("File too large for console retrieve."));
        return;
    }

    for (int i = 0; i < file.length; i++)
    {
        byte b = EEPROM.read(file.position + i);
        if (b >= 32 && b <= 126)
        {
            Serial.write(b);
        }
        else
        {
            Serial.print('.');
        }
    }
    Serial.println();
}
void files(const char *arg)
{
    int validFileCount = 0;
    int eepromLength = EEPROM.length();
    for (int i = 0; i < noOfFiles; i++)
    {
        int positionIndex = i * 16 + 1;
        fileInfo file = readFATEntry(positionIndex);
        if (file.length > 0 && file.position >= 161 && file.name[0] != '\0' && (file.position + file.length) <= eepromLength)
        {
            validFileCount++;
        }
    }

    Serial.println("Number of files: ");
    Serial.println(validFileCount);
    for (int i = 0; i < noOfFiles; i++)
    {
        //  index + fileInfosize + translation for numberOfFiles variable
        int positionIndex = i * 16 + 1;
        fileInfo file = readFATEntry(positionIndex);
        if (file.length <= 0 || file.position < 161 || file.name[0] == '\0' || (file.position + file.length) > eepromLength)
        {
            continue;
        }
        Serial.print("Name: ");
        Serial.println(file.name);
        Serial.print("size: ");
        Serial.println(file.length);
        Serial.print("Position: ");
        Serial.println(file.position);
        Serial.println(" ");
    }
}

//berekent eeprom lengtes - laatste iend positie van die file
void freespace(const char *arg)
{
    int maxEnd = 161;

    for (int i = 0; i < noOfFiles; i++)
    {
        int positionIndex = i * 16 + 1;
        fileInfo file = readFATEntry(positionIndex);
        if (file.length <= 0 || file.position < 161 || file.name[0] == '\0')
        {
            continue;
        }

        int endPos = file.position + file.length;
        if (endPos > maxEnd)
        {
            maxEnd = endPos;
        }
    }

    int freeBytes = EEPROM.length() - maxEnd;
    if (freeBytes < 0)
    {
        freeBytes = 0;
    }

    if (maxEnd == 161)
    {
        Serial.print("Amount of space left is: ");
        Serial.println(EEPROM.length() - 161);
        return;
    }

    Serial.print("Amount of space left is: ");
    Serial.println(freeBytes);
}
void showCharacterEEPROM(const char *arg)
{
    int eepromLength = EEPROM.length();
    for (int i = 161; i < eepromLength; i++)
    {
        byte b = EEPROM.read(i);
        if (b >= 32 && b <= 126)
        {
            Serial.print((char)b); // printable ASCII
        }
        else
        {
            Serial.print(".");
        }
    }
    Serial.println();
}
void showEEPROM(const char *arg)
{
    int eepromLength = EEPROM.length();
    for (int i = 0; i < eepromLength; i++)
    {
        byte b = EEPROM.read(i);
        if (b >= 32 && b <= 126)
        {
            Serial.print((char)b); // printable ASCII
        }
        else
        {
            Serial.print(b);
        }
    }
    Serial.println();
}
void unknownCommand(const char *arg)
{
    Serial.println("Onbekend commando. Beschikbare commando's:");
    for (int i = 0; i < nCommands; i++)
    {
        Serial.println(commands[i].name);
    }
}

void clearEEPROM(const char *arg)
{
    resetProcesses();

    int eepromLength = EEPROM.length();
    for (int i = 0; i < eepromLength; i++)
    {
        EEPROM.write(i, 0);
    }
    noOfFiles = 0;

    while (Serial.available())
    {
        Serial.read();
    }

    Serial.println("EEPROM cleared");
}
void showProcesByteCode(const char *arg)
{
    if (arg == NULL)
    {
        Serial.println(F("Not enough arguments has been given. Please try again."));
        return;
    }

    int nameIndex = findName(arg);
    if (nameIndex == -1)
    {
        Serial.println(F("No instruction file exists with that name"));
        return;
    }

    fileInfo file = readFATEntry(nameIndex);
    if (file.length <= 0 || file.position < 161 || (file.position + file.length) > EEPROM.length())
    {
        Serial.println(F("Corrupted file entry."));
        return;
    }

    for (int i = 0; i < file.length; i++)
    {
        Serial.println(EEPROM.read(file.position + i));
    }
}
