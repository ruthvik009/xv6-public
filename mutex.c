// mutex.c
#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

#define MAX_MUTEXES 64

struct mutex {
  int allocated;
  int locked;
  struct spinlock lock;
};

struct mutex mutexes[MAX_MUTEXES];

void
mutex_init(void)
{
  int i;
  for(i = 0; i < MAX_MUTEXES; i++){
    mutexes[i].allocated = 0;
    mutexes[i].locked = 0;
    initlock(&mutexes[i].lock, "mutex");
  }
}

int
sys_getmutex(void)
{
  int i;
  acquire(&tickslock); // Use a global lock to protect mutex allocation
  for(i = 0; i < MAX_MUTEXES; i++){
    if(mutexes[i].allocated == 0){
      mutexes[i].allocated = 1;
      release(&tickslock);
      return i;
    }
  }
  release(&tickslock);
  return -1; // No available mutex
}

int
sys_lock(void)
{
  int mutexid;
  if(argint(0, &mutexid) < 0)
    return -1;
  if(mutexid < 0 || mutexid >= MAX_MUTEXES)
    return -1;

  struct mutex *m = &mutexes[mutexid];

  acquire(&m->lock);
  while(m->locked){
    sleep(m, &m->lock);
  }
  m->locked = 1;
  release(&m->lock);
  return 0;
}

int
sys_unlock(void)
{
  int mutexid;
  if(argint(0, &mutexid) < 0)
    return -1;
  if(mutexid < 0 || mutexid >= MAX_MUTEXES)
    return -1;

  struct mutex *m = &mutexes[mutexid];

  acquire(&m->lock);
  m->locked = 0;
  wakeup(m);
  release(&m->lock);
  return 0;
}