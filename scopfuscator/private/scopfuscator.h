#ifndef __SCOPEFUSECATOR_H
#define __SCOPEFUSECATOR_H

#include <signal.h>
#include <ucontext.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#define WRONG_FLAG        \
    printf("Wrong :(\n"); \
    exit(0);


/*  Unfortunately, I didn't find a way to make makecontext accept other types of
 *  fns beyond void (*__func) (void) without emitting warnings :( */
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

/*#define INC_CUR_CTX if (++cur_ctx_idx == N_CONTEXTS) {EXIT}*/
/*#define DEC_CUR_CTX if (--cur_ctx_idx == -1) {EXIT}*/
#define INC_CUR_CTX ++cur_ctx_idx;
#define DEC_CUR_CTX --cur_ctx_idx;

char stacks[1000][4096];
int ret;

ucontext_t context_stack[1000] = {0};
unsigned int cur_ctx_idx = 1;

/*  small helper functions ... in a header file, *sigh* */
void isneq(int a, int b){
    if (a != b) {WRONG_FLAG};
}

void loop_switcher(int a, int b) {
    if (a < b) { setcontext(&context_stack[cur_ctx_idx]); }
}

#define _SCFUSE_STACKSIZE 4096

#define CALL(fn, n_args, ...) \
    INC_CUR_CTX; \
    INC_CUR_CTX; \
    getcontext(&context_stack[cur_ctx_idx]); \
    context_stack[cur_ctx_idx].uc_stack.ss_sp = &stacks[cur_ctx_idx]; \
    context_stack[cur_ctx_idx].uc_stack.ss_size = _SCFUSE_STACKSIZE; \
    context_stack[cur_ctx_idx].uc_link = &context_stack[cur_ctx_idx-1]; \
    makecontext(&context_stack[cur_ctx_idx], fn, n_args, __VA_ARGS__); \
    swapcontext(&context_stack[cur_ctx_idx-1], &context_stack[cur_ctx_idx]); \
    DEC_CUR_CTX; \
    DEC_CUR_CTX; \



/*context_stack[cur_ctx_idx].uc_stack.ss_sp = mmap((void *) 0, _SCFUSE_STACKSIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN, -1, 0); \*/


#define CALL_0(fn) \
    INC_CUR_CTX; \
    getcontext(&context_stack[cur_ctx_idx]); \
    context_stack[cur_ctx_idx].uc_stack.ss_sp = mmap((void *) 0, _SCFUSE_STACKSIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN, -1, 0); \
    context_stack[cur_ctx_idx].uc_stack.ss_size = _SCFUSE_STACKSIZE; \
    makecontext(&context_stack[cur_ctx_idx], fn, 0); \
    swapcontext(&context_stack[cur_ctx_idx-1], &context_stack[cur_ctx_idx]); \

#define RET(retval) \
    ret = retval; \
    context_stack[cur_ctx_idx-1].uc_mcontext.gregs[REG_RAX] = retval; \
    setcontext(&context_stack[cur_ctx_idx-1]);

#define GC \
    int gci = cur_ctx_idx+1; \
    WHILE_INC_START(gci); \
        munmap(context_stack[gci].uc_stack.ss_sp, _SCFUSE_STACKSIZE); \
        if ( context_stack[gci].uc_stack.ss_sp != NULL) { \
    WHILE_INC_STOP(1000); \
    }; \


#define ISNEQ_EXIT(a, b) \
    CALL(isneq, 2, a, b)


#define WHILE_INC_START(n, var) \
    var = n; \
    getcontext(&context_stack[++cur_ctx_idx]); \

#define WHILE_INC_STOP(max, var) \
    var++;\
    loop_switcher(var, max); \
    DEC_CUR_CTX

#endif
