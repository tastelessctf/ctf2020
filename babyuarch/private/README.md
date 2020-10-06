The challenge idea is based on the recend blindside paper:
https://download.vusec.net/papers/blindside_ccs20.pdf.

Before building the challenge, we reimplemented the idea of the attack in the
userspace (blindside_user_minimized.c). Special thanks to Enes Göktaş for
helping finding bugs in this original replication.

The main challenge itself is in chall.c, while assembler.py generates the
exploit code.

hint.bin is the hint given out during the CTF, which basically shows the timing
difference when accessing the vm_lock_bit dependent on whether it is loaded in
the cache or not.
