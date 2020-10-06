#!/usr/bin/env python3

import sys
import subprocess

from random import randint
from pwn import *
import re


def bits(s):
    for c in s:
        yield from (int(bit) for bit in bin(ord(c))[2:].zfill(8))


CC = 'arm-linux-gnueabi-gcc'

FLAG = 'tstlss{br4nch_sign1fic4nt_b1t_st3g0...do_u_hate_us_now??}\x00'
flag_gen = bits(FLAG)
MAZEFILE = 'maze.txt'


outfile = open('maze.c', 'w')

INCLUDES = '''#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

'''

MAIN = '''
/* thanks stackoverflow for termios code! https://stackoverflow.com/a/38316343 */
static struct termios oldTermios, newTermios;

void main(){
    tcgetattr(STDIN_FILENO, &oldTermios);
    newTermios = oldTermios;
    cfmakeraw(&newTermios);



    puts("Welcome to TastelessMaze!");
    puts("Use WASD to navigate, Q to exit");
    fn_0_1();
}

'''


TEMPLATE_FIELD = '''
__attribute__((noreturn)) void fn_{}_{}() {{
    puts("{}");
    while(1) {{
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        char dir = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);

        switch (dir) {{
            case 'w':
            case 'W':
                {}
                break;
            case 'd':
            case 'D':
                {}
                break;
            case 's':
            case 'S':
                {}
                break;
            case 'a':
            case 'A':
                {}
                break;
            case 'q':
            case 'Q':
                exit(0);
            default:
                break;
        }}
   }}
}}

'''

SUCC_STRINGS = ["Fine.",
                "Okay.",
                "Yes, this is a way",
                "Wow",
                "Impressive",
                "Step by step",
                "The smell of cake intensifies",
                "Wonderful.",
                "Oh no, you are getting closer to the exit!",
                "You were always good at walking, weren't you?"
               ]


FAIL_STRINGS = ["This is a wall -.-",
               "Ouch!",
               "Nope",
               "Solid stone",
               "OUCH!!!!",
               "Uh... yeah, no.",
               "Lolnope",
               "Did you not SEE that WALL?!",
               "You keep doing this. Don't.",
               "The cake is the other way",
               "What a horrible night to have a curse",
              ]



def maze_entry(maze, x, y):
    outfile.write(f'''
__attribute__((noreturn)) void fn_{y}_{x}() {{
    puts("This is the beginning. You can only go south.");
    while(1) {{
        tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);
        char dir = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
        switch (dir) {{
            case 's':
            case 'S':
                fn_{y+1}_{x}();
                break;
            case 'q':
            case 'Q':
                exit(0);
            default:
                break;
        }}
    }}
}}
''')


def maze_exit(maze, x, y):
    outfile.write(f'''
__attribute__((noreturn)) void fn_{y}_{x}() {{
    puts("Wow, you made it through the maze! Amazing. \\n"
    "Did you see the flag along the way? If not, you may need to go faster!");
    exit(0);
}}
''')


def emit_cell(maze, x, y):
    call_templ = "                fn_{}_{}();"
    fail_templ = "                puts(\"{}\");"

    fmt1 = call_templ.format(y-1, x) if maze[y-1][x] != '*' else fail_templ.format(FAIL_STRINGS[ randint(0, len(FAIL_STRINGS)-1) ])
    fmt2 = call_templ.format(y, x+1) if maze[y][x+1] != '*' else fail_templ.format(FAIL_STRINGS[randint(0, len(FAIL_STRINGS)-1)] )

    fmt3 = call_templ.format(y+1, x) if maze[y+1][x] != '*' else fail_templ.format(FAIL_STRINGS[randint(0, len(FAIL_STRINGS)-1)])

    fmt4 = call_templ.format(y, x-1) if maze[y][x-1] != '*' else fail_templ.format(FAIL_STRINGS[randint(0, len(FAIL_STRINGS)-1)])


    outfile.write( TEMPLATE_FIELD.format(y, x,
                                    SUCC_STRINGS[randint(0, len(SUCC_STRINGS)-1)],
                                    fmt1, fmt2, fmt3, fmt4) )





