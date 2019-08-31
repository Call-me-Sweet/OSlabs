#include <common.h>
#include <klib.h>
#include <my_os.h>

static uintptr_t pm_start, pm_end;


#ifdef CORRECTNESS_FIRST
 static uintptr_t estart;
#else
 static uintptr_t pm_now;
 static l_header* recycle;
 static m_header* using;
#endif
static struct spinlock alloc_lk;

static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
#ifdef CORRECTNESS_FIRST
  estart = (uintptr_t) _heap.start;
#else
  pm_now = (uintptr_t)_heap.start;
  using = NULL;
  recycle = NULL;
#endif
//init lock
  my_spinlock_init(&alloc_lk,"alloclock");
 
 // printf("heap_start:%d\n",pm_start);
}
#ifndef CORRECTNESS_FIRST

static uintptr_t create(size_t size){//need to alloc from heap
    uintptr_t ret;
    ret = pm_now;
    pm_now += (LEN(l_header) + alloc_num*size);//alloc 4 
    if(pm_now > pm_end){
        my_spinlock(&plock);
        printf("The heap is not enough!\n");
        assert(0);
        my_spinunlock(&plcok);
    }
    return ret;
} 

static uintptr_t big_alloc(size_t size){//already locked in kalloc
      size_t rsize = size + LEN(m_header);
      assert(rsize >= LEN(m_header));
      #ifdef POW
      int index = 0;
      while(1){
          if(rsize > (1<<index) && rsize <= (1<<(index+1)))
              break;
          index++;
      }
    
     size_t alloc_size = 1<<(index + 1);//the size need to alloc
     #else
     size_t alloc_size = rsize; 
     #endif
      
     
     if(recycle == NULL){
        recycle = (l_header*) create(alloc_size);//alloc from tail
        recycle->lsize = alloc_size;
        recycle->prev = NULL;
        recycle->next = NULL;
        
        m_header* tmpm = (m_header*)((uintptr_t)recycle + LEN(l_header));
        recycle->bmheader = tmpm;
        tmpm->smaddr = (uintptr_t)tmpm + LEN(m_header);
        //tmpm->myblock = NULL;
        tmpm->msize = alloc_size - LEN(m_header);
        tmpm->prev = NULL;
        for(int i = 0; i < (alloc_num-1); ++i){
             m_header* tmpn = (m_header*)((uintptr_t)tmpm + alloc_size);
             tmpn->smaddr = (uintptr_t)tmpn + LEN(m_header);
           //  tmpn->myblock = NULL;
             tmpn->msize = alloc_size - LEN(m_header);
             tmpn->prev = tmpm;
             tmpm->next = tmpn;
             if(i == (alloc_num - 2)) tmpn->next = NULL;
             tmpm = tmpn;
        }

        //sert into using and return
        //now tmpm is the last,which will be alloced
        if(using == NULL){
            using = tmpm;
            //if(tmpm->prev!= NULL)
            assert(tmpm->prev);
            assert(tmpm->next == NULL);
            tmpm->prev->next = NULL;
            tmpm->prev = NULL;//because using is null
            return using->smaddr; 
        }
        else{
            assert(0);//impossible
        }
     }
     else{//recycle is not NULL

         l_header * pointer = recycle; 
         assert(pointer);
         while(pointer != NULL){
             if((pointer->lsize == alloc_size) && (pointer->bmheader != NULL))
                 break;
             pointer = pointer->next;
         }
         if(pointer == NULL){//means need to create
             l_header * newl = (l_header*) create(alloc_size);//alloc from tail
             assert(newl);
             newl->lsize = alloc_size;
             newl->prev = NULL;
             newl->next = recycle;
             recycle->prev = newl;
             recycle = newl;
                
             m_header* tmpm = (m_header*)((uintptr_t)recycle + LEN(l_header));
             recycle->bmheader = tmpm;
             recycle->bmheader->prev = NULL;
             tmpm->smaddr = (uintptr_t)tmpm + LEN(m_header);
             //tmpm->myblock = NULL;
             tmpm->msize = alloc_size - LEN(m_header);
             tmpm->prev = NULL;
             for(int i = 0; i < (alloc_num-1); ++i){
                 m_header* tmpn = (m_header*)((uintptr_t)tmpm + alloc_size);
                 tmpn->smaddr = (uintptr_t)tmpn + LEN(m_header);
               //  tmpn->myblock = NULL;
                 tmpn->msize = alloc_size - LEN(m_header);
                 tmpn->prev = tmpm;
                 tmpm->next = tmpn;
                 if(i == (alloc_num - 2)) tmpn->next = NULL;
                 tmpm = tmpn;
             }
             if(using == NULL){
                 using  = tmpm;
                 assert(tmpm->prev);
                 assert(tmpm->next == NULL);
                 tmpm->prev->next = NULL;
                 tmpm->prev = NULL;//because using is null
                 //tmpm->next = NULL; //it's tail
                 return using->smaddr; 
             }
             else{//using is not null
                 assert(tmpm->prev);
                 assert(tmpm->next == NULL);
                 tmpm->prev->next = NULL;
                 //tmpm->next = NULL; //it's tail
                 tmpm->prev = NULL;//insert at first of using
                 tmpm->next = using;
                 using->prev = tmpm;
                 using  = tmpm;
                 return using->smaddr;
             }
         }
         else{//pointer is not null
            m_header* tmpm = pointer->bmheader;
            assert(tmpm);
            
            assert(tmpm->prev == NULL);
            pointer->bmheader = tmpm->next;
            if(pointer->bmheader != NULL){
                pointer->bmheader->prev = NULL;
            }
             
            tmpm->prev = tmpm->next = NULL;
            if(using == NULL){
                using = tmpm;
                using->prev = using->next = NULL;
                return using->smaddr;
            }
            else{//using isnot null
               tmpm->prev = NULL;
               tmpm->next = using;
               using->prev = tmpm;
               using = tmpm;
               return using->smaddr;
            }
         }
        //recycle is not null 
     }
}


