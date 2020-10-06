#!/usr/bin/env python3

vvar_location = 0x7ffff7ffb080

src = '''
//
// 256 byte buffer:
// set half of the bytes to 0x00, and the other half to 0x08
//
mov eax, 0x08080808
''' + ''.join(f'''
mov rdx, 0x7fffffffd0{i:02x}
mov [rdx], rax
''' for i in range(0, 0x100, 8)) + '''

//
// this part of [vvar] seems to be a timer or so
// we will use the lower 8 bits as an RNG
//
mov rbx, ''' + hex(vvar_location) + '''
mov rdx, 0x7fffffffd000
mov dl, [rbx]
// rdx now points to a byte that is either 0x00 or 0x08

// 0x...00: "TASTE"
mov rax, 0x4554534154
mov rsi, 0x7fffffffe000
mov [rsi], rax
// 0x...08: "LESS!"
mov rax, 0x215353454c
mov rsi, 0x7fffffffe008
mov [rsi], rax

// use the RNG to select 0x00 or 0x08
// for the LSB
mov sil, [rdx]

// write(stdout, ..., 5)
mov edi, 1
mov edx, 5
'''

for line in src.splitlines():
    if line.startswith('mov'):
        print(line.strip())
