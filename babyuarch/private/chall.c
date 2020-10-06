#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>




/*  config - these values are partially hardcoded in the op emitters :/ */

#define N_OBJECTS 32
#define MMAP_MIN_ADDR (void *) 0x10000
#define REGISTER_STORE MMAP_MIN_ADDR + 0x100
#define R0_OFF 0x00
#define R1_OFF 0x08
#define R2_OFF 0x10
#define L1_OFF 0x18
#define L2_OFF 0x20
#define SUB_OFF 0x28

#define R0_REG "RAX"
#define R1_REG "RBX"
#define R2_REG "RCX"

#define L1_REG "R12"
#define L2_REG "R13"
#define SUB_REG "R14"




#define BLINDSIDE_OBJS MMAP_MIN_ADDR + 0x400

#define UNLOCKED_ADDR MMAP_MIN_ADDR

#define CODE_LEN 512


typedef struct blindside_obj_t {
  int fp_enabled;
  char fill2[64 - 0x8]; // cacheline alignement
  void (*fp)();
} blindside_obj_t;



/*  VM helper functions: DON'T CHANGE THEIR ORDER! */

__attribute__((naked))
void helper_bin_sh() {
    char *unlocked = UNLOCKED_ADDR;
    if (*unlocked == 1)
    asm(
            ".intel_syntax noprefix;"
            "mov rax, [0x10000];" // needs update?
            "cmp rax, 0x00;"
            "je return;"
            "xor eax, eax;"
            "mov rbx, 0xFF978CD091969DD1;"
            "neg rbx;"
            "push rbx;"
            "mov rdi, rsp;"
            "cdq;"
            "push rdx;"
            "push rdi;"
            "mov rsi, rsp;"
            "mov al, 0x3b;"
            "syscall;"
            "return: ;"
            ".att_syntax;"
       );
    asm("ret");
}

__attribute__((naked))
void helper_nop(){
    asm("ret");
}

#define FORCE_INLINE __attribute__ ((always_inline)) static inline

FORCE_INLINE void itoa_write(char *in, char * out) {
    char high, low;
    low  = (*in & 0x0f)      + '0';
    high = ((*in & 0xf0) >> 4) + '0';
    if (high > '9') high += 0x27;
    if (low  > '9') low  += 0x27;
    out[0] = high;
    out[1] = low;

}


void helper_print_vm_state(){
    char high, low;

    char output[80] = ("r0: __\n"
                       "r1: __\n"
                       "r2: __\n"
                       "l0: __\n"
                       "l1: __\n"
                       "unlocked: __\n"
                       "last_sub: ____\n\n");
    char *acc = REGISTER_STORE;
    char *r1 = REGISTER_STORE+R1_OFF;
    char *r2 = REGISTER_STORE+R2_OFF;
    char *l1 = REGISTER_STORE+L1_OFF;
    char *l2 = REGISTER_STORE+L2_OFF;
    char *last_sub = REGISTER_STORE+SUB_OFF;




    char *unlocked = UNLOCKED_ADDR;

    itoa_write(REGISTER_STORE+R0_OFF, &output[4]);
    itoa_write(r1, &output[11]);
    itoa_write(r2, &output[18]);
    itoa_write(l1, &output[25]);
    itoa_write(l2, &output[32]);
    itoa_write(unlocked, &output[45]);
    itoa_write(last_sub+1, &output[58]);
    itoa_write(last_sub, &output[60]);

    asm(
            ".intel_syntax noprefix;"
            "mov rdx, 80;"
            "mov rsi, %0;"
            "mov rdi, 1;"
            "mov rax, rdi;"
            "syscall;"
            ".att_syntax;"
        :: "r"(output)
       );

}

__attribute__((naked))
void helper_unlock_vm(){

    register char * acc asm ("rdi") = REGISTER_STORE;
    register char * reg asm ("rsi") = REGISTER_STORE+R1_OFF;
    //register int acc asm ("al");
    int *unlocked = UNLOCKED_ADDR;
    if (*acc == 0x13 && *reg == 0x37) *unlocked = 1;

    asm("ret");
}


