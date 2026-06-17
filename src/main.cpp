#include <Arduino.h>
#include <commands.h>
#include <CLIHandeling.h>
#include <EEPROMHandeling.h>
#include <memoryHandeling.h>
#include <procesHandeling.h>

static char token[BUFSIZE];
void setup()
{
  Serial.begin(9600);
  Serial.println("ArduinOS 1.0 ready.");
  Serial.print("> ");
}
int argCount = 0;

void loop()
{

  if (readToken(token))
  {
    const char **userInput = seperateArgCommand(token);
    commandHandler(userInput);
    Serial.print("> ");
  }
  runProcesses(); // wordt elke iteratie aangeroepen zodat het programma blijft draaien terwijl je ook kan typen
}
