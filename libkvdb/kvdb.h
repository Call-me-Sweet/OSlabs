#ifndef __KVDB_H__
#define __KVDB_H__

#include <pthread.h>
#include <inttypes.h>
 
#define DEBUG
#define DATA 0x0//0x100000
#define MDLK 130//128+32
#define MDLV (16*(1<<20))//1<<24
/*
 * for key delete 0xe5 first B
 * end of 0x0b
 * for value delete  
 * or not level 0x01,0x02,0x03,0x04,0x05 first B
 * end of 0x0a 
 *
 */

struct kvdb {
    pthread_mutex_t lock;
    int logfd;
    int fd;
};
typedef struct kvdb kvdb_t;

int kvdb_open(kvdb_t *db, const char *filename);
int kvdb_close(kvdb_t *db);
int kvdb_put(kvdb_t *db, const char *key, const char *value);
char *kvdb_get(kvdb_t *db, const char *key);

#endif
