#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include "yaknote.h"

#define DEVICE_NAME "pwn"
#define DEVICE_FILE_NAME "pwn"
#define MAJOR_NUM 69

MODULE_LICENSE("WTFPL");
MODULE_AUTHOR("tasteless");
MODULE_DESCRIPTION("Yet another kernel Notepad");
MODULE_VERSION("13.37");

static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static long device_ioctl(struct file *f, unsigned int cmd, unsigned long arg);

struct note {
    char *data;
    long int hash;
    size_t size;
};

struct notebook {
	struct kref refcount;
    ssize_t n_notes;
    struct note notes[N_NOTES];
};

static struct file_operations file_ops = {
    .open = device_open,
    .release = device_release,
    .unlocked_ioctl = device_ioctl,
};

static void notebook_release(struct kref *ref) {
    int i;
	struct notebook *notebook = container_of(ref, struct notebook, refcount);

    for (i = 0; i <= notebook->n_notes; i++) {
        kfree(notebook->notes[i].data);
    }

    kfree(notebook);
}

static long ioctl_note_read(struct notebook *notebook, unsigned long arg) {
    read_arg_t req;
    int ret = 0;

    kref_get(&notebook->refcount);

    if (copy_from_user(&req, (void*) arg, sizeof(req))) {
        ret = -EFAULT;
        goto out;
    };

    if ((req.slot > notebook->n_notes) || (req.size > notebook->notes[req.slot].size)) {
        ret = -EINVAL;
        goto out;
    }

    if (copy_to_user(req.dest, notebook->notes[req.slot].data, notebook->notes[req.slot].size)) {
        ret = -EFAULT;
        goto out;
    };

 out:
    kref_put(&notebook->refcount, notebook_release);
    return ret;
}

static long ioctl_note_add(struct notebook *notebook, unsigned long arg) {
    add_arg_t req;
    int ret = 0;
    size_t old_n_notes, slot;

    kref_get(&notebook->refcount);

    old_n_notes =  notebook->n_notes;
    slot = ++notebook->n_notes;

    if (copy_from_user(&req, (void*) arg, sizeof(req))) {
        ret = -EFAULT;
        goto error;
    };

    if (slot >= N_NOTES) {
        ret = -ENOMEM;
        goto error;
    }

    notebook->notes[notebook->n_notes].size = req.size;
    notebook->notes[notebook->n_notes].data = kmalloc(req.size, GFP_USER);

    if (copy_from_user(notebook->notes[notebook->n_notes].data, req.src, req.size)) {
        kfree(notebook->notes[notebook->n_notes].data);
        ret = -EFAULT;
        goto error;
    };

    goto cleanup;

 error:
    notebook->n_notes = old_n_notes;
 cleanup:
    kref_put(&notebook->refcount, notebook_release);
    return ret;
}

static long ioctl_note_edit(struct notebook *notebook, unsigned long arg) {
    edit_arg_t req;
    int ret = 0;

    if (copy_from_user(&req, (void*) arg, sizeof(req))) {
        return -EFAULT;
    };

    kref_get(&notebook->refcount);
    if ((req.slot > notebook->n_notes) || (req.size > notebook->notes[req.slot].size)) {
        ret = -EINVAL;
        goto out;
    }

    if (copy_from_user(notebook->notes[req.slot].data, req.src, req.size)) {
        ret = -EFAULT;
    };

 out:
    kref_put(&notebook->refcount, notebook_release);
    return ret;
}

static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case READ_NOTE:
            return ioctl_note_read(file->private_data, arg);

        case ADD_NOTE:
            return ioctl_note_add(file->private_data, arg);

        case EDIT_NOTE:
            return ioctl_note_edit(file->private_data, arg);

        default:
            return -EINVAL;
    }
}

static int device_open(struct inode *inode, struct file *file) {
    struct notebook* notebook;

    try_module_get(THIS_MODULE);

    notebook = kzalloc(sizeof(struct notebook), GFP_USER);
    kref_init(&notebook->refcount);

    notebook->n_notes = -1;
    file->private_data = notebook;
    return 0;
}

static int device_release(struct inode *inode, struct file *file) {
    struct notebook* notebook = file->private_data;
    kref_put(&notebook->refcount, notebook_release);
    module_put(THIS_MODULE);
    return 0;
}

static int __init devtmp_init(void) {
    int ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &file_ops);
    if (ret < 0)
        return ret;

    return 0;
}

static void __exit devtmp_exit(void) {
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

module_init(devtmp_init);
module_exit(devtmp_exit);