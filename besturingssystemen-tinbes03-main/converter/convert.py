"""
Python equivalent of convert.exe - reads text bytecode files,
converts to binary, uploads to Arduino via serial.
"""
import serial
import time
import sys
import os
import struct

INSTRUCTIONS = {
    "CHAR": 1, "INT": 2, "STRING": 3, "FLOAT": 4,
    "SET": 5, "GET": 6, "INCREMENT": 7, "DECREMENT": 8,
    "PLUS": 9, "MINUS": 10, "TIMES": 11, "DIVIDEDBY": 12,
    "MODULUS": 13, "UNARYMINUS": 14,
    "EQUALS": 15, "NOTEQUALS": 16, "LESSTHAN": 17,
    "LESSTHANOREQUALS": 18, "GREATERTHAN": 19,
    "GREATERTHANOREQUALS": 20,
    "LOGICALAND": 21, "LOGICALOR": 22, "LOGICALXOR": 23,
    "LOGICALNOT": 24,
    "BITWISEAND": 25, "BITWISEOR": 26, "BITWISEXOR": 27,
    "BITWISENOT": 28,
    "TOCHAR": 29, "TOINT": 30, "TOFLOAT": 31,
    "ROUND": 32, "FLOOR": 33, "CEIL": 34,
    "MIN": 35, "MAX": 36, "ABS": 37, "CONSTRAIN": 38, "MAP": 39,
    "POW": 40, "SQ": 41, "SQRT": 42,
    "DELAY": 43, "DELAYUNTIL": 44, "MILLIS": 45,
    "PINMODE": 46, "ANALOGREAD": 47, "ANALOGWRITE": 48,
    "DIGITALREAD": 49, "DIGITALWRITE": 50,
    "PRINT": 51, "PRINTLN": 52,
    "OPEN": 53, "CLOSE": 54, "WRITE": 55,
    "READINT": 56, "READCHAR": 57, "READFLOAT": 58, "READSTRING": 59,
    "IF": 128, "ELSE": 129, "ENDIF": 130,
    "WHILE": 131, "ENDWHILE": 132,
    "LOOP": 133, "ENDLOOP": 134,
    "STOP": 135, "FORK": 136, "WAITUNTILDONE": 137,
    "THEN": 138, "DO": 139,
}

def read_tokens(filepath):
    with open(filepath, 'r') as f:
        text = f.read()
    tokens = []
    i = 0
    while i < len(text):
        if text[i] in ' \t\n\r':
            i += 1
            continue
        if text[i] == '"':
            i += 1
            token = '"'
            while i < len(text) and text[i] != '"':
                if text[i] == '\\':
                    i += 1
                    if text[i] == 'n': token += '\n'
                    elif text[i] == 'r': token += '\r'
                    elif text[i] == 't': token += '\t'
                    else: token += text[i]
                else:
                    token += text[i]
                i += 1
            token += '"'
            if i < len(text): i += 1
            tokens.append(token)
            continue
        if text[i] == "'":
            i += 1
            if text[i] == '\\':
                i += 1
                token = "'\\" + text[i] + "'"
            else:
                token = "'" + text[i] + "'"
            i += 1
            if i < len(text) and text[i] == "'": i += 1
            tokens.append(token)
            continue
        start = i
        while i < len(text) and text[i] not in ' \t\n\r"\'':
            i += 1
        t = text[start:i]
        if t:
            tokens.append(t)
    return tokens

