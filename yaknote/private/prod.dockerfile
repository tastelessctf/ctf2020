FROM ubuntu as linux-build
RUN apt-get update && apt-get -y install build-essential wget flex bison bc
RUN wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.8.11.tar.xz
RUN tar xf linux-5.8.11.tar.xz
WORKDIR /linux-5.8.11/
RUN apt-get -y install libelf-dev libssl-dev
ADD kernel-config /linux-5.8.11/.config
RUN make -j32

ADD module /module
WORKDIR /module/
RUN make

FROM ubuntu as busybox-build
RUN apt-get update && apt-get -y install build-essential wget flex bison bc
RUN wget https://busybox.net/downloads/busybox-1.32.0.tar.bz2
RUN tar xf busybox-1.32.0.tar.bz2
WORKDIR /busybox-1.32.0
ADD busybox-config /busybox-1.32.0/.config
RUN make -j32

FROM ubuntu as pack-initrd
RUN apt-get update && apt-get -y install cpio
COPY --from=busybox-build /busybox-1.32.0/busybox /rootfs/bin/busybox
WORKDIR /rootfs/
ADD passwd ./etc/passwd
RUN mkdir -p ./home/user/ && chown 1000:1000 ./home/user
COPY --from=linux-build /module/yaknote.ko .
ADD init .
ADD flag.txt /rootfs/flag.txt
RUN chown 0:0 flag.txt && chmod 600 flag.txt
RUN find . -print0 | cpio --null -ov --format=newc | gzip -9 > /initrd.gz

FROM ubuntu as pack-initrd-noflag
RUN apt-get update && apt-get -y install cpio
COPY --from=busybox-build /busybox-1.32.0/busybox /rootfs/bin/busybox
WORKDIR /rootfs/
ADD passwd ./etc/passwd
RUN mkdir -p ./home/user/ && chown 1000:1000 ./home/user
COPY --from=linux-build /module/yaknote.ko .
ADD init .
ADD fake-flag.txt /rootfs/flag.txt
RUN chown 0:0 flag.txt && chmod 600 flag.txt
RUN find . -print0 | cpio --null -ov --format=newc | gzip -9 > /initrd.gz

FROM ubuntu
RUN apt-get update && apt-get -y install qemu-system-x86
#COPY --from=pack-initrd /initrd.gz /initrd.gz
COPY --from=pack-initrd-noflag /initrd.gz /initrd.gz
COPY --from=linux-build /linux-5.8.11/arch/x86/boot/bzImage /bzImage
COPY --from=linux-build /linux-5.8.11/vmlinux /vmlinux
ADD ./entrypoint.sh /entrypoint.sh
ENTRYPOINT [ "/entrypoint.sh" ]