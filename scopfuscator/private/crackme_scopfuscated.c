// ------------------------------------------------------------------------------------------------
// Tasteless CTF 2020 - scopfuscator crackme
//
// This is a classic crackme. Flag is broken into 6-byte groups and each group is verified
// individually.
//
// Flag format: tstlss{aaaaaabbbbbbccccccdddddd}
// Final flag : tstlss{$C0PfU5c4ToR_1z_so_c00l!}
//
// ------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "scopfuscator.h"

// Uncomment for debug version.
//#define __DEBUG__


// ------------------------------------------------------------------------------------------------
// CRC32 Implementation (copied from: https://web.mit.edu/freebsd/head/sys/libkern/crc32.c)
//
// Note: I swapped some values from the table to add some confusion (:evil)
const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x5268e236, 0x2802b89e, 0x5f058808, 0xc60cd9b2,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x196c3671, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0xe5d5be0d, 0x856530d8,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb10be924,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf262004e,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0x5edef90e, 0xbc66831a, 0x256fd2a0,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x086d3d2d,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


// ------------------------------------------------------------------------------------------------
uint32_t add(uint32_t a, uint32_t b) { return a + b;  }
uint32_t sub(uint32_t a, uint32_t b) { return a - b;  }
uint32_t shl(uint32_t a, uint32_t b) { return a << b; }
uint32_t shr(uint32_t a, uint32_t b) { return a >> b; }
uint32_t and(uint32_t a, uint32_t b) { return a & b;  }
uint32_t or (uint32_t a, uint32_t b) { return a | b;  }
uint32_t xor(uint32_t a, uint32_t b) { return a ^ b;  }
uint32_t mul(uint32_t a, uint32_t b) { return a * b;  }
uint32_t mod(uint32_t a, uint32_t b) { return a % b;  }
uint32_t not(uint32_t a) { return ~a; }
uint32_t rol(uint32_t a, uint32_t b) { return or(shl(a, b), shr(a, sub(32, b))); }
uint32_t ror(uint32_t a, uint32_t b) { return or(shr(a, b), shl(a, sub(32, b))); }


uint32_t adds(uint32_t a, uint32_t b) { RET( a + b; )  }
uint32_t subs(uint32_t a, uint32_t b) { RET( a - b; ) }
uint32_t shls(uint32_t a, uint32_t b) { RET( a << b;) }
uint32_t shrs(uint32_t a, uint32_t b) { RET( a >> b;) }
uint32_t ands(uint32_t a, uint32_t b) { RET( a & b; ) }
uint32_t ors (uint32_t a, uint32_t b) { RET( a | b; ) }
uint32_t xors(uint32_t a, uint32_t b) { RET(a ^ b;) }
uint32_t muls(uint32_t a, uint32_t b) { RET(a * b;)  }
uint32_t mods(uint32_t a, uint32_t b) { RET(a % b;)  }
uint32_t nots(uint32_t a) { RET(~a;) }
uint32_t rols(uint32_t a, uint32_t b) { RET ( or(shl(a, b), shr(a, sub(32, b)));) }
uint32_t rors(uint32_t a, uint32_t b) { RET ( or(shr(a, b), shl(a, sub(32, b)));) }

// ------------------------------------------------------------------------------------------------
//  Verify group #1
//
//  How to break it: There are 3 rounds. Each round takes 3 characters from the flag and
//  calculates 100 the (custom) CRC32. Brute force.
//

int get_pair_1(int idx) {
    size_t iterations[] = {1, 2, 4};
    RET(iterations[idx];)
}

int get_pair_2(int idx) {
    size_t iterations[] = {2, 0, 3};
    RET(iterations[idx];)
}

int get_pair_3(int idx) {
    size_t iterations[] = {3, 5, 0};
    RET(iterations[idx];)
}

int get_target_value_1(int idx) {
    uint32_t target_values[] = {0x7BA3E0DC, 0xE7F42D9A, 0xD9A4F450};
    RET (target_values[idx];)
}



