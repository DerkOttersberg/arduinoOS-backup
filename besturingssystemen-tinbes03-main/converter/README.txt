ArduinOS Bytecode Upload Tool
=============================

Upload bytecode programs to Arduino running ArduinOS.

Quick start
-----------

    cd converter
    python upload2.py hello test_vars test_loop write_file test_delay loopx loopy child testfork

Or upload a single file:

    python upload2.py my_program

Requirements
------------

- Python 3 with pyserial:  pip install pyserial
- Arduino connected on COM3 with Serial Monitor closed

Writing bytecode
----------------

Text format (same as convert.exe):

    "Hello, world\n" PRINT     -- string
    'x'                        -- char
    123                        -- int
    123.45                     -- float
    0x0A                       -- raw hex byte
    LOOP ... ENDLOOP           -- infinite loop
    SET a / GET a              -- variable store/retrieve (single-letter name)
    INCREMENT / DECREMENT      -- unary operators
    PLUS / MINUS / TIMES / ... -- binary operators
    IF ... THEN ... ELSE ... ENDIF   -- conditional
    WHILE ... DO ... ENDWHILE        -- while loop
    MILLIS 1000 PLUS DELAYUNTIL      -- non-blocking delay
    "filename" FORK                  -- fork child process
    WAITUNTILDONE                    -- wait for child
    STOP                             -- terminate process

See instruction_set.h for all opcodes.
