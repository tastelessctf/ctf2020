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


// Uncomment for debug version.
#define __DEBUG__

#define WRONG_FLAG        \
    printf("Wrong :(\n"); \
    exit(0)


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


// ------------------------------------------------------------------------------------------------
//  Verify group #1 
//
//  How to break it: There are 3 rounds. Each round takes 3 characters from the flag and
//  calculates 100 the (custom) CRC32. Brute force.
//
int get_pair_1(int idx) {
    size_t iterations[] = {1, 2, 4};
    return iterations[idx];
}

int get_pair_2(int idx) {
    size_t iterations[] = {2, 0, 3};
    return iterations[idx];
}

int get_pair_3(int idx) {
    size_t iterations[] = {3, 5, 0};
    return iterations[idx];
}

int get_target_value_1(int idx) {
    uint32_t target_values[] = {0x7BA3E0DC, 0xE7F42D9A, 0xD9A4F450};
    return target_values[idx];
}

int verify_group_1(char *flag) {   
    uint32_t crc = not(0U);
    uint32_t i, j, k;
    int result = 0;

    for (i=0; i<3; ++i) {
        // Do a CRC32 100 times in a loop, without reseting crc value.
        char test[4] = {flag[get_pair_1(i)], flag[get_pair_2(i)], flag[get_pair_3(i)], 0};

        for (j=0; j<100; j=add(j, 1)) {
            for (k=0; k<4; k=add(k, 1)) {
                crc = xor(crc32_tab[and(xor(crc, test[k]), 0xFF)], shr(crc, 8));                
            }
        }

        crc = xor(crc, not(0U));
        result |= crc != get_target_value_1(i);
    }

    return result;
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
    return target_values[idx];
}

int verify_group_2(char *flag) {   
    uint32_t i, j;
    int result = 0;

    uint32_t state[6] = {
        ror(or(shl(crc32_tab[0x67], 8), flag[0]), 11),
        rol(or(shr(crc32_tab[0x11], 8), shl(flag[1], 24)), 23),
        ror(or(and(crc32_tab[0xEC], 0xFFFF00FF), shl(flag[2], 8)), 7),
        rol(or(and(crc32_tab[0x94], 0xFF00FFFF), shl(flag[3], 16)), 19),
        ror(crc32_tab[and(not(flag[4]), 0xFF)], 5),
        rol(not(crc32_tab[flag[5]]), 27)
    };
   
    for (i=0; i<10; ++i) {
        size_t tmp = state[0];
        for (j=0; j<5; ++j) {
            state[j] = rol(state[j+1], 11);
        }

        state[5] = rol(state[0], 11);
               
        for (j=0; j<6; ++j) {
            state[j] ^= crc32_tab[shl(i, 3)];
        }
    }

    for (i=0; i<6; ++i) {
        result |= state[i] != get_target_value_2(i);
    }

    return result;
}


// ------------------------------------------------------------------------------------------------
//  Verify group #3
//
//  How to break it: Modulo inverse from 3 characters.
//
int get_target_value_3(int idx) {
    uint32_t target_values[] = {0xB30BB088, 0x60CCE52C, 0x8BB5A7C0, 0x3D4B4A14};
    return target_values[idx];
}

int verify_group_3(char *flag) {   
    uint32_t i;
    int result = 0;
    int cipher[4];

    for (i=0; i<4; ++i) {
        cipher[i] = mod(mul(or(or(shl(flag[add(mul(3, i), 0)], 16),
                                  shl(flag[add(mul(3, i), 1)], 8)),
                                      flag[add(mul(3, i), 2)]), 0x5BE794),
                        sub(shl(2, 31), 1));
    }

    for (i=0; i<4; ++i) {
        result |= cipher[i] != get_target_value_3(i);
    }

    return result;
}


// ------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
    char flag[128] = {0};
    size_t flaglen = 0;
    int result = 0;

#ifndef __DEBUG__
    printf("Enter the flag: ");
    scanf("%99s", flag);
#else
    strncpy(flag, "tstlss{aaaaaabbbbbbccccccdddddd}", 32);
    strncpy(flag, "tstlss{$C0PfU5c4ToR_1z_so_c00l!}", 32);
#endif            
    flaglen = strnlen(flag, 99);

    /* Base check: Check flag's prefix/suffix & length */
    // TODO(ispo): Make base check less obvious.
    if (flaglen != 32 || strncmp(flag, "tstlss{", 7) || flag[flaglen - 1] != '}') {
        WRONG_FLAG;
    }

    // Drop prefix and suffix from the flag (do it in-place).
    memmove(flag, &flag[7], flaglen);
    flag[flaglen] = '\0';

    // If at least 1 group returns non-NULL, flag is wrong.
    result  = verify_group_1(flag);
    result |= verify_group_2(&flag[6]);
    result |= verify_group_3(&flag[12]);

    if (result) { 
        WRONG_FLAG;
    }

    printf("Correct!\n");

    return 0;
}

// ------------------------------------------------------------------------------------------------