// ------------------------------------------------------------------------------------------------
//  Verify group #2
//
//  How to break it: Work backwards. From round i you can go to round i-1. All operations are
//  invertible.
//
int get_target_value_2(int idx) {
    uint32_t target_values[] = {0x9B3ECCA5, 0x0EA10FB0, 0x5F18A534, 0xCEBCE886,
                                0xB8A0E16B, 0x8EBA9263};
    RET(target_values[idx];)
}

static inline int verify_group_2(char *flag) {
    int result = 0;

    return result;
}


// ------------------------------------------------------------------------------------------------
//  Verify group #3
//
//  How to break it: Modulo inverse from 3 characters.
//
int get_target_value_3(int idx) {
    uint32_t target_values[] = {0xB30BB088, 0x60CCE52C, 0x8BB5A7C0, 0x3D4B4A14};
    RET(target_values[idx]);
}

static inline int verify_group_3(char *flag) {

    //return result;
}

// taken from https://clc-wiki.net/wiki/C_standard_library:string.h:strlen
size_t strlen(const char *s) {
    size_t i;
    for (i = 0; s[i] != '\0'; i++) ;
    RET(i);
    return i;
}


// ------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    char flag[128] = {0};
    size_t flaglen = 0;
    int result = 0;
    int combined_result = 0;

#ifndef __DEBUG__
    CALL(puts, 1, "Enter the flag: ");
    CALL(scanf, 2, "%99s", flag);
#else
    // don't scopfuscate these strings, as they are only for dbg build
    strncpy(flag, "tstlss{aaaaaabbbbbbccccccdddddd}", 32);
    strncpy(flag, "tstlss{$C0PfU5c4ToR_1z_so_c00l!}", 32);
