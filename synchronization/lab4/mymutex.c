#define _GNU_SOURCE
#include "mymutex.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/futex.h>
#include <syscall.h>



long futex(atomic_flag *uaddr, const int futex_op, const int val) {
  return syscall(SYS_futex, uaddr, futex_op, val, NULL, NULL, 0);
}

void mymutex_init(mymutex *s) {
  atomic_flag_clear(&s->lock);
}

void mymutex_lock(mymutex *s) {
  while (atomic_flag_test_and_set(&s->lock)) {
    if (futex(&s->lock, FUTEX_WAIT, 0) == -1 && errno != EAGAIN) {
      printf("futex FUTEX_WAIT failed: %s\n", strerror(errno));
      abort();
    }
  }
}

void mymutex_unlock(mymutex *s) {
  atomic_flag_clear(&s->lock);

  if (futex(&s->lock, FUTEX_WAKE, 1) == -1) {
    printf("futex FUTEX_WAKE failed: %s\n", strerror(errno));
    abort();
  }
}