def generate_maze():
    with open(MAZEFILE, 'r') as f:
        maze = f.read()


    maze_bits = maze.count("-")
    print(f"This maze can hide {maze_bits} bits")
    print(f"This Flag has {len(list(bits(FLAG)))} bits")
    assert maze_bits+1 >= len(FLAG) * 8 #last flag bits are 0 byte, so a bit oob is ok
    maze = maze.splitlines()[1::]

    outfile.write( INCLUDES )

    # create forward declarations
    for y in range(len(maze)):
        for x in range(len(maze[0])):
               if maze[y][x] != '*':
                   outfile.write(f'void fn_{y}_{x}();\n')

    outfile.write( MAIN )


    for y in range(len(maze)):
        for x in range(len(maze[0])):
            #sys.stdout.write(maze[x][y])
            if maze[y][x] != "*":
                if y == 0 and x==1:
                    maze_entry(maze, x, y)
                elif y == len(maze[0])-1 and x == len(maze)-2:
                    maze_exit(maze, x, y)
                else:
                    emit_cell(maze, x, y)
    outfile.close()
    subprocess.run([CC, '-o', 'maze', 'maze.c', '-static'])






def process_fn(e, fn_name, use_flag=False, correct_fn_addr=None):
    print(f"Processing fn {fn_name}")

    bl_regex = re.compile(r'^\s+([0-9a-z]+):\s+([0-9a-z]+)\s+bl\w?\s+0x([0-9a-z]+)$', re.M )
    cur_fn = fn_name
    fns = sorted(e.sym.items(), key=lambda x: x[1])

    denylist_addrs = [
        e.symbols['exit'],
        e.symbols['puts'],
    ]

    # get disassembly of cur_fn by searching for the sequental next_fn in mem
    fn_idx = fns.index((cur_fn, e.symbols[cur_fn]))
    next_fn = fn_idx+1
    fn_length = e.symbols[fns[next_fn][0]] - e.symbols[cur_fn]
    disas = e.disasm(e.symbols[cur_fn], fn_length)

    # very simple preprocessing: look for addls insn opening the jmptable
    to_process =  disas[disas.find('addls'):]
    to_patch = bl_regex.findall(to_process)

    for entry in to_patch:
        addr, raw_bytes, branch_target = [int(x, 16) for x in entry]
        if branch_target in denylist_addrs:
            continue
        # get either fake bit, or flag bit
        #import IPython; IPython.embed()

        if branch_target == correct_fn_addr:
            try:
                bit = next(flag_gen)
            except:
                bit = randint(0,1)
        else:
            bit = randint(0,1)

        # set the branch condition
        if bit:
            val = (raw_bytes << 4 & 0xffffffff) >> 4 ^ 0xe << 28
        else:
            val = (raw_bytes << 4 & 0xffffffff) >> 4 ^ 0x9 << 28
        e.p32(addr, val)





def embed_solution():
    with open(MAZEFILE, 'r') as f:
        maze = f.read()
    maze = maze.splitlines()[1::]


    solve_file = open('solve.in', 'w')

    context.arch = 'arm'
    e = ELF('./maze')



    y, x = (0, 1)
    last_y, last_x = (0, 1)


    visited_fns = []


    # first-pass: patch functions along the right path
    while True:
        cur_fn = f'fn_{y}_{x}'

        if y == len(maze[0])-1 and x == len(maze)-2:
            break

        # determine correct fn - the one we need for solving the maze
        if maze[y][x+1] == '-'   and  (y, x+1) not in visited_fns:
            solve_file.write('D')
            last_x = x
            x += 1
        elif maze[y+1][x] == '-' and  (y+1, x) not in visited_fns:
            solve_file.write('S')
            last_y = y
            y += 1
        elif maze[y][x-1] == '-' and  (y, x-1) not in visited_fns:
            solve_file.write('A')
            last_x = x
            x -= 1
        elif maze[y-1][x] == '-' and  (y-1, x) not in visited_fns:
            solve_file.write('W')
            last_y = y
            y -= 1
        visited_fns.append(  (y, x) )
        #if last_y == y and last_x == x or x==11:
            #import IPython; IPython.embed()


        correct_fn = f'fn_{y}_{x}'
        correct_fn_addr = e.symbols[correct_fn]

        process_fn(e, cur_fn, use_flag=True, correct_fn_addr=correct_fn_addr)
        cur_fn = correct_fn


    # second pass: process EVERYTHING
    for y in range(len(maze)):
        for x in range(len(maze[0])):
            #sys.stdout.write(maze[x][y])
            if maze[y][x] != "*" and (y, x) not in visited_fns:
                process_fn(e, f'fn_{y}_{x}', use_flag=False)

        #print(disasm(e.symbols[cur_fn]
    #import IPython; IPython.embed()


    e.save()



def main():

    generate_maze()
    embed_solution()

    # to verify solution gives flag:
    # cat solve.in | qemu-arm -d in_asm -D out.log ./maze && python verify.py







if __name__ == '__main__':
    main()
