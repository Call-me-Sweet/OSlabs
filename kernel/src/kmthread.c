#include <common.h>
#include <devices.h>
#include <am.h>

struct task *current_tasks[MAXCPU];
//struct task *taskhead = NULL;
struct tasklist tl[MAXCPU];
static int tidcnt;

static spinlock_t tasklock = {0,"TaskCreateSwitch",NULL};
//static spinlock_t syscalllock = {0,"syscall",NULL};

_Context * kmt_context_save(_Event ev, _Context * context){

     if(cur_task->id != -1)
        cur_task->context = context;

     return NULL;
}

_Context * kmt_context_switch(_Event ev, _Context * context){
     //this func should not run while create is running 
 
     int heldtask = holding(&tasklock);
     if(!heldtask) my_spinlock(&tasklock);

     _Context *ret_con = NULL;
     
     if(cur_task->status == _READY){
          //assert(0);
         cur_task->status = _RUNNING;
         //heldtask = holding(&tasklock);
         ret_con = cur_task->context;
         if(!heldtask) my_spinunlock(&tasklock);
         return ret_con;
     }
      

/*
     if(cur_task->status == _WAKEUP){
          ret_con = cur_task->context;
          cur_task->status = _RUNNING;
          heldtask = holding(&tasklock);
          if(heldtask) my_spinunlock(&tasklock);
          return ret_con; 
     }
*/
     //polling which is not good 
     
     int findcnt = 0;//only once polling
     int tn = tl[_cpu()].tasknum;
     assert(tn != 0);
     for(int ind = (cur_task->id + 1) % tn; ; ind = (ind+1) % tn){
#ifdef DEBUG
         my_spinlock(&p_lk);
         printf("task: %s:   %d\n",tl[_cpu()].list[ind]->name,tl[_cpu()].list[ind]->status);
         my_spinunlock(&p_lk);
#endif
      //   printf("ind %d\n",ind); 
         if(tl[_cpu()].list[ind]->status == _READY){
             if(cur_task->id != -1){
                 if(cur_task->status == _RUNNING)
                      cur_task->status = _READY;
             }
             cur_task = tl[_cpu()].list[ind];
             cur_task->cpu = &cpu_info[_cpu()];
             cur_task->status = _RUNNING;
             break;
         }    
         findcnt++;
         if(findcnt == tn)// the rest task is all suspend not change 
             break;
     }
     
     assert(cur_task);
     assert(cur_task->cpu);
     assert(cur_task->status != _READY);

     
     
     
     /*   struct task *ind = taskhead;
     assert(ind);
     while(ind != NULL){
#ifdef DEBUG
        my_spinlock(&p_lk);
        printf("task: %s:   %d\n",ind->name,ind->status);
        my_spinunlock(&p_lk);
#endif
         if(ind->status == _READY){
             if(cur_task){
                if(cur_task->status == _RUNNING)
                     cur_task->status = _READY;
             } 
              cur_task = ind;
              cur_task->cpu = &cpu_info[_cpu()];
              cur_task->status = _RUNNING;
              break;
         }
         if(ind->status == _WAKEUP && ind->cpu == &cpu_info[_cpu()]){
             if(cur_task){
                 if(cur_task->status == _RUNNING)
                     cur_task->status = _READY;
             }
             cur_task = ind;
             cur_task->cpu = &cpu_info[_cpu()];
             cur_task->status = _RUNNING;
             break;
         }

         ind = ind->next;
     }
     assert(cur_task);
     assert(cur_task->cpu);
     assert(cur_task->status != _READY);
*/    
     ret_con = cur_task->context; 
    
     heldtask = holding(&tasklock);
     if(heldtask) my_spinunlock(&tasklock);
     return ret_con;
}
/*

static void kmt_semwait(struct semaphore *sem){
    my_spinlock(&sem->lock);

    if(sem->wlcnt > 0){
          sem->wlcnt--;
    }
    else if(sem->wlcnt == 0){
          cur_task->status = _SUSPEND;
          if(sem->wl[sem->wlright] != NULL)
              panic("The waiting queue is full!");
          sem->wl[sem->wlright] = cur_task;
          sem->wlright = (sem->wlright + 1) % NTHREAD;
    }
    else
        assert(0);

    my_spinunlock(&sem->lock);
}

static void kmt_semsignal(struct semaphore *sem){
    my_spinlock(&sem->lock);
    
    if(sem->wl[sem->wlleft] != NULL){
        sem->wl[sem->wlleft]->status = _READY;
        sem->wl[sem->wlleft] = NULL;
        sem->wlleft = (sem->wlleft + 1) % NTHREAD;
    }
    else{
        if(sem->wlleft != sem->wlright)
            panic("The waiting queue should be empty but not!");
        else
            sem->wlcnt++;
    }

    my_spinunlock(&sem->lock);
}



_Context *kmt_sem_syscall(_Event ev, _Context *context){
    // int held = holding(&syscalllock);
    // if(!held) my_spinlock(&syscalllock);
     if(ev.event != _EVENT_SYSCALL)
         panic("This intr is not syscall!\n");
     uintptr_t a[4];
     a[0] = context->eax;
     a[1] = context->ebx;
     a[2] = context->ecx;
     a[3] = context->edx;

     switch(a[0]){
         case _SEM_WAIT : kmt_semwait((struct semaphore *)a[1]); break;
         case _SEM_SIGN : kmt_semsignal((struct semaphore *)a[1]); break;
         default : assert(0);
     }     
    // if(!held) my_spinunlock(&syscalllock);
     return NULL;
}
*/
void kmt_init(){
     //taskhead = NULL;
     memset(tl,0,sizeof(tl));
     tidcnt = 0;
     for(int i = 0; i < MAXCPU; ++i)
         current_tasks[i]->id = -1;
     os->on_irq((int)INT_MIN, _EVENT_NULL, kmt_context_save);
     os->on_irq((int)INT_MAX, _EVENT_NULL, kmt_context_switch);
 //  os->on_irq(-1 , _EVENT_SYSCALL, kmt_sem_syscall);

}

