#include "kvdb.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

pthread_mutex_t openlock = PTHREAD_MUTEX_INITIALIZER;
static int DLV[18] = {
    0,(1<<6),(1<<8),(1<<10),(1<<11),(1<<12),(1<<14),(1<<15),(1<<16),(1<<17),(1<<18),(1<<19),(1<<20),(1<<21),(1<<22),(1<<23),(1<<24),((1<<24)+16)
};

int read_key(int fd,char* key, ssize_t size){
     char c;
     int cnt = 0;
     while(read(fd,&c,1) > 0 && c != 0x0b && c != 0x00){
         key[cnt++] = c;
     }
     key[cnt]='\0';
     return cnt;
}

int read_val(int fd,char* val, ssize_t size){
     char c;
     int cnt = 0;
     while(read(fd,&c,1) > 0 && c != 0x0a && c != 0x00){
        val[cnt++] = c;
     }
     val[cnt] = '\0';
     return cnt;
}

void journal_write(kvdb_t *db,const char*key, const char*value, uint8_t lev){
     char c[1] = {0x0b};
     write(db->logfd,key,strlen(key));
     write(db->logfd,c,1);
     write(db->logfd,&lev,1);
     write(db->logfd,value,strlen(value));
     c[0] = 0x0a;
     write(db->logfd,c,1);
     fsync(db->logfd);
     c[0] = 0x0f;
     write(db->logfd,c,1);
     fsync(db->logfd);
}


int rec_put(kvdb_t *db, const char*key, const char*value, uint8_t level){

     struct stat buf;
     int che = fstat(db->fd,&buf);
     if(che == -1){
          printf("The db is free!\n");
          return -1;
     }

     if((buf.st_mode & 0700) != 0600){
         printf("%o\n",buf.st_mode &0700); 
         printf("rec put permission denied!\n");
         return -1;
     }
     
     //data_write
     
     if(buf.st_size <= DATA){//no data
        lseek(db->fd,DATA,SEEK_SET);

        write(db->fd,key,strlen(key));
        char c[1] = {0x0b};
        write(db->fd,c,1);
        lseek(db->fd,DATA+MDLK,SEEK_SET);
        
        write(db->fd,&level,1);
       
        write(db->fd,value,strlen(value));
        c[0] = 0x0a;
        write(db->fd,c,1);
        
        lseek(db->fd,DLV[level]-strlen(value)-2,SEEK_CUR);
        write(db->fd,"\0",1);

     }
     else{//there already data
         lseek(db->fd,DATA,SEEK_SET);
         char c[1];
         int flag = 0;
         while(1){
            char *oldkey = (char*)malloc(MDLK);
            uint8_t oldlevel;
            if(oldkey == NULL){
                printf("malloc wrong!\n");
                exit(0);
                // return -1;
            }
            
            read(db->fd,&oldlevel,1); 
            if(oldlevel == 0xe5){//already invalid
                lseek(db->fd,MDLK-1,SEEK_CUR);
                read(db->fd,&oldlevel,1);
                lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
                continue;

            }
            
            lseek(db->fd,-1,SEEK_CUR);
            if(read_key(db->fd,oldkey,MDLK) == 0){
                lseek(db->fd,-1,SEEK_CUR);
                write(db->fd,"\0",1);
                break;
            }
            if(strcmp(oldkey,key) == 0){//the same 
                lseek(db->fd,MDLK-strlen(key)-1,SEEK_CUR);
                read(db->fd,&oldlevel,1);
               
                 if(oldlevel >= level){
                   // read has moven the offset
                    write(db->fd,value,strlen(value));
                    c[0] = 0x0a;
                    write(db->fd,c,1);
                    lseek(db->fd,DLV[oldlevel]-strlen(value)-1,SEEK_CUR);
                    flag = 1;
                    break;
                }
                else{
                    //key delete
                    c[0] = 0xe5;
                    lseek(db->fd,-1,SEEK_CUR);
                    lseek(db->fd,-MDLK,SEEK_CUR);
                    write(db->fd,c,1);
                    lseek(db->fd,MDLK-1,SEEK_CUR);
                    lseek(db->fd,DLV[oldlevel],SEEK_CUR);
                    free(oldkey);
                    continue;
               }
            }
            lseek(db->fd,MDLK-strlen(oldkey)-1,SEEK_CUR);
            read(db->fd,&oldlevel,1);
            
            free(oldkey);
            //   printf("%d\n",(int)strlen(oldkey));
            lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
         }
         if(!flag){
            lseek(db->fd,-1,SEEK_CUR);
            write(db->fd,key,strlen(key));
            c[0] = 0x0b;
            write(db->fd,c,1);
            lseek(db->fd,MDLK-strlen(key)-1,SEEK_CUR);

            write(db->fd,&level,1);
            write(db->fd,value,strlen(value));
            c[0] = 0x0a;
            write(db->fd,c,1);
            lseek(db->fd,DLV[level]-strlen(value)-2,SEEK_CUR);
            write(db->fd,"\0",1);
         }

     }
   
     fsync(db->fd);
     return 0;

}

