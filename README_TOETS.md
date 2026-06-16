# Mondelinge Toets ArduinOS

## Minimale 6 (items 1, 2, 4, 5, 6, 7)

---

## Demo-script (exacte commando's die je typt)

```
> store demo 5 hello              ← item 2: bestand aanmaken
> files                           ← item 2: lijst tonen
> retrieve demo                   ← item 2: inhoud tonen
> freespace                       ← item 2: vrije ruimte
> erase demo                      ← item 2: bestand wissen

> run loopx                       ← item 4+6: proces starten, bytecode voert uit
> list                            ← item 5: processen met state (r/p/0)

> suspend 0                       ← item 4: pauzeren
> list                            ← item 5: state = p
> resume 0                        ← item 4: hervatten

> run loopy                       ← item 7: tweede proces starten
                                  ← beide draaien nu tegelijk (afwisselend x en y)
> kill 0                          ← item 4: stoppen
> kill 1                          ← opruimen
```

---

## Code die je moet aanwijzen per item

| Item | Functie | Bestand:Regel | Wat je zegt |
|------|---------|---------------|-------------|
| 1 | `readToken()` | `CLIHandeling.cpp:7` | Leest karakter voor karakter van Serial, backspace werkt, newline = klaar. Non-blocking zodat processen blijven draaien. |
| 1 | `commandHandler()` | `commands.cpp:22` | Loopt door een array van 13 commands, `strcmp` op naam, roept bijbehorende functie aan. |
| 2 | `fileInfo` struct | `EEPROMHandeling.h:9` | Elke FAT-entry is 16 bytes: naam, positie, lengte. FAT start op byte 1, data op byte 161, max 10 bestanden. |
| 2 | `store()` | `commands.cpp:47` | Parseert naam+grootte, checkt duplicaten en limiet (10), schrijft data naar EEPROM. |
| 2 | `freespace()` | `commands.cpp:325` | Berekent `EEPROM.length() - laatsteEindPositie`. |
| 4 | `procesEntry` struct | `procesHandeling.h:7` | State ('r'/'p'/0), programCounter (positie in EEPROM), eigen stack[64], stack pointer. |
| 4 | `run()` | `procesHandeling.cpp:23` | Zoekt bestand in FAT, vult procesEntry in, state='r', PC=bestandspositie. |
| 4 | `suspend()` | `procesHandeling.cpp:67` | `changeState(index, 'p')` — proces slaat volgende instructie over. |
| 5 | `list()` | `procesHandeling.cpp:152` | Toont ID, naam, state van elk proces. |
| 5 | `procesEntry` struct | `procesHandeling.h:7` | Program counter wordt hier opgeslagen en per instructie opgehoogd. Variabelen via `memoryEntry.procesID`. |
| 6 | `execute()` | `procesHandeling.cpp:283` | Grote switch-case, 50+ opcodes. Leest 1 instructie uit EEPROM, voert uit, verhoogt PC. |
| 6 | `runProcesses()` | `procesHandeling.cpp:273` | Wordt elke `loop()` aangeroepen, itereert over alle processen. |
| 7 | `loop()` | `main.cpp:57` | `runProcesses()` wordt elke iteratie aangeroepen — round-robin scheduling. |
| 7 | `runProcesses()` | `procesHandeling.cpp:273` | `for(i=0;i<noOfProces;i++) execute(i)` — elk proces precies 1 instructie per cyclus. |

---

## Structuur van je presentatie (15-20 min)

| Tijd | Actie |
|------|-------|
| **0-2m** | Open Serial Monitor, je ziet `ArduinOS 1.0 ready` + `>` prompt |
| **2-4m** | **Item 1+2**: Type `store demo 5 hello` → `files` → `retrieve demo` → `freespace` → `erase demo`. Wijs `store()` en `fileInfo` struct aan. |
| **4-7m** | **Item 4+5**: `run loopx` → `list` (state='r') → `suspend 0` → `list` (state='p') → `resume 0` → `kill 0`. Wijs `procesEntry` struct aan, leg state/PC uit. |
| **7-10m** | **Item 6+7**: `run loopx` → `run loopy` → beide draaien tegelijk, x en y alternerend. Wijs `execute()` switch-case en `runProcesses()` round-robin aan. |
| **10-12m** | Kill beide processen. Klaar = **minimaal 6 punten**. |

---

## Wil je meer punten?

Voeg deze items toe:

