#include <kvdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
kvdb_t db;
#define PNUM 4
//#define CHE
void *func(void* args){
       char * s = (char*)args;
        
        printf("open at thread 0\n"); 
#ifdef CHE
        assert(kvdb_open(&db,s) == 0);
#else
        kvdb_open(&db,s);
#endif
        printf("open down at thread 0\n");
        const char *key = "operating-systems";
        char *value;
        printf("get at thread 0\n");
        value = kvdb_get(&db,key);
       // assert(value != NULL);
        printf("get down at thread 0\n");
        printf("thread 0:[%s]: [%s]\n", key, value);
        free(value);
//        printf("close at thread 0\n");
//#ifdef CHE
//        assert(kvdb_close(&db) == 0);
//#else
//        kvdb_close(&db);
//#endif
//        printf("close down at thread 0\n");
        return NULL;
}


void *func1(void* args){
        char *s = (char*)args;
        printf("open at thread 1\n");
#ifdef CHE
        assert(kvdb_open(&db,s) == 0);
#else
        kvdb_open(&db,s);
#endif
        printf("open down at thread 1\n");
        const char *key = "operating-systems";
        printf("put at thread 1\n");
#ifdef CHE
        assert(kvdb_put(&db,key,"three-easy-pieces") == 0);
#else
        kvdb_put(&db, key, "three-easy-pieces");
#endif
        printf("put down at thread 1\n");
        char *value;
        printf("get at thread 1\n");
        value = kvdb_get(&db,key);
        //assert(value != NULL);
        printf("get down at thread 1\n");
        printf("thread 1: [%s]: [%s]\n", key, value);
        free(value);
//        printf("close at thread 1\n");
//#ifdef CHE
//        assert(kvdb_close(&db) == 0);
//#else
//        kvdb_close(&db);
//#endif
//        printf("close down at thread 1\n");
        return NULL;
}




void *func2(void*args){
        char * s = (char*) args;
        printf("open at thread 2\n");
#ifdef CHE
        assert(kvdb_open(&db,s) == 0);
#else
        kvdb_open(&db,s);
#endif
        printf("open down at thread 2\n");
        const char *key = "operating-systems";
        printf("put at thread 2\n");

        for(int j = 0; j < 1; ++j){
       
        uint8_t i;
        if(strcmp(s,"a.db") == 0)
            i = 0;
        else
            i = 5;
        for(; i < 10; ++i){
            int size = rand()%(1<<(1+i)) + 1;
           printf("%d\n",size);
            char *nval = (char*)malloc(size);
            memset(nval,0,size); 
            memset(nval,0x41+i,size-1);
            nval[size-1]='\0';
            printf("%s\n",nval);
#ifdef CHE
            assert(kvdb_put(&db,key,(const char*)nval) == 0);
#else
            kvdb_put(&db, key, (const char*)nval);
#endif
            printf("put down at thread 2\n");
        
        char *value;
        printf("get at thread 2\n");
        value = kvdb_get(&db,key);
        printf("get down at thread 2\n");
        assert(value != NULL);
        printf("thread 2: [%s]: [%s] size: %d\n", key, value,(int)strlen(value));
        //if(strcmp(value,nval) != 0) assert(0);
        free(value);
        free(nval);
        }
        }
        printf("close at thread 2\n");
#ifdef CHE
        assert(kvdb_close(&db) == 0);
#else
        kvdb_close(&db);
#endif
        printf("close down at thread 2\n");
        return NULL;
    
}

void *func3(void*args){
        char * s = (char*) args;
        printf("open at thread 3\n");
#ifdef CHE
        assert(kvdb_open(&db,s) == 0);
#else
        kvdb_open(&db,s);
#endif
        printf("open down at thread 3\n");
        const char *key = "jarvis";
        printf("put at thread 3\n");

        for(int j = 0; j < 1; ++j){
       
        uint8_t i;
        if(strcmp(s,"a.db") == 0)
            i = 0;
        else
            i = 5;
        for(; i < 10; ++i){
            int size = rand()%(1<<(10+i)) + 1;
            printf("%d\n",size);
            char *nval = (char*)malloc(size);
            memset(nval,0,size); 
            memset(nval,0x61+i,size-1);
            nval[size-1]='\0';
            printf("%s\n",nval);
#ifdef CHE
            assert(kvdb_put(&db,key,(const char*)nval) == 0);
#else
            kvdb_put(&db, key, (const char*)nval);
#endif
            printf("put down at thread 3\n");
        
        char *value;
        printf("get at thread 3\n");
        value = kvdb_get(&db,key);
        printf("get down at thread 3\n");
        assert(value != NULL);
        printf("thread 3: [%s]: [%s] size: %d\n", key, value,(int)strlen(value));
        //if(strcmp(value,nval) != 0) assert(0);
        free(value);
        free(nval);
        }
        }
//        printf("close at thread 2\n");
//#ifdef CHE
//        assert(kvdb_close(&db) == 0);
//#else
//        kvdb_close(&db);
//#endif
//        printf("close down at thread 2\n");
        return NULL;
    
}

void progress1(){
   pthread_t pthread[PNUM];
   int i;
   for(i = 0; i < PNUM; ++i){
       int ex;
       switch(i){
           case 0:
                ex = pthread_create(&pthread[i],NULL,func,"a.db");
                break;
           case 1:
                ex = pthread_create(&pthread[i],NULL,func1,"a.db");
                break;
           case 2:
                ex = pthread_create(&pthread[i],NULL,func2,"a.db");
                break;
           case 3:
                ex = pthread_create(&pthread[i],NULL,func3,"a.db");
                break;
       }
       if(ex != 0){
           printf("wrong!\n");
       }
   }


   for(i = 0; i < PNUM; ++i){
       pthread_join(pthread[i],NULL);
   }

}


void progress2(){
   pthread_t pthread[PNUM];
   int i;
   for(i = 0; i < PNUM; ++i){
       int ex;
       switch(i){
           case 0:
                ex = pthread_create(&pthread[i],NULL,func,"b.db");
                break;
           case 1:
                ex = pthread_create(&pthread[i],NULL,func1,"b.db");
                break;
           case 2:
                ex = pthread_create(&pthread[i],NULL,func2,"b.db");
                break;
           case 3:
                ex = pthread_create(&pthread[i],NULL,func3,"b.db");
                break;
           default:break;
       }
       if(ex != 0){
           printf("wrong!\n");
       }
   }


   for(i = 0; i < PNUM; ++i){
       pthread_join(pthread[i],NULL);
   }

}

int main(){

    int pid = fork();
    if(pid < 0){ perror("fork"); exit(1); }

    if(pid == 0){
        srand((unsigned)time(NULL));
        progress1();
    }
    else{
        srand((unsigned)time(NULL));
        progress2();
    }


//  srand((unsigned)time(NULL));
//  progress2();

}