__attribute__((naked))
void helper_exit(){
    asm(
            ".intel_syntax noprefix;"
            "mov rdi, rbx;"
            "mov rax, 0x3c;"
            "syscall;"
            ".att_syntax;"
       );
}

/*  Will not be called, just for getting memcpy-addrs */
__attribute__((naked))
void helper_end(){
    asm("ret");
}




typedef void (*vm_helper_t)();
vm_helper_t vm_helpers[] = {
    helper_nop,
    helper_print_vm_state,
    helper_unlock_vm,
    helper_exit,
    helper_end,
};

/*  VM code */
#define EMIT_BYTE(b) emit_loc[i++] = (b);
#define OP_SKELETON(x) do { \
    int i = 0; \
    if (flag) i = 6; \
    x \
    if (flag) { \
        /*  cmp r14, 0x00 */ \
        emit_loc[0] = 0x49;  \
        emit_loc[1] = 0x83;  \
        emit_loc[2] = 0xfe;  \
        emit_loc[3] = 0x00;  \
        /*  jg afterins  */  \
        emit_loc[4] = 0x7f;  \
        emit_loc[5] = i - 6; \
        flag = 0; \
    } \
    return i; \
    } while(0);


char * loop_start_loc = 0;
char * big_loop_start_loc = 0;
uint8_t flag = 0;



/*  complex insns not using the skeleton */
int op_prefix_conditional(char * operands, char *emit_loc){
    /*  only if flag is set to one, the conditional check insn are emitted */
    flag=1;
    return 0;
}

int not_implemented(char * operands, char * emit_loc) {
    OP_SKELETON(
            EMIT_BYTE(0xcc)
    )
}


int op_loop_start(char * operands, char *emit_loc){
    OP_SKELETON (
        loop_start_loc = emit_loc + 3;
        /*  mov r12, rax */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0x89)
        EMIT_BYTE(0xc4)
    )
}

int op_big_loop_start(char * operands, char *emit_loc){
    OP_SKELETON (
        big_loop_start_loc = emit_loc + 7;
        /*  mov r12, rax */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0x89)
        EMIT_BYTE(0xc5)
        /*  shl r12, 12 */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0xc1)
        EMIT_BYTE(0xe5)
        EMIT_BYTE(0x0c)
    )
}

int op_loop_end(char * operands, char *emit_loc){
    if (loop_start_loc == 0) return 0;
    int off =  loop_start_loc - emit_loc - 0xa-3;
    loop_start_loc = 0;
    OP_SKELETON (
        /*  dec r12 */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0xff)
        EMIT_BYTE(0xcc)
        /*  cmp r12, 00 */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0x83)
        EMIT_BYTE(0xfc)
        EMIT_BYTE(0x00)
        /*  jz  loop_start_loc*/
        EMIT_BYTE(0x0f)
        EMIT_BYTE(0x85)
        EMIT_BYTE( ( off      ) & 0xff )
        EMIT_BYTE( ( off >> 8 ) & 0xff )
        EMIT_BYTE( ( off >> 16) & 0xff )
        EMIT_BYTE( ( off >> 24) & 0xff )
    )
}

int op_big_loop_end(char * operands, char *emit_loc) {
    if (big_loop_start_loc == 0) return 0;
    int off =  big_loop_start_loc - emit_loc - 0xc;
    big_loop_start_loc = 0;
    OP_SKELETON (
        /*  dec r13 */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0xff)
        EMIT_BYTE(0xcd)
        /*  cmp r13, 00 */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0x83)
        EMIT_BYTE(0xfd)
        EMIT_BYTE(0x00)
        /*  jz  loop_start_loc*/
        EMIT_BYTE(0x0f)
        EMIT_BYTE(0x85)
        EMIT_BYTE( ( off      ) & 0xff )
        EMIT_BYTE( ( off >> 8 ) & 0xff )
        EMIT_BYTE( ( off >> 16) & 0xff )
        EMIT_BYTE( ( off >> 24) & 0xff )
    )
}


int op_rdtsc(char * operands, char *emit_loc){
    OP_SKELETON(
        /*  rdtscp */
        EMIT_BYTE(0x0f)
        EMIT_BYTE(0x01)
        EMIT_BYTE(0xf9)
    )
}

