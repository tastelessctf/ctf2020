CC=gcc
CFLAGS=-I. -D_GNU_SOURCE

all: crackme.c crackme.o
	$(CC) -o crackme crackme.o

obfuscated: crackme_scopfuscated.o
	$(CC) $(CFLAGS) -o crackme -O2 $^
	strip crackme

clean:
	rm -f *.o
	rm -f crackme

