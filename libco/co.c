#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ucontext.h>
#include <assert.h>
#include <time.h>
#include "co.h"


struct co {
  ucontext_t uctx;
  char name[16];
  enum State state;
  uint16_t cnt;//the first time need to come back to main()
  uint16_t _stack[4096]__attribute__((aligned(16)));
}__attribute__((aligned(16)));

struct co  coroutines[MAX_CO];
int current;//-1 means main(),others related to coroutines;
//time_t t = 0;//the seed

struct co dual[MAX_CO];//the cording of coroutines


void co_init() {
  current = -1;
  for(int i = 0; i < MAX_CO; ++i){
      coroutines[i].state = FREE;
      dual[i].state = FREE;
      coroutines[i].cnt = 0;
  }
}

struct co* co_start(const char *name, func_t func, void *arg) {
  int id;
  for(id = 0; id < MAX_CO; ++id){
      if(coroutines[id].state == FREE)
          break;
  }
  
  assert(id!=MAX_CO);
 
  strcpy(coroutines[id].name,name);
  coroutines[id].state = ABLE;
  dual[id].state = ABLE;

  coroutines[id].cnt = 0;//the first time

  int ret = getcontext(&coroutines[id].uctx);
  assert(ret != -1);
  coroutines[id].uctx.uc_stack.ss_sp = coroutines[id]._stack;
  coroutines[id].uctx.uc_stack.ss_size = 4096;
  coroutines[id].uctx.uc_stack.ss_flags = 0;

  ret =  getcontext(&dual[id].uctx);
  assert(ret != -1);
  dual[id].uctx.uc_stack.ss_sp = dual[id]._stack;
  dual[id].uctx.uc_stack.ss_size = 4096;
  dual[id].uctx.uc_stack.ss_flags = 0;
  
  coroutines[id].uctx.uc_link = &(dual[id].uctx);

  makecontext(&coroutines[id].uctx,(void (*)(void))func,1,arg);
  
  coroutines[id].state = RUNNING;
  current = id;
  
  ret = swapcontext(&dual[id].uctx,&coroutines[id].uctx);
  assert(ret != -1);
//  srand((unsigned)time(&t));
  
  return &coroutines[id];
}
            
void co_yield() {
    int ret; 
    if(coroutines[current].cnt == 0){//back to main()
         coroutines[current].state = ABLE;
         coroutines[current].cnt++;
        // dual[current].state = RUNNING;
         
         int tem = current; 
         current = -1;
         
         ret = swapcontext(&coroutines[tem].uctx,&dual[tem].uctx);
         assert(ret != -1);
     }
     else{//rand select
         int tem = current;
         coroutines[tem].state = ABLE;
        
         
         /*
          * rand failed!!!
          */

         //firstly run dual 
        /* int index = 0;
         while(dual[index].state != ABLE && index <MAX_CO) index++;
         
         if(index != MAX_CO){
            current = -1;
            swapcontext(&coroutines[tem].uctx,&dual[index].uctx);
            return;
         }
         
         int ind = rand() % MAX_CO;
         while(coroutines[ind].state != ABLE){
             ind = rand() % MAX_CO;
         }
         */

         /*
          * turns
          */
        
         int ind = tem;
         for(int i = 0; i < MAX_CO; ++i){
             if(ind == MAX_CO-1) ind = 0;
             else ind++;
             if(coroutines[ind].state == ABLE) break;
         }
         current = ind;
         coroutines[current].state = RUNNING;
         if(current == tem) 
             return;
         else{
           ret = swapcontext(&coroutines[tem].uctx,&coroutines[current].uctx);
           assert(ret != -1);
         }  
           
     }

}

int tt;
void co_wait(struct co *thd) {
    int tem = current;
    tt = 0;
    while(strcmp(coroutines[tt].name,thd->name)) tt++;
    if(tem == -1){
        current = tt;
        coroutines[current].state = RUNNING;
        dual[current].state = SUSPEND;
        int ret = swapcontext(&dual[current].uctx,&coroutines[current].uctx);
        assert(ret != -1);
    }

   /* else{
        int tt = 0;
        while(strcmp(coroutines[tt].name,thd->name)) tt++;
        current = tt;
        coroutines[current].state = RUNNING;
        coroutines[tem].state = SUSPEND;
        
        swapcontext(&coroutines[tem].uctx,&coroutines[current].uctx);
        //printf("tem:%d\n",tem);
        //assert(0);
    }
*/
    coroutines[tt].state = FREE;
    dual[tt].state = FREE;
    current = -1;
}

