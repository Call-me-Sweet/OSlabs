#include <devices.h>
#include <common.h>
#include <klib.h>
void my_spinlock_init(struct spinlock *lk,const char *name){
    lk->name = name;
    lk->locked = 0;
    lk->cpu = NULL;
}

struct mycpu cpu_info[8] = {
    {0,0,1},
    {1,0,1},
    {2,0,1},
    {3,0,1},
    {4,0,1},
    {5,0,1},
    {6,0,1},
    {7,0,1},
};

/*
void getcallerpcs(void* v,uint32_t pcs[]){
    //TBD
}
*/
void pushcli(){
    int efl;
    efl = get_efl();
    _intr_write(0);//cli
    if(cpu_info[_cpu()].ncli == 0){
        cpu_info[_cpu()].intena = efl & FL_IF;
    }
    cpu_info[_cpu()].ncli++;

}

void popcli(){
    if(get_efl()&FL_IF)
        panic("popcli-intr");
    if(--cpu_info[_cpu()].ncli < 0){
        panic("popcli");
    }
    if(cpu_info[_cpu()].ncli == 0 && cpu_info[_cpu()].intena)
        _intr_write(1);//sti;
}

int holding(struct spinlock *lk){
    int r;
    pushcli();
    r = lk->locked && (lk->cpu == &cpu_info[_cpu()]);
    popcli();
    return r;
}

void my_spinlock(struct spinlock *lk){
    pushcli();
    if(holding(lk)){
        printf("%s\n",lk->name);
        assert(0);
        panic("spinlock");
    }

    while(_atomic_xchg(&lk->locked,1));

    lk->cpu = &cpu_info[_cpu()];
 //   getcallerpcs(&lk,lk->pcs);
}

void my_spinunlock(struct spinlock *lk){
    if(!holding(lk)){
        printf("%s\n",lk->name);
        assert(0);
        panic("spinunlock");
    }
    //lk->pcs[0] = 0;
    lk->cpu = NULL;

    _atomic_xchg(&lk->locked,0);
    popcli();
}









/*
static inline void pushcli(){
    cli();
    cpu_cnt[_cpu()]++;
}

static inline void popcli(){
    assert(cpu_cnt[_cpu()] != 0);
    cpu_cnt[_cpu()]--;
    if(cpu_cnt[_cpu()] == 0)
        sti();
}

static inline void my_spinlock(my_lock *lk){
    //cli();
    pushcli();
    while(_atomic_xchg(&lk->locked,1));
    lk->cpu = _cpu();
}

static inline void my_spinunlock(my_lock *lk){
    _atomic_xchg(&lk->locked,0);
    lk->cpu = -1;
    //sti();
    popcli();
}
*/