void recovery(kvdb_t *db){
    lseek(db->logfd,0,SEEK_SET);
    while(1){
        char* key = (char*)malloc(MDLK);
        uint8_t lev;
        char* val;
        uint8_t check;
        if(read_key(db->logfd,key,MDLK) == 0){
            free(key);
            break;
        }
        if(read(db->logfd,&lev,1) == 0)
            break;
        val = (char*)malloc(DLV[lev]);
        if(read_val(db->logfd,val,DLV[lev])== 0){
            free(val);
            break;
        }
        read(db->logfd,&check,1); 
        if(check == 0x0f){
           rec_put(db,key,val,lev); 
           free(key);
           free(val);
        }
        else{
            free(key);
            free(val);
            break;
        }

    }
    
    return;
}


int kvdb_open(kvdb_t *db, const char *filename){
     //根据日志恢复文件,保存open操作,储存信息到db中
     pthread_mutex_lock(&openlock);
     struct stat buf2; 
     int che = fstat(db->fd,&buf2);
     if(che != -1 && db->fd != 0 && db->fd != 1 && db->fd != 2){
     //different threads use the same db open the same file
         printf("repeatly open the same file \n");
         pthread_mutex_unlock(&openlock);
         return 0;
     }

     int fd;
     fd = open(filename,O_CREAT|O_RDWR,0666);
     if(fd == -1){
         printf("open file failed!\n");
         pthread_mutex_unlock(&openlock);
         return -1;
     }

     pthread_mutex_init(&db->lock,NULL);
     pthread_mutex_lock(&db->lock);

     int check = flock(fd,LOCK_EX);
     assert(check == 0);
     //save db    
     db->fd = fd;
     
     char *logname = (char*)malloc(strlen(filename)+6);
     strcpy(logname,filename);
     strcat(logname,".log");
     //printf("%s\n",logname);
     
     //recover
     db->logfd = open(logname,O_CREAT|O_RDWR,0666); 
     assert(flock(db->logfd,LOCK_EX) == 0);
     
     struct stat logbuf;
     fstat(db->logfd,&logbuf);;
     if(logbuf.st_size != 0){
         recovery(db);    
     }

     ftruncate(db->logfd,0);
     lseek(db->logfd,0,SEEK_SET);
     assert(flock(db->logfd,LOCK_UN) == 0);

     
     //close(db->logfd);
     //db->logfd = -1;
     //journal_reopen
    //db->logfd = open(logname,O_RDWR|O_TRUNC,0666);
    //lseek(db->logfd,0,SEEK_SET);
     
     check = flock(db->fd,LOCK_UN);
     assert(check == 0);

     pthread_mutex_unlock(&db->lock);
     pthread_mutex_unlock(&openlock);
     return 0;
}

