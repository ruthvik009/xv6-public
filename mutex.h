// mutex.h
#ifndef MUTEX_H
#define MUTEX_H

void mutex_init(void);
int sys_getmutex(void);
int sys_lock(void);
int sys_unlock(void);

#endif 