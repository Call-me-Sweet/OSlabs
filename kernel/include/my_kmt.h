#ifndef __KMT_H__
#define __KMT_H__
#include <stdint.h>
#include <common.h>
#include <klib.h>
#include <my_os.h>
#include <vfs.h>

//#define CHARCHECK
//#define PANDC
//#define TTYTEST
//#define DEBUG

//#define L3TEST
// ==============================HandlerList==========================

typedef struct HL{
    handler_t handler;
    int event;
    int seq;
    struct HL* next;
}HandlerList;

// =============================multiple thread control==============
#define INT_MAX 2147483647
#define INT_MIN -2147483648

#define STK_SZ (4096*1024)
#define NTHREAD 40
#define MAXCPU 8
enum{//task status
    _READY = 0,
    _RUNNING,
    _SUSPEND,
//    _WAKEUP,
};
enum{//syscall
    _SEM_WAIT = 0,
    _SEM_SIGN,
};

struct task{
    const char *name;
    int id;//ind in array of per cpu
    _Context *context;
    struct mycpu *cpu;
    int status;
    void (*entry)(void *arg);
    
    file_t *fildes[NFILE];

    uint8_t fence1[32];
    uint8_t stack[STK_SZ];
    uint8_t fence2[32];
  //  struct task *next;
};

struct tasklist{
    struct task *list[NTHREAD];
    int tasknum;
};

extern struct task* current_tasks[MAXCPU];
#define cur_task current_tasks[_cpu()]

_Context *kmt_context_save(_Event ev, _Context *context);
_Context *kmt_context_switch(_Event ev, _Context *context);
void kmt_init();
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
void kmt_teardown(task_t *task);


// ===================================sem part========================
struct semaphore{
   int value;
   const char* name;
   int active;//judge whether the sem is init
   struct task *wl[NTHREAD];//waitinglist of sem
   int wlleft,wlright;
//   uint32_t wlcnt;//during the unlock and yield how many times sign
   struct spinlock lock;
};

void seminit(sem_t *sem, const char *name, int value);
void semwait(sem_t *sem);
void semsignal(sem_t *sem);

#endif
