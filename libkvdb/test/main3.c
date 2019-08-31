#include "kvdb.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
int main() {
      kvdb_t db;
      const char *key = "THREAD-TEST0";
      char *value;
      char *nval[3]={
          "three1",
          "three2",
          "three3",
      };
      assert(kvdb_open(&db, "a.db") == 0); // BUG: should check for errors
      printf("open down\n");
      for(int i = 0; i < 0; ++i){
          printf("%d\n",i);

          assert(kvdb_put(&db, key,nval[i]) == 0);
      }
      value = kvdb_get(&db, key);
      printf("[%s]: [%s]\n", key, value);
      free(value);
      //kvdb_put(&db, key, (const char*)nval);
     // value = kvdb_get(&db, key);
     // printf("[%s]: [%s]\n", key, value);
      const char* key1 = "jarvis";
      //free(value);
      value = kvdb_get(&db,key1);
      printf("[%s]: [%s]\n", key1, value);
      assert(kvdb_close(&db) == 0);
      free(value);
      return 0;
                        
}
