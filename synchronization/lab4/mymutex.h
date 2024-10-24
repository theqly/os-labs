#ifndef MYMUTEX_H
#define MYMUTEX_H

#include <stdatomic.h>

typedef struct {
  atomic_flag lock;
} mymutex;

long futex(atomic_flag *uaddr, const int futex_op, const int val);

void mymutex_init(mymutex *s);

void mymutex_lock(mymutex *s);

void mymutex_unlock(mymutex *s);

#endif //MYMUTEX_H
