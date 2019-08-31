#include <common.h>
#include <klib.h>
#include <devices.h>
#include <kernel.h>
//static struct spinlock p_lk;

HandlerList *handlehead; 
static struct spinlock HLlock = {0,"HandlerListlock",NULL}; 
static struct spinlock traplock = {0,"os_traplock",NULL};


static void zombie(void *arg){
    while(1) _yield();
}

#ifdef PANDC
static sem_t fill,emp;

static void comsumer( void *arg){
    while(1){
        kmt->sem_wait(&fill);
        _putc(')');
        _putc(' ');
        kmt->sem_signal(&emp);
    } 
}

static void procedur( void *arg){
    while(1){
        kmt->sem_wait(&emp);
        _putc('(');
        _putc(' ');
        kmt->sem_signal(&fill);
    }
}
#endif

#ifdef CHARCHECK
static void func(void *arg){
    int cur = (intptr_t)arg;
    while(1){
        switch(cur){
            case 0: _putc('0'); break;
            case 1:;_putc('1'); break;
            case 2: _putc('2'); break;
            case 3: _putc('3'); break;
            default: break;
        }
    }
}
#endif

#ifdef TTYTEST
extern ssize_t tty_write(device_t *dev, off_t offset, const void* buf, size_t count);

void echo_task(void *name){
    device_t *tty = dev_lookup(name);
    while(1){
        char line[128], text[128];
        sprintf(text, "(%s) $",name);
        tty_write(tty, 0, (const char*)text, strlen(text));
        int nread = tty->ops->read(tty, 0, line, sizeof(line));
        line[nread - 1] = '\0';
        sprintf(text, "Echo: %s\n", line);
        tty_write(tty, 0, (const char*)text, strlen(text));
    }
}
#endif

extern filesystem_t *devfs;
extern filesystem_t *blkfs;
extern filesystem_t *procfs;
spinlock_t plock = {0,"plock",NULL};
extern void shell_thread(void*args);
#ifdef L3test
void l3test(void* args){
    int caces = vfs->access("/proc/cpuinfo",F_OK);
     my_spinlock(&plock);
     logq("cpu %d,aces %d",_cpu(),caces);
     my_spinunlock(&plock);

     int copen = vfs->open("/proc/meminfo",0);
     my_spinlock(&plock);
     logy("cpu %d, copen %d",_cpu(),copen);
     my_spinunlock(&plock);
  
     int ss[128];
     memset(ss,0,sizeof(ss));
     int rbyte = vfs->read(copen,(void*)ss,sizeof(ss));
     my_spinlock(&plock);
     logy("cpu %d, rbyte %d, ss %s",_cpu(),rbyte,ss);
     my_spinunlock(&plock);

     while(1); 
}   
#endif

extern struct Proc hgproc;
static void os_init() {
  pmm->init();
  handlehead = NULL;
  kmt->init();
  //
  dev->init();
  vfs->init();
  int pidcnt = 1;
  for(int i = 0; i < _ncpu(); ++i)
  {
     kmt->create(pmm->alloc(sizeof(task_t)), "zombie", zombie, NULL);
     char buf[128];
     memset(buf,0,sizeof(buf));
     sprintf(buf,"/proc/%d",pidcnt);
     vfs->open(buf,O_CREAT|O_RDWR);
     memset(buf,0,sizeof(buf));
     sprintf(buf,"ThreadName: zombie%d.\nCreateTime: %d",i,uptime());
     memcpy(hgproc.block[pidcnt+2],buf,strlen(buf)+1);
     pidcnt++;
  }
#ifdef TTYTEST
  kmt->create(pmm->alloc(sizeof(task_t)), "print1", echo_task,"tty1");
  kmt->create(pmm->alloc(sizeof(task_t)), "print2", echo_task,"tty2");
  kmt->create(pmm->alloc(sizeof(task_t)), "print",echo_task,"tty3");
  kmt->create(pmm->alloc(sizeof(task_t)), "print",echo_task,"tty4");
#endif

#ifdef SHELL 
  for(int i = 0; i< 4; ++i){
      kmt->create(pmm->alloc(sizeof(task_t)),"shell_thread",shell_thread,(void*)(i+1));
      char buf[128];
      memset(buf,0,sizeof(buf));
      sprintf(buf,"/proc/%d",pidcnt);
      vfs->open(buf,O_CREAT|O_RDWR);
      memset(buf,0,sizeof(buf));
      sprintf(buf,"ThreadName:    shell_thread%d.\nCreateTime:   %d",i,uptime());
      memcpy(hgproc.block[pidcnt+2],buf,strlen(buf)+1);
      pidcnt++;
  }
#endif

#ifdef PANDC
  kmt->sem_init(&fill,"fill",0);
  kmt->sem_init(&emp,"emp",1);
  kmt->create(pmm->alloc(sizeof(task_t)), "proceduer1",procedur,NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "consumer1",comsumer, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "proceduer2",procedur,NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "consumer2",comsumer, NULL);
#endif

#ifdef CHARCHECK
  kmt->create(pmm->alloc(sizeof(task_t)), "func", func,(void*)0);
  kmt->create(pmm->alloc(sizeof(task_t)), "func", func,(void*)1);
  kmt->create(pmm->alloc(sizeof(task_t)), "func", func,(void*)2);
  kmt->create(pmm->alloc(sizeof(task_t)), "func", func,(void*)3);
#endif
}