int op_mfence(char * operands, char *emit_loc){
    OP_SKELETON(
        /*  mfence */
        EMIT_BYTE(0x0f)
        EMIT_BYTE(0xae)
        EMIT_BYTE(0xf0)
    )
}


int op_clr(char * operands, char *emit_loc){
    OP_SKELETON(
        /*  mov xor rax, rax */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xc0)
        /*  xor rbx, rbx  */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xdb)
        /*  xor rcx, rcx  */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xc9)
        /*  xor r12, r12  */
        EMIT_BYTE(0x4d)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xe4)
        /*  xor r13, r13  */
        EMIT_BYTE(0x4d)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xed)
        /*  xor r14, r14  */
        EMIT_BYTE(0x4d)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xf6)
    )
}


FORCE_INLINE void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }
FORCE_INLINE void mfence() { asm volatile("mfence"); }



#define EMIT_GET_BLINDSIDE_OBJ \
    /* movsxd rdi, eax */ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0x63); \
    EMIT_BYTE(0xf8); \
    /* mov rsi, rdi*/ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0x89); \
    EMIT_BYTE(0xfe); \
    /* shl rsi, 0x03*/ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0xc1); \
    EMIT_BYTE(0xe6); \
    EMIT_BYTE(0x03); \
    /* add rsi, rdi*/ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0x01); \
    EMIT_BYTE(0xfe); \
    /* shl rsi, 0x03*/ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0xc1); \
    EMIT_BYTE(0xe6); \
    EMIT_BYTE(0x03); \
    /* lea    rdx,[rsi+0x10400] */ \
    EMIT_BYTE(0x48); \
    EMIT_BYTE(0x8d); \
    EMIT_BYTE(0x96); \
    EMIT_BYTE(0x00); \
    EMIT_BYTE(0x04); \
    EMIT_BYTE(0x01); \
    EMIT_BYTE(0x00);

#define BLINDSIDE_ACC_CHECK \
    /*  cmp ax, n_objects */ \
    EMIT_BYTE(0x3c) \
    EMIT_BYTE(N_OBJECTS) \
    /*  jl+1 */ \
    EMIT_BYTE(0x7c) \
    EMIT_BYTE(0x01)\
    /*  int3*/ \
    EMIT_BYTE(0xcc)

#define STORE_REGS \
    /*  mov rdi, REGSTORE */ \
    EMIT_BYTE(0x48) \
    EMIT_BYTE(0xc7) \
    EMIT_BYTE(0xc7) \
    EMIT_BYTE(0x00) \
    EMIT_BYTE(0x01) \
    EMIT_BYTE(0x01) \
    EMIT_BYTE(0x00) \
    /*  mov [rdi], al */ \
    EMIT_BYTE(0x88) \
    EMIT_BYTE(0x07) \
    /*  mov [rdi+4], bl */ \
    EMIT_BYTE(0x88) \
    EMIT_BYTE(0x5f) \
    EMIT_BYTE(R1_OFF) \
    /*  mov [rdi+off], rcx */ \
    EMIT_BYTE(0x48) \
    EMIT_BYTE(0x89) \
    EMIT_BYTE(0x4f) \
    EMIT_BYTE(R2_OFF) \
    /*  [rdi+OFF], r12,  */ \
    EMIT_BYTE(0x44) \
    EMIT_BYTE(0x89) \
    EMIT_BYTE(0x67) \
    EMIT_BYTE(L1_OFF) \
    /*  [rdi+OFF], r13,  */ \
    EMIT_BYTE(0x44) \
    EMIT_BYTE(0x89) \
    EMIT_BYTE(0x6f) \
    EMIT_BYTE(L2_OFF) \
    /*  [rdi+OFF], r14,  */ \
    EMIT_BYTE(0x44) \
    EMIT_BYTE(0x89) \
    EMIT_BYTE(0x77) \
    EMIT_BYTE(SUB_OFF) \

