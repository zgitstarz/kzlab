#ifndef KZLAB_H
#define KZLAB_H

#define MODULE_NAME "kzlab"
#define BUF_SIZE 200

struct kzlab_manager{
    struct semaphore kzsem;
    struct cdev kzc;
    dev_t kzdev;
    char* kzbuffer;
};



#endif