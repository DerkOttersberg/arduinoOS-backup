Mondelinge Toets ArduinOS — Minimale 6 (items 1,2,4,5,6,7)
=========================================================

Demo-script (exacte commando's)
-------------------------------

> store demo 5 hello
> files
> retrieve demo
> freespace
> erase demo

> run loopx
> list

> suspend 0
> list
> resume 0

> run loopy
> kill 0
> kill 1


Code per item (aan te wijzen in IDE)
-------------------------------------

Item 1 - CLI leest opdrachten
  readToken()          CLIHandeling.cpp:7
  commandHandler()     commands.cpp:22

Item 2 - Bestandsoperaties
  fileInfo struct      EEPROMHandeling.h:9
  store()              commands.cpp:47
  freespace()          commands.cpp:325

Item 4 - Proces starten/pauzeren/stoppen
  procesEntry struct   procesHandeling.h:7
  run()                procesHandeling.cpp:23
  suspend()            procesHandeling.cpp:67

Item 5 - Processtatus, PC, variabelen bijhouden
  procesEntry struct   procesHandeling.h:7    (state=byte, programCounter, eigen stack)
  list()               procesHandeling.cpp:152

Item 6 - Bytecode-instructies uitvoeren
  execute()            procesHandeling.cpp:283   (switch-case, 50+ opcodes)
  runProcesses()       procesHandeling.cpp:273

Item 7 - Meerdere processen tegelijk (round-robin)
  loop()               main.cpp:57               (elke iteratie runProcesses())
  runProcesses()       procesHandeling.cpp:273   (1 instructie per proces)


Meer punten (items 3, 8, 9)
----------------------------

Item 3 - Variabelen:  run test_vars
  setVar()             memoryHandeling.cpp:8
  getVar()             memoryHandeling.cpp:87

Item 8 - File I/O:  run write_file (eerst), daarna files + retrieve om data te checken
  OPEN/WRITE/READ*     procesHandeling.cpp:1042-1253

Item 9 - Fork:  run testfork
  FORK                 procesHandeling.cpp:1296
  WAITUNTILDONE        procesHandeling.cpp:1312


Valkuilen
---------
- IF/ELSE/ENDIF, WHILE/ENDWHILE zijn stubs (alleen debug print)
- MILLIS heeft een double-push bug (procesHandeling.cpp:953 + 1255)
- Gebruik clearEEPROM NOOIT tenzij FAT corrupt is (het wist alles)
- Vraagt docent naar IF/WHILE: "alleen LOOP/ENDLOOP flow control werkt"


Bytecode her-uploaden
---------------------
Zie converter/upload2.py + converter/README.txt
