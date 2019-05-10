#include <stdint.h>
#include <am.h>

//#define DEBUG
#define CORRECTNESS_FIRST
//#define MYSTI() __asm__ __volatile__("sti":::"memory")
//#define MYCLI() __asm__ __volatile__("cli":::"memory")

#define POW
#define diffsize (4*1024)//4KB
#define LEN(x) (sizeof(x))
#define unitsize (diffsize - LEN(m_header))
#define alloc_num (4)

// cpu() to get the current cpu
// ncpu() is the number of cpu

struct mycpu{
     int id;
     int ncli;
     int intena;
};

struct spinlock{
//    char name[20];
    intptr_t locked;
    const char *name;
    struct mycpu *cpu;
    //uint32_t pcs[10];
};

extern struct mycpu cpu_info[8];

int holding(struct spinlock *lk);
void my_spinlock(struct spinlock *lk);
void my_spinunlock(struct spinlock *lk);
void my_spinlock_init(struct spinlock *lk,const char *name);

extern struct spinlock p_lk;

#ifndef CORRECTNESS_FIRST
/*typedef struct BLOCK{
    size_t maxusable;
    uintptr_t saddr;//the start address of BLOCK
    struct BLOCK *prev,*next;
    struct m_header * bheader;//the first header of this block
}BLOCK;
*/
typedef struct m_header{
   uintptr_t smaddr;//the begin of usable mm without block and header
   //BLOCK* myblock;//for big mm is NULL
   size_t msize;//useful when kfree
   struct m_header *next,*prev;
}m_header;
/*
typedef struct CPULIST{
    BLOCK* head_block;
}CPULIST;
*/
typedef struct l_header{
    struct l_header *next, *prev;
    size_t lsize;
    m_header *bmheader;
}l_header;

#endif


