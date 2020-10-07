obj-m += yaknote.o
VERSION=5.8.11

all:
	make -C../linux-${VERSION} M=$(PWD) modules

clean:
	make -C ../linux${VERSION}/build M=$(PWD) clean