#define RESTORE_REGS \
    /*  mov rdi, REGSTORE */ \
    EMIT_BYTE(0x48) \
    EMIT_BYTE(0xc7) \
    EMIT_BYTE(0xc7) \
    EMIT_BYTE(0x00) \
    EMIT_BYTE(0x01) \
    EMIT_BYTE(0x01) \
    EMIT_BYTE(0x00) \
    /*  mov al, [rdi] */ \
    EMIT_BYTE(0x8a) \
    EMIT_BYTE(0x07) \
    /*  mov bl, [rdi+4] */ \
    EMIT_BYTE(0x8a) \
    EMIT_BYTE(0x5f) \
    EMIT_BYTE(R1_OFF) \
    /*  mov rcx, [rdi+OFF] */ \
    EMIT_BYTE(0x48) \
    EMIT_BYTE(0x8b) \
    EMIT_BYTE(0x4f) \
    EMIT_BYTE(R2_OFF) \
    /*  mov r12, [rdi+off]*/ \
    EMIT_BYTE(0x4c) \
    EMIT_BYTE(0x8b) \
    EMIT_BYTE(0x67) \
    EMIT_BYTE(L1_OFF) \
    /*  mov r13, [rdi+off]*/ \
    EMIT_BYTE(0x4c) \
    EMIT_BYTE(0x8b) \
    EMIT_BYTE(0x6f) \
    EMIT_BYTE(L2_OFF) \
    /*  mov r14, [rdi+off]*/ \
    EMIT_BYTE(0x4c) \
    EMIT_BYTE(0x8b) \
    EMIT_BYTE(0x77) \
    EMIT_BYTE(SUB_OFF) \


//TODO Write Emitter code
int op_do_blindside(char * operands, char *emit_loc){
  OP_SKELETON(
    STORE_REGS;
    BLINDSIDE_ACC_CHECK;
    EMIT_GET_BLINDSIDE_OBJ
    /*  clflush [rdx] */
    EMIT_BYTE(0x0f)
    EMIT_BYTE(0xae)
    EMIT_BYTE(0x3a)
    /*  mfence */
    EMIT_BYTE(0x0f)
    EMIT_BYTE(0xae)
    EMIT_BYTE(0xf0)
    /*  mov eax, [rdx] */
    EMIT_BYTE(0x8b)
    EMIT_BYTE(0x02)
    /*  test eax, eax*/
    EMIT_BYTE(0x85)
    EMIT_BYTE(0xc0)
    /*  je _end */
    EMIT_BYTE(0x74)
    EMIT_BYTE(0x0b-5)
    /*  mov rdx, [rdx+0x40] */
    EMIT_BYTE(0x48)
    EMIT_BYTE(0x8b)
    EMIT_BYTE(0x52)
    EMIT_BYTE(0x40)
    /*  call rdx */
    EMIT_BYTE(0xff)
    EMIT_BYTE(0xd2)
    RESTORE_REGS;
  )

  /* Base code emmitted
  blindside_obj_t *obj = BLINDSIDE_OBJS + acc * sizeof(blindside_obj_t);

  flush(&obj->fp_enabled);
  mfence();
  if (obj->fp_enabled) {
      obj->fp();
  }
  */
}

int op_inc_blindside_ptr(char * operands, char *emit_loc){
  OP_SKELETON(
    BLINDSIDE_ACC_CHECK;
    EMIT_GET_BLINDSIDE_OBJ
    /*  mov rdi, [rdx+0x40] */
    EMIT_BYTE(0x48)
    EMIT_BYTE(0x8b)
    EMIT_BYTE(0x7a)
    EMIT_BYTE(0x40)
    /*  add rdi, 0x1000 */
    EMIT_BYTE(0x81)
    EMIT_BYTE(0xc7)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x10)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
    /*  mov [rdx+0x40], rdi */
    EMIT_BYTE(0x48)
    EMIT_BYTE(0x89)
    EMIT_BYTE(0x7a)
    EMIT_BYTE(0x40)
  )
}


int op_enable_blindside_fp(char * operands, char *emit_loc){
  OP_SKELETON(
    BLINDSIDE_ACC_CHECK;
    EMIT_GET_BLINDSIDE_OBJ
    /*  mov [rdx], 0x1 */
    EMIT_BYTE(0xc7)
    EMIT_BYTE(0x02)
    EMIT_BYTE(0x01)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
  )
  //blindside_obj_t *obj = BLINDSIDE_OBJS + acc * sizeof(blindside_obj_t);
  //obj->fp_enabled = 1;
}

