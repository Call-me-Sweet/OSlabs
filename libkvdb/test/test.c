#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kvdb.h"
#include <pthread.h>

const char db_path[] = "/home/jarviszly/os/os-workbench/libkvdb/test/a.db";

kvdb_t db;

const char * key   = "operating-system";
const char * key1  = "mcf-test-1";
const char * key2 = \
"\
TESTLONG--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--\
TESTLONG--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--\
TESTLONG--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--\
TESTLONG--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--\
TESTLONG--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--AAAAAAAA--\
";

const char * val2 = \
"\
CODELONG--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--\
CODELONG--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--\
CODELONG--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--\
CODELONG--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--\
CODELONG--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--BBBBBBBB--\
";

const char key_thread0[] = "THREAD-TEST0";
const char key_thread1[] = "THEEAD-TEST1";
const char * val_thread0[] = {
    "THREAD-VAL0-0",
    "THREAD-VAL0-1",
    "THREAD-VAL0-2",
    "THREAD-VAL0-3",
    "THREAD-VAL0-4",
    "THREAD-VAL0-5",
    "THREAD-VAL0-6",
    "THREAD-VAL0-7",
    "THREAD-VAL0-8",
    "THREAD-VAL0-9"
};

const char * val_thread1[] = {
    "THREAD-VAL1-0",
    "THREAD-VAL1-1",
    "THREAD-VAL1-2",
    "THREAD-VAL1-3",
    "THREAD-VAL1-4",
    "THREAD-VAL1-5",
    "THREAD-VAL1-6",
    "THREAD-VAL1-7",
    "THREAD-VAL1-8",
    "THREAD-VAL1-9"
};



void test1(){/*{{{*/
    char * value = NULL;
    kvdb_put(&db, key, "three-easy-pieces");
    value = kvdb_get(&db, key);
    printf("[%s]: [%s]\n", key, value);
    free(value);

    kvdb_put(&db, key, "three-easy-pieces-gggggggggg");
    value = kvdb_get(&db, key);
    printf("[%s]: [%s]\n", key, value);
    free(value);

    kvdb_put(&db, key1, "mcf-code-test-1");
    value = kvdb_get(&db, key1);
    printf("[%s]: [%s]\n", key1, value);
    free(value);

    kvdb_put(&db, key, "three-easy-pieces-GGGGGGGGGG");
    value = kvdb_get(&db, key);
    printf("[%s]: [%s]\n", key, value);
    free(value);
   
    kvdb_put(&db, key1, "mcf-code-test-1-AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    value = kvdb_get(&db, key1);
    printf("[%s]: [%s]\n", key1, value);
    free(value);

    kvdb_put(&db, key, "three-easy-pieces-hhhhhhhhhh");
    value = kvdb_get(&db, key);
    printf("[%s]: [%s]\n", key, value);
    free(value);

    kvdb_put(&db, key1, "mcf-code-test-1-VVVVVVVVVVV");
    value = kvdb_get(&db, key1);
    printf("[%s]: [%s]\n", key1, value);
    free(value);

  
    kvdb_put(&db, key2, val2);
    value = kvdb_get(&db, key2);
    printf("[%s]: [%s]\n", key2, value);
    free(value);

    kvdb_put(&db, key2, "CODELONG-SHORT-TEST-1");
    value = kvdb_get(&db, key2);
    printf("[%s]: [%s]\n", key2, value);
    free(value);

    kvdb_put(&db, key2, "CODELONG-SHORT-TEST-2");
    value = kvdb_get(&db, key2);
    printf("[%s]: [%s]\n", key2, value);
    free(value);
    return;
}/*}}}*/

void test2(){/*{{{*/
    char * val;
    val = kvdb_get(&db, key);
    printf("[%s]: [%s]\n", key, val);
    free(val);
    return;
}/*}}}*/

void test3(){/*{{{*/
    kvdb_put(&db, key2, val2);
    char * val;
    val = kvdb_get(&db, key2);
    printf("[%s]: [%s]\n", key2, val);
    free(val);
    return;
}/*}}}*/

void test4(){/*{{{*/
    char * val;
    val = kvdb_get(&db, key2);
    printf("[%s]: [%s]\n", key2, val);
    free(val);
    return;
}
/*}}}*/

void * thread_test_1(void * arg){/*{{{*/
    printf("******TEST THREAD MODIFY******\n");
    char * value;
    int i = 0;
    while(1){
        kvdb_put(&db, key_thread0, val_thread0[i%10]);
        value = kvdb_get(&db, key_thread0);
        printf("[%s]:[%s]\n", key_thread0, value);
        free(value);
        i = (i + 1) % 10;
    }
}/*}}}*/

void * thread_test_2(void * arg){/*{{{*/
    printf("******TEST THREAD READ******\n");
    char * value;
    while(1){
        value = kvdb_get(&db, key_thread0);
        printf("READ0[%s]:[%s]\n", key_thread0, value);
        free(value);
    }
}/*}}}*/

void * thread_test_3(void * arg){/*{{{*/
    printf("******TEST THREAD MODIFY******\n");
    char * value;
    int i = 0;
    while(1){
        kvdb_put(&db, key_thread1, val_thread1[i%10]);
        value = kvdb_get(&db, key_thread1);
        printf("[%s]:[%s]\n", key_thread1, value);
        free(value);
        i = (i + 1) % 10;
    }
}/*}}}*/

void * thread_test_4(void * arg){/*{{{*/
    printf("******TEST THREAD READ******\n");
    char * value;
    while(1){
        value = kvdb_get(&db, key_thread1);
        printf("READ1[%s]:[%s]\n", key_thread1, value);
        free(value);
    }
}/*}}}*/

void pthread_test_1(){
    pthread_t p_t1;
    pthread_t p_t2;
    pthread_t p_t3;
    pthread_t p_t4;
    pthread_create(&p_t1, NULL, thread_test_1, NULL);
    pthread_create(&p_t2, NULL, thread_test_2, NULL);
    pthread_create(&p_t3, NULL, thread_test_3, NULL);
    pthread_create(&p_t4, NULL, thread_test_4, NULL);
    pthread_join(p_t1, NULL);
}

int main(int argc, char * argv[])
{
    printf("******TSET DB******\n");
    kvdb_open(&db, db_path);

    if(argc >= 2){
        if(strcmp(argv[1], "3") == 0){
            test1();
        }
        else if(strcmp(argv[1], "p1") == 0){
            thread_test_1(NULL);
        }
        else if(strcmp(argv[1], "p2") == 0){
            thread_test_2(NULL);
        }
        else if(strcmp(argv[1], "p3") == 0){
            thread_test_3(NULL);
        }
        else if(strcmp(argv[1], "p4") == 0){
            thread_test_4(NULL);
        }
        else if(strcmp(argv[1], "pt") == 0){
            pthread_test_1();
        }
        else{
            test4();
        }
    }

    kvdb_close(&db);
    return 0;
}

