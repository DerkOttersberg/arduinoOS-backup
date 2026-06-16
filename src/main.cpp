#include <Arduino.h>
#include <commands.h>
#include <CLIHandeling.h>
#include <EEPROMHandeling.h>
#include <memoryHandeling.h>
#include <procesHandeling.h>

static char token[BUFSIZE];

// fileInfo file1 = {"name1", 161, "123123123"};
// fileInfo file2 = {"name2", 12, "asdasdas"};
// fileInfo file3 = {"name3", 12, 5};
// const int nCommands = sizeof(commands) / sizeof(commandType);
void setup()
{
  Serial.begin(9600);
  Serial.println("ArduinOS 1.0 ready.");
  Serial.print("> ");

  // store("file1 12 bbbbbbb");
  // store("file2 12 aaaaaaaa");
  // store("file3 12 ccccccccc");

  // pushChar('a');
  // setVar('x', 0);
  // getVar('x', 0);
  // popByte(); // gets type
  // Serial.println(popChar());

  // pushInt(12);
  // setVar('y', 1);

  // getVar('y', 1);
  // popByte(); // gets type
  // Serial.println(popInt());

  // pushFloat(1.234);
  // setVar('z', 1);
  // getVar('z', 1);
  // popByte(); // gets type
  // Serial.println(popFloat());

  // setVar('s', 2);

  // pushInt(12);
  // setVar('s', 2);
  // getVar('s', 2);
  // popByte(); // gets type
  // Serial.println(popString());
  // Serial.println(F("before clearing"));
  // showMemory();
  // deleteProcesVars(1);
  // Serial.println(F("after clearing"));
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
  runProcesses();
}
