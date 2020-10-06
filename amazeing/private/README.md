The setup of the challenge is a bit convoluted:

1. generate a maze using the maze_generator.c (written by Joe Wingbermuehle in 1999!):
`gcc maze_generator.c -o maze_gen; gen_maze 60 60 s > maze.txt`

2. Call mazegen.py, which creates the challenge file (maze), and the solution (solve.in)

3. Verify solution is indeed working:
`cat solve.in | qemu-arm -d in_asm -D out.log ./maze && python verify.py`
