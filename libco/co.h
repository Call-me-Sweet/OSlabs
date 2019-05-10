#ifndef __CO_H__
#define __CO_H__
#define MAX_CO 20

typedef void (*func_t)(void *arg);
struct co;
enum State{FREE,RUNNING,SUSPEND,ABLE};
void co_init();
struct co* co_start(const char *name, func_t func, void *arg);
void co_yield();
void co_wait(struct co *thd);
//void non(){return;}

#endif
