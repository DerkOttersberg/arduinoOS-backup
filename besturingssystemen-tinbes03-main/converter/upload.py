"""Robust bytecode uploader"""
import serial
import time
import struct
import os
import sys

INSTRUCTIONS = {
    "CHAR":1,"INT":2,"STRING":3,"FLOAT":4,"SET":5,"GET":6,
    "INCREMENT":7,"DECREMENT":8,"PLUS":9,"MINUS":10,"TIMES":11,
    "DIVIDEDBY":12,"MODULUS":13,"UNARYMINUS":14,"EQUALS":15,
    "NOTEQUALS":16,"LESSTHAN":17,"LESSTHANOREQUALS":18,
    "GREATERTHAN":19,"GREATERTHANOREQUALS":20,"LOGICALAND":21,
    "LOGICALOR":22,"LOGICALXOR":23,"LOGICALNOT":24,"BITWISEAND":25,
    "BITWISEOR":26,"BITWISEXOR":27,"BITWISENOT":28,"TOCHAR":29,
    "TOINT":30,"TOFLOAT":31,"ROUND":32,"FLOOR":33,"CEIL":34,
    "MIN":35,"MAX":36,"ABS":37,"CONSTRAIN":38,"MAP":39,"POW":40,
    "SQ":41,"SQRT":42,"DELAY":43,"DELAYUNTIL":44,"MILLIS":45,
    "PINMODE":46,"ANALOGREAD":47,"ANALOGWRITE":48,"DIGITALREAD":49,
    "DIGITALWRITE":50,"PRINT":51,"PRINTLN":52,"OPEN":53,"CLOSE":54,
    "WRITE":55,"READINT":56,"READCHAR":57,"READFLOAT":58,
    "READSTRING":59,"IF":128,"ELSE":129,"ENDIF":130,"WHILE":131,
    "ENDWHILE":132,"LOOP":133,"ENDLOOP":134,"STOP":135,
    "FORK":136,"WAITUNTILDONE":137,"THEN":138,"DO":139,
}

def safe_bytes(b):
    return ''.join(chr(x) if 32<=x<=126 else '.' for x in b[:100])

def read_tokens(path):
    with open(path,'rb') as f:
        data = f.read().decode('utf-8','ignore')
    tokens=[]
    i=0
    n=len(data)
    while i<n:
        c=data[i]
        if c in ' \t\n\r':
            i+=1
            continue
        if c=='"':
            i+=1
            s='"'
            while i<n and data[i]!='"':
                if data[i]=='\\':
                    i+=1
                    s+=chr(ord(data[i]))
                else:
                    s+=data[i]
                i+=1
            s+='"'
            if i<n: i+=1
            tokens.append(s)
            continue
        if c=="'":
            i+=1
            if data[i]=='\\':
                i+=1
                tokens.append(f"'\\{data[i]}'")
            else:
                tokens.append(f"'{data[i]}'")
            i+=1
            if i<n and data[i]=="'": i+=1
            continue
        start=i
        while i<n and data[i] not in ' \t\n\r"\'':
            i+=1
        t=data[start:i]
        if t: tokens.append(t)
    return tokens

def convert(path):
    tokens=read_tokens(path)
    prog=bytearray()
    stack=[]
    for tok in tokens:
        if tok.startswith("'") and tok.endswith("'") and len(tok)>=3:
            prog.append(1)
            ch=tok[1:-1]
            if len(ch)==2 and ch[0]=='\\':
                prog.append(ord({'n':'\n','r':'\r','t':'\t'}.get(ch[1],ch[1])))
            else:
                prog.append(ord(ch[0]))
        elif tok.startswith('"') and tok.endswith('"'):
            prog.append(3)
            inner=tok[1:-1]
            i=0
            while i<len(inner):
                if inner[i]=='\\':
                    i+=1
                    prog.append(ord({'n':'\n','r':'\r','t':'\t'}.get(inner[i],inner[i])))
                else:
                    prog.append(ord(inner[i]))
                i+=1
            prog.append(0)
        elif tok.upper().startswith("0X"):
            prog.append(int(tok,16))
        elif '.' in tok or (tok.startswith('-') and '.' in tok[1:]):
            prog.append(4)
            b=struct.pack('<f',float(tok))
            for j in range(3,-1,-1): prog.append(b[j])
        elif tok.lstrip('-').isdigit():
            prog.append(2)
            b=struct.pack('<h',int(tok))
            prog.append(b[1]); prog.append(b[0])
        elif tok.upper() in INSTRUCTIONS:
            op=INSTRUCTIONS[tok.upper()]
            if op==138:  # THEN
                prog.append(128); stack.append(len(prog)); prog.append(0)
            elif op==129:  # ELSE
                if stack:
                    p=stack.pop(); prog[p]=len(prog)-p-2
                    stack.append(len(prog)); prog.append(0)
            elif op in (130,132):  # ENDIF/ENDWHILE
                if stack: p=stack.pop(); prog[p]=len(prog)-p-2
            elif op==131:  # WHILE
                stack.append(len(prog)-1)
            elif op==139:  # DO
                prog[-1]=131
                if stack:
                    p=stack.pop(); prog.append(len(prog)-p-1)
                    stack.append(len(prog)); prog.append(0)
            else:
                prog.append(op)
        elif len(tok)==1:
            prog.append(ord(tok))
    return bytes(prog)

def cmd(ser, s):
    ser.write((s+'\r\n').encode())
    time.sleep(0.3)
    raw=b''
    t0=time.time()
    while time.time()-t0<3:
        ch=ser.read(256)
        if ch:
            raw+=ch
            if b'>' in raw: break
        else: time.sleep(0.03)
    return raw

def store_file(ser, name, data):
    size=len(data)
    ser.read(4096)
    ser.write(f'store {name} {size}\r\n'.encode())
    time.sleep(0.15)
    for i in range(0,size,64):
        ser.write(data[i:i+64])
        time.sleep(0.08)
    time.sleep(0.5)
    raw=b''
    t0=time.time()
    while time.time()-t0<5:
        ch=ser.read(256)
        if ch:
            raw+=ch
            if b'> ' in raw: break
        else: time.sleep(0.03)
    ok=b'Succesfully' in raw
    print(f'  {name} ({size}b): {"OK" if ok else "FAIL"} {safe_bytes(raw[-40:])}')
    return ok

def main():
    d=os.path.dirname(os.path.abspath(__file__))
    files=sys.argv[1:] if len(sys.argv)>1 else [
        "hello","test_vars","test_loop","write_file","test_delay",
        "loopx","loopy","child","testfork"
    ]
    
    ser=serial.Serial('COM3',9600,timeout=0.2)
    time.sleep(1.5)
    ser.read(4096)  # flush boot
    
    # Clear EEPROM
    print('Clearing EEPROM...')
    ser.write(b'clearEEPROM\r\n')
    time.sleep(1)
    ser.read(4096)
    
    for f in files:
        p=os.path.join(d,f)
        if not os.path.exists(p):
            print(f'  {f}: NOT FOUND')
            continue
        data=convert(p)
        store_file(ser, f, data)
        time.sleep(0.15)
    
    # Verify
    print('\n--- files ---')
    raw=cmd(ser, 'files')
    print(raw.decode('ascii','replace'))
    ser.close()
    print('Done!')

if __name__=='__main__':
    main()