def convert_file(filepath):
    tokens = read_tokens(filepath)
    prog = bytearray()
    if_stack = []

    for token in tokens:
        if token.startswith("'") and token.endswith("'") and len(token) >= 3:
            prog.append(1)
            ch = token[1:-1]
            if len(ch) == 2 and ch[0] == '\\':
                esc = {'n': '\n', 'r': '\r', 't': '\t'}.get(ch[1], ch[1])
                prog.append(ord(esc))
            else:
                prog.append(ord(ch[0]))
        elif token.startswith('"') and token.endswith('"'):
            prog.append(3)
            inner = token[1:-1]
            i = 0
            while i < len(inner):
                if inner[i] == '\\':
                    i += 1
                    esc = {'n': '\n', 'r': '\r', 't': '\t'}.get(inner[i], inner[i])
                    prog.append(ord(esc))
                else:
                    prog.append(ord(inner[i]))
                i += 1
            prog.append(0)
        elif token.upper().startswith("0X"):
            prog.append(int(token, 16))
        elif '.' in token or (token.startswith('-') and '.' in token[1:]):
            prog.append(4)
            val = float(token)
            b = struct.pack('<f', val)
            for j in range(3, -1, -1):
                prog.append(b[j])
        elif token.lstrip('-').isdigit():
            prog.append(2)
            val = int(token)
            b = struct.pack('<h', val)
            prog.append(b[1])
            prog.append(b[0])
        elif token.upper() in INSTRUCTIONS:
            opcode = INSTRUCTIONS[token.upper()]
            if opcode == 138:  # THEN
                prog.append(128)
                if_stack.append(len(prog))
                prog.append(0)
            elif opcode == 129:  # ELSE
                if if_stack:
                    prev = if_stack.pop()
                    prog[prev] = len(prog) - prev - 2
                    if_stack.append(len(prog))
                    prog.append(0)
            elif opcode in (130, 132):  # ENDIF or ENDWHILE
                if if_stack:
                    prev = if_stack.pop()
                    prog[prev] = len(prog) - prev - 2
            elif opcode == 131:  # WHILE
                if_stack.append(len(prog) - 1)
            elif opcode == 139:  # DO
                prog[-1] = 131
                if if_stack:
                    prev = if_stack.pop()
                    prog.append(len(prog) - prev - 1)
                    if_stack.append(len(prog))
                    prog.append(0)
            else:
                prog.append(opcode)
        elif len(token) == 1:
            prog.append(ord(token))
        else:
            print(f"  Warning: unknown token '{token}'")
    return bytes(prog)

def upload_file(ser, name, data):
    size = len(data)
    print(f"  {name} ({size} bytes)...", end=" ", flush=True)
    ser.read(4096)
    ser.write(f"store {name} {size}\n".encode())
    time.sleep(0.1)
    for i in range(0, size, 64):
        ser.write(data[i:i+64])
        time.sleep(0.1)
    time.sleep(0.5)
    raw = b""
    t0 = time.time()
    while time.time() - t0 < 4.0:
        ch = ser.read(256)
        if ch:
            raw += ch
            if b"> " in raw:
                break
            t0 = time.time()
        else:
            time.sleep(0.02)
    if b"Succesfully" in raw:
        print("OK")
    elif b"already exists" in raw:
        print("SKIP (already exists)")
    else:
        txt = raw.decode('ascii', errors='replace')[-120:]
        print(f"FAIL {txt}")
    return raw

def read_prompt(ser, timeout=3.0):
    t0 = time.time()
    buf = b""
    while time.time() - t0 < timeout:
        ch = ser.read(256)
        if ch:
            buf += ch
            if b"> " in buf:
                break
            t0 = time.time()
        else:
            time.sleep(0.02)
    return buf

def main():
    converter_dir = os.path.dirname(os.path.abspath(__file__))
    files = sys.argv[1:] if len(sys.argv) > 1 else [
        "hello", "test_vars", "test_loop",
        "write_file", "test_delay", "loopx", "loopy", "child", "testfork"
    ]
    print("Connecting to COM3...")
    ser = serial.Serial('COM3', 9600, timeout=0.2)
    time.sleep(1)
    boot = read_prompt(ser)
    print(f"Arduino: {boot.decode('ascii', errors='replace').strip()[-80:]}")
    # Skip clearEEPROM - only clear when explicitly needed
    # print("Clearing EEPROM...")
    # ser.write(b"clearEEPROM\n")
    # time.sleep(0.5)
    # read_prompt(ser)
    # ser.read(4096)
    for fname in files:
        fpath = os.path.join(converter_dir, fname)
        if not os.path.exists(fpath):
            print(f"  File '{fname}' not found, skipping")
            continue
        name = os.path.basename(fname)
        data = convert_file(fpath)
        upload_file(ser, name, data)
        time.sleep(0.2)
    print("\nVerifying...")
    ser.write(b"files\n")
    time.sleep(0.5)
    out = read_prompt(ser)
    print(out.decode('ascii', errors='replace'))
    ser.close()
    print("Done!")

if __name__ == "__main__":
    main()
