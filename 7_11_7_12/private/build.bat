del password.7z
del challenge.7z
del part2\flag.7z
del part2\part2\*
cd part1
"\Program Files\7-Zip\7z.exe" a ..\password.7z password.txt
cd ..\part2
py -3 part2_generate_files.py flag_part2.png
"\Program Files\7-Zip\7z.exe" a -pgive_me_the_flag flag.7z flag.txt part2
cd ..
py -3 insert_gap.py password.7z part2\flag.7z challenge.7z