#endif

    CALL(strlen, 1, flag);
    flaglen = ret;
    ISNEQ_EXIT(ret, 32);


    ISNEQ_EXIT(flag[0], 't');
    ISNEQ_EXIT(flag[1], 's');
    ISNEQ_EXIT(flag[2], 't');
    ISNEQ_EXIT(flag[3], 'l');
    ISNEQ_EXIT(flag[4], 's');
    ISNEQ_EXIT(flag[5], 's');
    ISNEQ_EXIT(flag[6], '{');
    ISNEQ_EXIT(flag[ret -1 ], '}');

    // Drop prefix and suffix from the flag (do it in-place).
    //memmove(flag, &flag[7], flaglen);
    CALL(memmove, 3, flag, &flag[7], flaglen);
    flag[flaglen] = '\0';



    // If at least 1 group returns non-NULL, flag is wrong.
    uint32_t crc = not(0U);
    uint32_t i, j, k;
    int p1, p2, p3;

    uint32_t shr_ret, xor_ret, and_ret;

    WHILE_INC_START(0, i);
        // Do a CRC32 100 times in a loop, without reseting crc value.
        CALL(get_pair_1, 1, i);
        p1 = ret;

        CALL(get_pair_2, 1, i);
        p2 = ret;

        CALL(get_pair_3, 1, i);
        p3 = ret;
        char test[4] = {flag[p1], flag[p2], flag[p3], 0};


        WHILE_INC_START(0, j);
            WHILE_INC_START(0, k);

                CALL(shrs, 2, crc, 8);
                shr_ret = ret;

                CALL(xors, 2, crc, test[k]);
                xor_ret = ret;

                CALL(ands, 2, xor_ret, 0xff);
                and_ret = ret;

                CALL(xors, 2, crc32_tab[and_ret], shr_ret);
                crc = ret;

            WHILE_INC_STOP(4, k);
        WHILE_INC_STOP(100, j);


        CALL(nots, 1, 0U );
        CALL(xors, 2, crc, ret );
        crc = ret;

        CALL(get_target_value_1, 1, i);
        result |= crc != ret;
    WHILE_INC_STOP(3, i);

    combined_result = result;


    uint32_t tmp;

    uint32_t state[6];
    result = 0;


    CALL(shls, 2, crc32_tab[0x67], 8);
    CALL(ors , 2, ret, flag[6]);
    CALL(rors, 2, ret, 11);
    state[0] = ret;


    CALL(shrs, 2, crc32_tab[0x11], 8);
    tmp = ret;
    CALL(shls, 2, flag[7], 24);
    CALL(ors , 2, tmp, ret);
    CALL(rols, 2, ret, 23);
    state[1] = ret;

    CALL(ands, 2, crc32_tab[0xEC], 0xffff00ff);
    tmp = ret;
    CALL(shls, 2, flag[8], 8);
    CALL(ors , 2, tmp, ret);
    CALL(rors, 2, ret, 7);
    state[2] = ret;


    CALL(ands, 2, crc32_tab[0x94], 0xff00ffff);
    tmp = ret;
    CALL(shls, 2, flag[9], 16);
    CALL(ors , 2, tmp, ret);
    CALL(rols, 2, ret, 19);
    state[3] = ret;


    CALL(nots, 1, flag[10]);
    CALL(ands, 2, ret, 0xff);
    CALL(rors, 2, crc32_tab[ret], 5);
    state[4] = ret;

    CALL(nots, 1, crc32_tab[flag[11]]);
    CALL(rols, 2, ret, 27);
    state[5] = ret;




    WHILE_INC_START(0, i);
        tmp = state[0];
        WHILE_INC_START(0, j);
            CALL(rols, 2, state[j+1], 11);
            state[j] = ret;
        WHILE_INC_STOP(5, j);

        CALL(rols, 2, state[0], 11);
        state[5] = ret;

        WHILE_INC_START(0, j);
            CALL(shls, 2, i, 3);
            CALL(xors, 2, state[j], crc32_tab[ret]);
            state[j] = ret;
        WHILE_INC_STOP(6, j);
    WHILE_INC_STOP(10, i);

    WHILE_INC_START(0, i);
        CALL(get_target_value_2, 1, i);
        result |= state[i] != ret;
    WHILE_INC_STOP(6, i);

    combined_result |= result;


    CALL(memmove, 3, flag, &flag[12], 12);
    uint32_t shl1_res, shl2_res, add_res, mul_res, mod_res, or_res;
    uint32_t f1, f2, f3;
    result = 0;
    int cipher[4];

    WHILE_INC_START(0, i);
        CALL(muls, 2, 3, i);
        mul_res = ret;

        CALL(adds, 2, mul_res, 2);
        f3 = ret;

        CALL(muls, 2, 3, i);
        mul_res = ret;

        CALL(adds, 2, mul_res, 1);
        f2 = ret;

        CALL(muls, 2, 3, i);
        mul_res = ret;

        CALL(adds, 2, mul_res, 0);
        f1 = ret;

        CALL(shls, 2, flag[f2], 8);
        shl1_res = ret;

        CALL(shls, 2, flag[f1], 16);
        shl2_res = ret;

        CALL(ors, 2, shl2_res, shl1_res);
        or_res = ret;

        CALL(ors, 2, or_res, flag[f3]);
        or_res = ret;

        CALL(muls, 2, or_res, 0x5be794);
        mul_res = ret;

        CALL(shls, 2, 2, 31);
        shl1_res = ret;

        CALL(subs, 2, shl1_res, 1);
        f1 = ret;

        CALL(mods, 2, mul_res, f1);
        f1 = ret;
        cipher[i] = f1;
    WHILE_INC_STOP(4, i);

    WHILE_INC_START(0, i);
        CALL(get_target_value_3, 1, i);
        result |= cipher[i] != ret;
    WHILE_INC_STOP(4, i);

    combined_result |= result;

    ISNEQ_EXIT(combined_result, 0);

    CALL(puts, 1, "Correct!");

    return 0;
}

// ------------------------------------------------------------------------------------------------