static void big_free(void * ptr){//ptr is the m_header pointer to the aim 
    assert(ptr);
    assert(recycle);
    m_header * mptr = (m_header*)ptr;
    l_header* lptr = recycle;
    while(lptr != NULL){
        if(lptr->lsize == mptr->msize + LEN(m_header))
            break;
        lptr = lptr->next;
    } 
    assert(lptr);
    mptr->prev = NULL;
    mptr->next = lptr->bmheader;
    lptr->bmheader = mptr;
    lptr->bmheader->prev = NULL;
    if(mptr->next){
        mptr->next->prev = mptr;
    }
    return;
}
#endif


static void *kalloc(size_t size) {
#ifdef CORRECTNESS_FIRST
    my_spinlock(&alloc_lk);
    estart += size;
  
    if(estart > pm_end){
        my_spinlock(&plock);
        printf("Heap isnot enough!\n");
        assert(0);
        my_spinunlock(&plock);
    }
    
    memset((void*)(estart-size),0,size);
    my_spinunlock(&alloc_lk);
    return (void*) (estart - size);
#else 
    //complex
    my_spinlock(&alloc_lk);
    //printf("heap_start:%d\n",pm_start);
    void * ret = (void*)big_alloc(size);
    
    memset(ret,0,size);
    my_spinunlock(&alloc_lk);
    return ret;
#endif
}

static void kfree(void *ptr) {
#ifdef CORRECTNESS_FIRST
    return;
#else
    //complex
    my_spinlock(&alloc_lk);
    m_header * tmph = (m_header *)((uintptr_t)ptr - LEN(m_header));
    assert(tmph);
     
    if(using == NULL){
         my_spinlock(&plock);
         printf("You cannot free a null ptr!\n");
         assert(0);
         my_spinunlock(&plock);
    }

    m_header *mptr = using;
    while(mptr != NULL){
        if(mptr == tmph)
              break;
        mptr = mptr->next;
    }
        
    if(mptr == NULL){
        my_spinlock(&plock);
        printf("You cannot free the addr already free\n");
       // assert(0);
        my_spinunlock(&plock);
        my_spinunlock(&alloc_lk);
        return;
    }
        
    if(mptr == using){
          using = using->next;
          if(using)
               using->prev = NULL;
    }
    else{
          assert(mptr->prev);
          mptr->prev->next = mptr->next;
          if(mptr->next){
              mptr->next->prev = mptr->prev;
          }
    }
          
    mptr->next = mptr->prev = NULL;
    big_free((void*)mptr);
    my_spinunlock(&alloc_lk);
    return; 
#endif
}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};
