#ifndef MYSPINLOCK_H
#define MYSPINLOCK_H

#include <stdatomic.h>

typedef struct {
  atomic_flag lock;
} myspinlock;

void myspinlock_init(myspinlock *s);

void myspinlock_lock(myspinlock *s);

void myspinlock_unlock(myspinlock *s);

#endif //MYSPINLOCK_H
