#ifndef __YAKNOTE_H
#define __YAKNOTE_H

#include <linux/ioctl.h>

typedef struct {
    size_t slot;
    void *dest;
    size_t size;
} read_arg_t;

#define READ_NOTE _IOR('r', 1, read_arg_t *)

typedef struct {
    const void *src;
    size_t size;
    long int id;
} add_arg_t;

#define ADD_NOTE _IOW('a', 1, add_arg_t *)
#define N_NOTES 8

typedef struct {
    size_t slot;
    const void *src;
    size_t size;
} edit_arg_t;

#define EDIT_NOTE _IOW('e', 1, edit_arg_t *)

#define N_NOTES 8
#endif // __YAKNOTE_H