int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){
     
     task->name = name;
     task->entry = entry;
     _Area stack = (_Area){ task->stack, task->stack + STK_SZ };
     task->context = _kcontext(stack, entry, arg);
     task->cpu = NULL;//&cpu_info[_cpu()];
     task->status = _READY;

     memset(task->fence1,0xcc,sizeof(task->fence1));
     memset(task->fence2,0xcc,sizeof(task->fence2));
     
     my_spinlock(&tasklock);
     //insert this task into tasklist
   
     //  task->next = taskhead;
   // taskhead = task;
     task->id = tl[tidcnt].tasknum;
     tl[tidcnt].list[tl[tidcnt].tasknum] = task; 
     tl[tidcnt].tasknum++;
     
#ifdef DEBUG
     my_spinlock(&p_lk);
    /* struct task* ind = taskhead;
     while(ind != NULL){
         printf("%s:\n",ind->name);
         ind = ind->next;
     }*/
     printf("tcpu:%d\n",tidcnt);
     int tn = tl[tidcnt].tasknum;
     for(int ind = 0; ind < tn; ++ind){
         printf("%s:\n",tl[tidcnt].list[ind]->name);
     }
    
     printf("\n");
     my_spinunlock(&p_lk);
#endif

     tidcnt = (tidcnt + 1) % _ncpu();
     my_spinunlock(&tasklock);
     
     return 0;
}


void kmt_teardown(task_t *task){
    //no dynamic alloc
    return;
}

MODULE_DEF(kmt){
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = my_spinlock_init,
    .spin_lock = my_spinlock,
    .spin_unlock = my_spinunlock,
    .sem_init = seminit,
    .sem_wait = semwait,
    .sem_signal = semsignal,
};