int op_disable_blindside_fp(char * operands, char *emit_loc){
  OP_SKELETON(
    BLINDSIDE_ACC_CHECK;
    EMIT_GET_BLINDSIDE_OBJ
    /*  mov [rdx], 0x1 */
    EMIT_BYTE(0xc7)
    EMIT_BYTE(0x02)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
    EMIT_BYTE(0x00)
  )
  /*
  register int acc asm ("al");
  blindside_obj_t *obj = BLINDSIDE_OBJS + acc * sizeof(blindside_obj_t);
  obj->fp_enabled = 0;
  */
}

int op_set_blindside_fp(char * operands, char *emit_loc){
  if (operands[4] || operands[3] & 0x0f) {
      printf("Invalid alignement!");
      exit(-1);
  }
  OP_SKELETON(
    BLINDSIDE_ACC_CHECK;
    EMIT_GET_BLINDSIDE_OBJ
    /*  mov [rdx+0x40], operandfoo */
    EMIT_BYTE(0x48)
    EMIT_BYTE(0xc7)
    EMIT_BYTE(0x42)
    EMIT_BYTE(0x40)
    EMIT_BYTE(operands[4])
    EMIT_BYTE(operands[3])
    EMIT_BYTE(operands[2])
    EMIT_BYTE(operands[1])
  )


    /*
  register int acc asm ("al");
  blindside_obj_t *obj = BLINDSIDE_OBJS + acc * sizeof(blindside_obj_t);
  obj->fp = 0x41424344;
  */
}

int op_access(char * operands, char *emit_loc){
    OP_SKELETON(
        /*  mov rdx, operand */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0xc7)
        EMIT_BYTE(0xc2)
        EMIT_BYTE(operands[4])
        EMIT_BYTE(operands[3])
        EMIT_BYTE(operands[2])
        EMIT_BYTE(operands[1])
        /*  mov al, [rdx] */
        EMIT_BYTE(0x8a)
        EMIT_BYTE(0x02)
        /*  mov xor rax, rax */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xc0)
    )
}


int op_sub(char * operands, char *emit_loc){
    OP_SKELETON(
        /*  sub rax, rbx */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x29)
        EMIT_BYTE(0xd8)
        if ( operands[1] != 0) {
            /*  mov r14, rax */
            EMIT_BYTE(0x49)
            EMIT_BYTE(0x89)
            EMIT_BYTE(0xc6)
        }
    )
}



int op_set_acc(char * operands, char * emit_loc) {
    OP_SKELETON(
        /*  mov xor rax, rax */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xc0)
        /*  mov al, op1 */
        EMIT_BYTE(0xb0)
        EMIT_BYTE(operands[1])
    )
}

int op_swap_reg1(char * operands, char * emit_loc) {
    OP_SKELETON(
        /*  xchg rax, rbx  */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x93)
    )
}

int op_swap_reg2(char * operands, char * emit_loc) {
    OP_SKELETON(
        /*  xchg r11, rax  */
        EMIT_BYTE(0x49)
        EMIT_BYTE(0x91)
    )
}

int op_mov_acc_loop(char * operands, char * emit_loc) {
    OP_SKELETON(
        /*  mov rax, r12 */
        EMIT_BYTE(0x4c)
        EMIT_BYTE(0x89)
        EMIT_BYTE(0xe0)
        /*  mov rbx, r13 */
        EMIT_BYTE(0x4c)
        EMIT_BYTE(0x89)
        EMIT_BYTE(0xeb)
    )
}


int op_div(char * operands, char * emit_loc) {
    OP_SKELETON(
         /*  xor edx, edx */
        EMIT_BYTE(0x31)
        EMIT_BYTE(0xd2)
         /*  div rbx */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0xf7)
        EMIT_BYTE(0xf3)
         /*  mov rbx, rdx */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0x89)
        EMIT_BYTE(0xd3)
    )
}


int op_flush(char * operands, char * emit_loc) {
    OP_SKELETON(
        /*  mov rdx, $op */
        EMIT_BYTE(0x48)
        EMIT_BYTE(0xc7)
        EMIT_BYTE(0xc2)
        EMIT_BYTE(operands[4])
        EMIT_BYTE(operands[3])
        EMIT_BYTE(operands[2])
        EMIT_BYTE(operands[1])

        /*  clflush [rdx] */
        EMIT_BYTE(0x0f)
        EMIT_BYTE(0xae)
        EMIT_BYTE(0x3a)
    )
}




