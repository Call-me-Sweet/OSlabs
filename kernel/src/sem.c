#include <common.h>
#include <klib.h>
#include <devices.h>

void seminit(sem_t *sem, const char *name, int value){
        my_spinlock_init(&sem->lock, name);
        sem->name = name;
        sem->value = value;
        sem->active = 1;
        sem->wlleft = sem->wlright = 0;
        //sem->wlcnt = 0;
        for(int i = 0; i < NTHREAD; ++i){
            sem->wl[i] = NULL;
        }
}

/*
void semwait(sem_t *sem){
    assert(sem->active == 1);
    my_spinlock(&sem->lock);

    sem->value--;
    if(sem->value < 0){
        my_spinunlock(&sem->lock);
        asm volatile ("int $0x80" : : "a"(_SEM_WAIT), "b"(sem));
        my_spinlock(&sem->lock);
    }
    
    my_spinunlock(&sem->lock);
}

/
void semsignal(sem_t *sem){
    assert(sem->active == 1);
    my_spinlock(&sem->lock);

    sem->value++;
   
    my_spinunlock(&sem->lock);
    asm volatile ("int $0x80" : : "a"(_SEM_SIGN), "b"(sem));
    my_spinlock(&sem->lock);

    my_spinunlock(&sem->lock);
}
*/

void semwait(sem_t *sem){
    assert(sem->active == 1);
    my_spinlock(&sem->lock);

#ifdef DEBUG
    my_spinlock(&plock);
    printf("wait!\n");
    my_spinunlock(&plock);
#endif
    
    while(sem->value <= 0){
        //To be suspend
        if(sem->wl[sem->wlright] != NULL)//should never right
             panic("The waiting queue is full!");
        
        sem->wl[sem->wlright] = cur_task;
        sem->wlright = (sem->wlright + 1) % NTHREAD; 
       
        cur_task->status = _SUSPEND;

        my_spinunlock(&sem->lock);
       // asm volatile ("int 0x80":: "a"(-1));    
        _yield();
        
        my_spinlock(&sem->lock);
    }

    sem->value--;
    my_spinunlock(&sem->lock);
}

void semsignal(sem_t *sem){
    assert(sem->active == 1);
    my_spinlock(&sem->lock);

    sem->value++;
#ifdef DEBUG
    my_spinlock(&plock);
    printf("signal!\n");
    my_spinunlock(&plock);
#endif
    if(sem->wl[sem->wlleft] != NULL){//there exits waiting task
       sem->wl[sem->wlleft]->status = _READY;
       sem->wl[sem->wlleft] = NULL;
       sem->wlleft = (sem->wlleft + 1) % NTHREAD;

    }
    else if(sem->wlleft != sem->wlright)
        panic("The waiting list should be empty but not");

    my_spinunlock(&sem->lock);
}



