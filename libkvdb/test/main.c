#include "kvdb.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#define PNUM 3
//#define CHE

kvdb_t db;

void *func(void* args){
        
        printf("open at thread 0\n"); 
#ifdef CHE
        assert(kvdb_open(&db,"a.db") == 0);
#else
        kvdb_open(&db,"a.db");
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
        printf("open at thread 1\n");
#ifdef CHE
        assert(kvdb_open(&db,"a.db") == 0);
#else
        kvdb_open(&db,"a.db");
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
        printf("open at thread 2\n");
#ifdef CHE
        assert(kvdb_open(&db,"a.db") == 0);
#else
        kvdb_open(&db,"a.db");
#endif
        printf("open down at thread 2\n");
        const char *key = "operating-systems";
        printf("put at thread 2\n");

        for(uint8_t i = 0; i < 10; ++i){
            int size = rand()%(1<<(12-i));
            printf("%d\n",size);
            char *nval = (char*)malloc(size);
            memset(nval,0x41+i,size);
#ifdef CHE
            assert(kvdb_put(&db,key,(const char*)nval) == 0);
#else
            kvdb_put(&db, key, (const char*)nval);
#endif
            printf("put down at thread 2\n");
        }
        char *value;
        printf("get at thread 2\n");
        value = kvdb_get(&db,key);
        //assert(value != NULL);
        printf("get down at thread 2\n");
        printf("thread 2: [%s]: [%s]\n", key, value);
        free(value);
        printf("close at thread 2\n");
#ifdef CHE
        assert(kvdb_close(&db) == 0);
#else
        kvdb_close(&db);
#endif
        printf("close down at thread 2\n");
        return NULL;
    
}


int main(){
    pthread_t pthread[PNUM];
    srand((unsigned)time(NULL)); 
    int i;
    for(i = 0; i < PNUM; ++i){
        int ex;
        int mod = i%PNUM;
        switch (mod){
            case 0:
             ex = pthread_create(&pthread[i],NULL,func,&i);
             break;
            case 1:
             ex = pthread_create(&pthread[i],NULL,func1,&i);
             break;
            case 2:
             ex = pthread_create(&pthread[i],NULL,func2,&i);
             break;
        }
        
        if(ex == 0)
             fprintf(stderr,"Create thread %d\n",i);
         else
             fprintf(stderr,"Create failed!\n");
    }
    for(i = 0; i < PNUM; ++i){
        pthread_join(pthread[i],NULL);
    }

    return 0;
}

