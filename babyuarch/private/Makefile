CC=gcc
STRIP=strip

all: chall

chall: chall.o
	$(CC) $(CFLAGS) -o $@ $^
	$(STRIP) $@
	cp $@ ../public


debug: CFLAGS += -DDEVBUILD
debug: chall.o
	$(CC) $(CFLAGS) -o chall_debug  $^

.PHONY: clean
clean:
	rm -f *.o
	rm -f chall
	rm -f chall_debug
