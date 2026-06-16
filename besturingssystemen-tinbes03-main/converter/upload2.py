"""Minimal upload - one file at a time, clean approach"""
import serial, time, struct, os, sys

INSTRUCTIONS = {k:v for k,v in {
    "CHAR":1,"INT":2,"STRING":3,"FLOAT":4,"SET":5,"GET":6,"INCREMENT":7,
    "DECREMENT":8,"PLUS":9,"MINUS":10,"TIMES":11,"DIVIDEDBY":12,"MODULUS":13,
    "UNARYMINUS":14,"EQUALS":15,"NOTEQUALS":16,"LESSTHAN":17,"LESSTHANOREQUALS":18,
    "GREATERTHAN":19,"GREATERTHANOREQUALS":20,"LOGICALAND":21,"LOGICALOR":22,
    "LOGICALXOR":23,"LOGICALNOT":24,"BITWISEAND":25,"BITWISEOR":26,"BITWISEXOR":27,
    "BITWISENOT":28,"TOCHAR":29,"TOINT":30,"TOFLOAT":31,"ROUND":32,"FLOOR":33,
    "CEIL":34,"MIN":35,"MAX":36,"ABS":37,"CONSTRAIN":38,"MAP":39,"POW":40,"SQ":41,
    "SQRT":42,"DELAY":43,"DELAYUNTIL":44,"MILLIS":45,"PINMODE":46,"ANALOGREAD":47,
    "ANALOGWRITE":48,"DIGITALREAD":49,"DIGITALWRITE":50,"PRINT":51,"PRINTLN":52,
    "OPEN":53,"CLOSE":54,"WRITE":55,"READINT":56,"READCHAR":57,"READFLOAT":58,
    "READSTRING":59,"IF":128,"ELSE":129,"ENDIF":130,"WHILE":131,"ENDWHILE":132,
    "LOOP":133,"ENDLOOP":134,"STOP":135,"FORK":136,"WAITUNTILDONE":137,"THEN":138,"DO":139,
}.items()}

def read_tokens(path):
    with open(path,'rb') as f: data=f.read().decode('utf-8','ignore')
    tokens,i=[],0
    while i<len(data):
        c=data[i]
        if c in ' \t\n\r': i+=1; continue
        if c=='"':
            i+=1; s='"'
            while i<len(data) and data[i]!='"':
                if data[i]=='\\': i+=1; s+=data[i]
                else: s+=data[i]
                i+=1
            s+='"'; tokens.append(s)
            if i<len(data): i+=1
        elif c=="'":
            i+=1; tokens.append(f"'{data[i]}'"); i+=1
            if i<len(data) and data[i]=="'": i+=1
        else:
            j=i
            while i<len(data) and data[i] not in ' \t\n\r"\'':
                i+=1
            t=data[j:i]
            if t: tokens.append(t)
    return tokens

def convert(path):
    tokens=read_tokens(path); prog=bytearray(); stack=[]
    for tok in tokens:
        if tok.startswith("'") and tok.endswith("'") and len(tok)>=3:
            prog.append(1); prog.append(ord(tok[1]))
        elif tok.startswith('"') and tok.endswith('"'):
            prog.append(3); inner=tok[1:-1]; i=0
            while i<len(inner):
                if inner[i]=='\\': i+=1
                prog.append(ord(inner[i])); i+=1
            prog.append(0)
        elif tok.upper().startswith("0X"): prog.append(int(tok,16))
        elif '.' in tok or (tok.startswith('-') and '.' in tok[1:]):
            prog.append(4); b=struct.pack('<f',float(tok))
            for j in range(3,-1,-1): prog.append(b[j])
        elif tok.lstrip('-').isdigit():
            prog.append(2); b=struct.pack('<h',int(tok))
            prog.append(b[1]); prog.append(b[0])
        elif tok.upper() in INSTRUCTIONS:
            op=INSTRUCTIONS[tok.upper()]
            if op==138: prog.append(128); stack.append(len(prog)); prog.append(0)
            elif op==129:
                if stack: p=stack.pop(); prog[p]=len(prog)-p-2; stack.append(len(prog)); prog.append(0)
            elif op in (130,132):
                if stack: p=stack.pop(); prog[p]=len(prog)-p-2
            elif op==131: stack.append(len(prog)-1)
            elif op==139:
                prog[-1]=131
                if stack: p=stack.pop(); prog.append(len(prog)-p-1); stack.append(len(prog)); prog.append(0)
            else: prog.append(op)
        elif len(tok)==1: prog.append(ord(tok))
    return bytes(prog)

def safeprint(b):
    return ''.join(chr(x) if 32<=x<=126 else '?' for x in b[:80])

def main():
    d=os.path.dirname(os.path.abspath(__file__))
    files=sys.argv[1:]
    
    ser=serial.Serial('COM3',9600,timeout=0.3)
    time.sleep(2)
    
    # Read boot
    boot=ser.read(4096)
    print('BOOT:', safeprint(boot[-50:]))
    
    # Skip clear - EEPROM is already clean
    
    # Upload files one by one
    for f in files:
        p=os.path.join(d,f)
        if not os.path.isfile(p): print(f'  {f}: SKIP (not found)'); continue
        data=convert(p)
        size=len(data)
        
        # Flush and wait
        ser.read(4096)
        time.sleep(0.3)
        
        # Send store command
        ser.write(f'store {f} {size}\n'.encode())
        time.sleep(0.2)
        
        # Send raw data
        ser.write(data)
        time.sleep(1)
        
        # Read response
        out=b''
        t0=time.time()
        while time.time()-t0<3:
            ch=ser.read(256)
            if ch: out+=ch
            if b'Succesfully' in out or b'Not enough' in out: break
            time.sleep(0.05)
        
        ok='Succesfully' in out.decode('ascii','replace')
        print(f'  {f} ({size}b): {"OK" if ok else "FAIL"} | {safeprint(out[-40:])}')
    
    # Verify
    time.sleep(0.5)
    ser.read(4096)
    ser.write(b'files\n')
    time.sleep(1)
    out=ser.read(4096)
    print('\nFILES:', safeprint(out[:200]))
    ser.close()

if __name__=='__main__': main()