/*
 static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
  }
  _putc("12345678"[_cpu()]); _putc('\n');
}*/

/*
static void alloc_test(){
#ifdef DEBUG
    my_spinlock(&p_lk);
    printf("LEN(l_header):%d\tLEN(m_header):%d\n",LEN(l_header),LEN(m_header));
    my_spinunlock(&p_lk);
#endif
    void * test[500];//8cpu test 10 times
    
    for(int i = 0; i < 100; ++i){
        test[i] = pmm->alloc(rand()%((1<<20)-100));
        //assert(test[_cpu()][i]);
   
     //   my_spinlock(&p_lk);
      //  printf("add: %p and size :xxxxx at cpu %d times: %d\n",test[_cpu()][i],_cpu(),i+1);
       // my_spinunlock(&p_lk);
       
    }
    for(int i = 0; i < 2000; ++i){
          int tmp = rand() % 100;
         // my_spinlock(&p_lk);
         // printf("free add:%p\n",test[_cpu()][tmp]);
         // my_spinunlock(&p_lk);
          pmm->free(test[tmp]);
          
         
          test[tmp] = pmm->alloc(rand()%((1<<20)-100));
         // my_spinlock(&p_lk);
         // printf("add: %p and size :yyyy at cpu %d times: %d\n",test[_cpu()][tmp],_cpu(),i+1);
         // my_spinunlock(&p_lk);
   
    }
    for(int i = 0; i < 100; ++i)
    {
       // my_spinlock(&p_lk);
       // printf("free add:%p\n",test[_cpu()][i]);
       // my_spinunlock(&p_lk);
        pmm->free(test[i]);
    }
    my_spinlock(&p_lk);
    printf("finish\n");
    my_spinunlock(&p_lk);
    
}
*/


static void os_run() {
 // hello();
// alloc_test();
  _intr_write(1);
  while (1) {
    _yield();
  }
}



static _Context *os_trap(_Event ev, _Context *context) {
   
    int heldtrap = holding(&traplock);
    if(!heldtrap) my_spinlock(&traplock); 
    
    //printf("ev:%d\n",ev.event);

    if(ev.event == _EVENT_ERROR){
        //sth wrong
        my_spinlock(&plock);
        printf("curcpu:%d\n",_cpu());
        printf("event:%d: %s",ev.event,ev.msg);
        printf("   cause by %p and %p\n",ev.cause,ev.ref);
        printf("name :%s\n",cur_task->name);
        assert(0);
        my_spinunlock(&plock);
        return context;
    }
   
    // the I/O may intr the os_trap and again come here repeatly get the same lk;
#ifdef DEBUG
     my_spinlock(&plock);
     printf("begins: %d\n",_cpu());
     printf("switch from:%s\n",cur_task->name);
     my_spinunlock(&plock);
#endif

    _Context *ret_context = NULL;
 
    int heldHL = holding(&HLlock);
    if(!heldHL) my_spinlock(&HLlock);

    HandlerList * ind = handlehead;
    assert(ind);
    while(ind != NULL){
         if(ind->event == _EVENT_NULL || ind->event == ev.event){
            _Context *next = ind->handler(ev, context);
            if(next) ret_context = next;
      
        }
        ind = ind->next;
    }
   
    assert(ret_context);
    heldHL = holding(&HLlock);
    if(heldHL) my_spinunlock(&HLlock);
#ifdef DEBUG
    my_spinlock(&plock);
    printf("switch to:%s\n",cur_task->name);
    printf("end\n");
    my_spinunlock(&plock);
    
#endif
    heldtrap = holding(&traplock);
    if(heldtrap) my_spinunlock(&traplock);
  
    return ret_context;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  // int held = holding(&HLlock);
  // if(!held) my_spinlock(&HLlock);
 
   HandlerList * newhandler = (HandlerList *)pmm->alloc(sizeof(HandlerList));
  

   newhandler->handler = handler;
   newhandler->event = event;
   newhandler->seq = seq;
  // printf("handseq:%d\n",newhandler->seq);
   //printf("%d\n",event);
   // sort handler according to seq
   HandlerList * ind = handlehead;
   HandlerList * old = ind;
   while(ind != NULL){
       if(newhandler->seq > ind->seq){
            old = ind;
            ind = ind->next;
       }
       else
            break;
   }
   
   newhandler->next = ind;
   if(old != NULL)
      old->next = newhandler;
   if(ind == handlehead)
       handlehead = newhandler;
   assert(handlehead);
#ifdef DEBUG
   my_spinlock(&plock);
   ind = handlehead;
   while(ind != NULL){
       printf("ev:%d seq:%d\n",ind->event,ind->seq);
       ind = ind->next;
   }

   printf("\n");
   my_spinunlock(&plock);
#endif
  // held = holding(&HLlock);
  // if(held) my_spinunlock(&HLlock);

}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