int kvdb_close(kvdb_t *db){
     pthread_mutex_lock(&db->lock);

     if(db->fd == -1){
         printf("already close the db!\n");
         pthread_mutex_unlock(&db->lock);
         return -1;
     }

    
   //  int check = flock(db->fd,LOCK_UN);
   //  assert(check == 0);
     
     int ret = close(db->fd);
     db->fd = -1;

   //  check = flock(db->logfd,LOCK_UN);
   //  assert(check == 0);
     close(db->logfd);
     db->logfd = -1;

     pthread_mutex_unlock(&db->lock);
     
     return ret;
}


int kvdb_put(kvdb_t *db, const char *key, const char* value){
     pthread_mutex_lock(&db->lock);

     struct stat buf;
     int che = fstat(db->fd,&buf);
     if(che == -1){
          printf("The db is free!\n");
          pthread_mutex_unlock(&db->lock);
          return -1;
     }

     assert(flock(db->fd,LOCK_EX) == 0);
     if((buf.st_mode & 0700) != 0600){
         printf("permission: %o\n",buf.st_mode &0700); 
         printf("put permission denied!\n");
         assert(flock(db->fd,LOCK_UN) == 0);
         pthread_mutex_unlock(&db->lock);
         return -1;
     }

     if(strlen(key) >= MDLK || strlen(value) > MDLV){
         printf("Your key or value is out of size!\n");
         assert(flock(db->fd,LOCK_UN) == 0);
         pthread_mutex_unlock(&db->lock);
         return -1;
     }
    
     uint8_t level = 1;
     while(strlen(value)+2 > DLV[level]) level++;
     
     //journal write

     assert(flock(db->logfd,LOCK_EX) == 0);
     
     journal_write(db,key,value,level);

     fsync(db->logfd);
     
     assert(flock(db->logfd,LOCK_UN) == 0);

     //data_write

     
     if(buf.st_size <= DATA){//no data
        lseek(db->fd,DATA,SEEK_SET);

        write(db->fd,key,strlen(key));
        char c[1] = {0x0b};
        write(db->fd,c,1);
        lseek(db->fd,DATA+MDLK,SEEK_SET);
        
        write(db->fd,&level,1);
         
        write(db->fd,value,strlen(value));
        c[0] = 0x0a;
        write(db->fd,c,1);
        
        lseek(db->fd,DLV[level]-strlen(value)-2,SEEK_CUR);
        write(db->fd,"\0",1);

     }
     else{//there already data
         lseek(db->fd,DATA,SEEK_SET);
         char c[1];
         int flag = 0;
         while(1){
            char *oldkey = (char*)malloc(MDLK);
            uint8_t oldlevel;
            if(oldkey == NULL){
                printf("malloc wrong!\n");
                pthread_mutex_unlock(&db->lock);
               exit(0);//crash
                // return -1;
            }
            
            read(db->fd,&oldlevel,1); 
            if(oldlevel == 0xe5){//already invalid
                lseek(db->fd,MDLK-1,SEEK_CUR);
                read(db->fd,&oldlevel,1);
                lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
                continue;

            }
            
            lseek(db->fd,-1,SEEK_CUR);
            if(read_key(db->fd,oldkey,MDLK) == 0){
                lseek(db->fd,-1,SEEK_CUR);
                write(db->fd,"\0",1);
                break;
            }
            if(strcmp(oldkey,key) == 0){//the same 
                lseek(db->fd,MDLK-strlen(key)-1,SEEK_CUR);
                read(db->fd,&oldlevel,1);
                
                if(oldlevel >= level){
                   // read has moven the offset
                    write(db->fd,value,strlen(value));
                    c[0] = 0x0a;
                    write(db->fd,c,1);
                    lseek(db->fd,DLV[oldlevel]-strlen(value)-1,SEEK_CUR);
                    flag = 1;
                    break;
                }
                else{
                    //key delete
                    c[0] = 0xe5;
                    lseek(db->fd,-1,SEEK_CUR);
                    lseek(db->fd,-MDLK,SEEK_CUR);
                    write(db->fd,c,1);
                    lseek(db->fd,MDLK-1,SEEK_CUR);
                    lseek(db->fd,DLV[oldlevel],SEEK_CUR);
                    free(oldkey);
                    continue;
               }
            }
            lseek(db->fd,MDLK-strlen(oldkey)-1,SEEK_CUR);
            read(db->fd,&oldlevel,1);
            
            free(oldkey);
            //   printf("%d\n",(int)strlen(oldkey));
            lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
         }
         if(!flag){
            lseek(db->fd,-1,SEEK_CUR);
            write(db->fd,key,strlen(key));
            c[0] = 0x0b;
            write(db->fd,c,1);
            lseek(db->fd,MDLK-strlen(key)-1,SEEK_CUR);

            write(db->fd,&level,1);
            write(db->fd,value,strlen(value));
            c[0] = 0x0a;
            write(db->fd,c,1);
            lseek(db->fd,DLV[level]-strlen(value)-2,SEEK_CUR);
            write(db->fd,"\0",1);
         }

     }
   
     fsync(db->fd);
     assert(flock(db->fd,LOCK_UN) == 0);
     pthread_mutex_unlock(&db->lock);
     return 0;

}