/*  handler will return the amount of bytes written to outloc.
 *  The bytelength of operands is encoded with the MSBs of the opcode:
 *      0x00-0x0f: 0-byte operands
 *      0x10-0x1f: 1-byte operands
 *      0x20-0x2f: 2-byte operands
 *      etc.
 */
typedef int (*op_emitter_t)(char *, char *);
op_emitter_t op_handlers[] = {
    /*  0x00-0x0f */
    op_clr,
    op_rdtsc,
    op_swap_reg1,
    op_swap_reg2,
    op_loop_start,
    op_big_loop_start,
    op_loop_end,
    op_big_loop_end,
    op_mov_acc_loop,
    op_div,
    op_enable_blindside_fp,
    op_disable_blindside_fp,
    op_do_blindside,
    op_inc_blindside_ptr,
    op_mfence,
    //op_get_unlocked_state,
    op_prefix_conditional,
    /*  0x10-0x1f */
    op_set_acc,
    op_sub,
    op_flush,
    op_access,
    op_set_blindside_fp,
    not_implemented,
};


void parse_program(int size, char * bytecode) {
    int oplen, in_loc = 0, out_loc = 0, in_loc_inc=0;
    char * program;
    char opcode;

    program = mmap(0, 4096, PROT_WRITE|PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
#ifdef DEVBUILD
    printf("program at: %p\n", program);
#endif

    while(in_loc <= size) {

        opcode = bytecode[in_loc];

        switch (opcode) {
            case 0x00 ... 0x0f:
                in_loc_inc = 1;
                break;
            case 0x10 ... 0x11:
                in_loc_inc = 2;
                break;
            case 0x12 ... 0x14:
                in_loc_inc = 5;
                break;
            default:
                printf("Invalid opcode at addr %d!\n", in_loc);
                exit(-1);
        }


        out_loc += op_handlers[opcode](&bytecode[in_loc], &program[out_loc]);

        in_loc += in_loc_inc;
    }
    program[out_loc] = 0xc3;

    mprotect(program, 4096, PROT_READ | PROT_EXEC);

    (*(void(*)()) program)();


}



void setup_vm() {
    void *vm_data;
    int fd;
    size_t secret_addr;

    vm_data = mmap(MMAP_MIN_ADDR, 4096, PROT_WRITE|PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);

    if ( vm_data == (void *) -1 ) { perror("mmap failed: "); exit(-1); }

    for (int i = 0 ; i < (sizeof(vm_helpers) / sizeof(void *) - 1) ; i++) {
        vm_data = mmap(MMAP_MIN_ADDR + (i+1) * 0x1000 , 4096, PROT_WRITE|PROT_READ|PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
        if ( vm_data == (void *) -1 ) { perror("mmap failed: "); exit(-1); }

        memcpy(vm_data, vm_helpers[i], (vm_helpers[i+1] - vm_helpers[i]));
        mprotect(vm_data, 0x1000, PROT_EXEC | PROT_READ);
    }

    fd = open("/dev/urandom", O_RDONLY);
    if ( fd == -1 ) { perror("open failed: "); exit(1);}
    read(fd, &secret_addr, 4);
    close(fd);


#ifdef DEVBUILD
    secret_addr = 0x20000;
#endif
    vm_data = mmap( (void *) (secret_addr & 0x03fff000), 4096, PROT_WRITE|PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if ( vm_data == (void *) -1 ) { perror("mmap failed: "); exit(-1); }

    memcpy(vm_data, helper_bin_sh, (&helper_nop - helper_bin_sh));
    mprotect(vm_data, 0x1000, PROT_EXEC | PROT_READ);

#ifdef DEVBUILD
    printf("Secret page at: %p\n", vm_data);
#endif
}


int main(){
    int n_read;
    setup_vm();
    char code[CODE_LEN] = {0};
    n_read = read(0, code+1, CODE_LEN);

    parse_program(n_read, code);

}