| Item | Commando | Tijd |
|------|----------|------|
| **3** (variabelen) | `run test_vars` — toont SET/GET van alle 4 types, PRINTLN van CHAR/INT/FLOAT/STRING | +2m |
| **8** (file I/O) | `run write_file` — creeert bestand + schrijft → `files` toont "data" → `run test_vars` toont de geschreven data | +3m |
| **9** (fork) | `run testfork` — print "Forking...", forkt child, wacht, print "bye!" | +2m |

### Code voor extra items

| Item | Functie | Bestand:Regel |
|------|---------|---------------|
| 3 | `setVar()` | `memoryHandeling.cpp:8` |
| 3 | `getVar()` | `memoryHandeling.cpp:87` |
| 8 | `OPEN`/`WRITE`/`READ*` | `procesHandeling.cpp:1042-1253` |
| 9 | `FORK` | `procesHandeling.cpp:1296` |
| 9 | `WAITUNTILDONE` | `procesHandeling.cpp:1312` |

---

## Valkuilen om te kennen

- `test_loop`/`test_delay` gebruiken **MILLIS** — die heeft een bekende double-push bug in de code (`procesHandeling.cpp:953` + `1255`) maar werkt voor de demo
- `write_file` creeert bestand "data" — voor item 8 moet je `run write_file` **eerst** draaien, dan kun je met `files` het bestand zien en met `run test_vars` de inhoud lezen
- Vraagt de docent naar **IF/ELSE/WHILE**: zeg eerlijk "die opcodes zijn nog stubs, alleen LOOP/ENDLOOP flow control werkt"
- **`clearEEPROM` NOOIT gebruiken** tijdens de demo tenzij FAT echt corrupt is — het wist alles
- De **`erase` functie heeft een bug** die de FAT kan corrumperen — vermijd `erase` op echte bytecode-bestanden, gebruik alleen `erase` op je testbestand "demo"

---

## Als de docent vraagt: "Waar in de code zie ik...?"

| Vraag | Antwoord |
|-------|----------|
| "Hoe weet je dat processen afwisselend lopen?" | `main.cpp:66` → `runProcesses()` elke loop → `procesHandeling.cpp:273` → 1 instructie per proces |
| "Hoe wordt state bijgehouden?" | `procesHandeling.h:7` → `byte state` in struct |
| "Hoe werkt de FAT?" | `EEPROMHandeling.h:9` → `fileInfo` struct, byte 0 = `noOfFiles`, FAT = byte 1-160, data vanaf 161 |
| "Hoe werkt de stack per proces?" | `procesHandeling.h:17` → `byte stack[STACKSIZE]` + `int sp` per proces |
| "Hoe weet setVar bij welk proces een variabele hoort?" | `memoryHandeling.h:8` → `memoryEntry` heeft `procesID` veld |
| "Wat gebeurt er bij kill?" | `procesHandeling.cpp:125-148` → state='0', `deleteProcesVars()`, `removeProcesFromList()` schuift tabel in |
| "Waarom is round-robin non-blocking?" | `CLIHandeling.cpp:7` → `readToken()` returned `false` als er geen input is, processen draaien door |

---

## Bytecode her-uploaden

Zie `converter/upload2.py` + `converter/README.txt`

```powershell
cd "converter"
python upload2.py hello test_vars test_loop write_file test_delay loopx loopy child testfork
```

---

## Beoordelingscriteria (volledig)

| # | Criteria | Aantonen |
|---|----------|----------|
| 1 | Het OS leest opdrachten die op de command line worden gegeven | Elk willekeurig commando van het OS uitvoeren |
| 2 | Vanuit de command line kan data in bestanden worden opgeslagen, lijst getoond, inhoud getoond, bestanden gewist en vrije ruimte getoond | `store`, `retrieve`, `erase`, `files`, `freespace` |
| 3 | Er kunnen variabelen opgeslagen worden van de typen CHAR, INT, FLOAT en STRING, teruggehaald en van waarde veranderd | `run test_vars` |
| 4 | Een bestand kan als proces gestart, gepauzeerd en gestopt worden | `run`, `suspend`, `resume`, `kill` |
| 5 | Van processen wordt bijgehouden: toestand (running/suspended/terminated), program counter, variabelen | `list` + code (procesEntry struct) |
| 6 | Het OS kan instructies in een bytecode-programma uitvoeren | Elk `run` commando dat bytecode uitvoert |
| 7 | Het OS kan meerdere processen tegelijk uitvoeren (afwisselend 1 instructie) | `run loopx` + `run loopy` tegelijk |
| 8 | Vanuit bytecode kan data van CHAR, INT, FLOAT, STRING naar bestanden geschreven en gelezen worden | `run write_file` (en uitlezen) |
| 9 | Vanuit bytecode kunnen andere processen opgestart (geforkt) en gewacht worden op voltooiing | `run testfork` |