char *kvdb_get(kvdb_t *db, const char *key){
     
     pthread_mutex_lock(&db->lock);

     struct stat buf;
     int che = fstat(db->fd,&buf);
     if(che == -1){
          printf("The db is free!\n");
          pthread_mutex_unlock(&db->lock);
          return NULL;
     }

     assert(flock(db->fd,LOCK_EX) == 0);
     
     if((buf.st_mode & 0700) != 0600){
         printf("permission: %o\n",buf.st_mode &0700); 
         printf("fd %d get permission denied!\n",db->fd);
         assert(flock(db->fd,LOCK_UN) == 0);
         pthread_mutex_unlock(&db->lock);
         return NULL;
     }

     char* ret = (char*) malloc(MDLV);
     if(ret == NULL){
         printf("malloc wrong!\n");
         pthread_mutex_unlock(&db->lock);
        exit(0);
         // return NULL;
     }

     lseek(db->fd,DATA,SEEK_SET);

     while(1){
         char* oldkey =(char*) malloc(MDLK);
         uint8_t oldlevel;
         if(oldkey == NULL){
            
            assert(flock(db->fd,LOCK_UN) == 0);
            pthread_mutex_unlock(&db->lock);
            return NULL;
         } 
         
         read(db->fd,&oldlevel,1);
         if(oldlevel == 0xe5){//already delete
              lseek(db->fd,MDLK-1,SEEK_CUR);
              read(db->fd,&oldlevel,1);
              lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
              continue;
         }

         lseek(db->fd,-1,SEEK_CUR);
         if(read_key(db->fd,oldkey,MDLK) == 0) break;
        
         if(strcmp(oldkey,key) == 0){
            lseek(db->fd,MDLK-strlen(key)-1,SEEK_CUR); 
            read(db->fd,&oldlevel,1);
            if(read_val(db->fd,ret,DLV[oldlevel]) == 0){
               
               assert(flock(db->fd,LOCK_UN) == 0);
               pthread_mutex_unlock(&db->lock);
               return NULL;
            } 
  
            assert(flock(db->fd,LOCK_UN) == 0);
            pthread_mutex_unlock(&db->lock);
            return ret;
         }
          
         lseek(db->fd,MDLK-strlen(oldkey)-1,SEEK_CUR);
         read(db->fd,&oldlevel,1);
         lseek(db->fd,DLV[oldlevel]-1,SEEK_CUR);
         free(oldkey);
     }

     assert(flock(db->fd,LOCK_UN) == 0);
     pthread_mutex_unlock(&db->lock);

     return NULL;
}
