#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
//#define  DEBUG
//#define LOCAL
//#define VIPIC
#define BASECLU 3

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

void *fhead, *fend, *FAT1, *FAT2, *dhead;
void *threecu;
int *SPF,*ROOT;
short *BPS,*RS;
uint8_t *SPC,*NOF;
void *mindex;

extern char **environ;
int main(int argc, char *argv[]) {

#ifdef DEBUG
    int fd = open("./fs.img",O_RDONLY);
#endif
#ifndef DEBUG
    int fd = open(argv[1],O_RDONLY);
#endif
    struct stat statbuf;
#ifdef DEBUG
    stat("./fs.img",&statbuf);
#endif
#ifndef DEBUG
    stat(argv[1],&statbuf);
#endif
    uint32_t fsize = statbuf.st_size;
    fhead = mmap(NULL,fsize,PROT_READ,MAP_PRIVATE,fd,0); 
    fend = fhead + fsize; 
    BPS = (short*)(fhead + 0xB); 
    SPC = (uint8_t*)(fhead + 0xD);
    RS = (short*)(fhead + 0xE);
    NOF = (uint8_t*)(fhead + 0x10);
    SPF = (int*)(fhead + 0x24);
    ROOT = (int*)(fhead + 0x2c);
    
#ifdef DEBUG
   printf("%x %x %x %x %x %x\n",*BPS,*SPC,*RS,*NOF,*SPF,*ROOT);
   printf("%x\n",fsize);
   printf("%lu\n",(unsigned long)fend);
#endif

    FAT1 = fhead + (*RS) * (*BPS);
    FAT2 = FAT1 + (*SPF) * (*BPS);
    dhead = FAT2 + (*SPF) * (*BPS);
    threecu = dhead + (*BPS);

#ifdef DEBUG 
    printf("%x\n",*(short*)threecu);
    printf("%lu\n",(unsigned long)threecu);
#endif
   

    mindex = threecu;
    while((unsigned long)mindex <= (unsigned long)fend){
        if(*(short*)mindex == 0x202e || *(short*)mindex == 0x2e2e){
            mindex = mindex + 0x20;
            continue;
        }               

        void *exname = mindex + 0x8;
        if(*(short*)exname == 0x4d42 && *(uint8_t*)(exname + 0x2) == 0x50){//the short name of the filehead
            char longname[50];
            int cnt = 0;
            memset(longname,0,sizeof(longname));
            int flag = 0;

            void *lncheck = (exname + 0x3) - 0x20;
           
            while(*(uint8_t*)lncheck == 0x0f){
                void *ln = lncheck - 0xA;

                for(int i = 0; i < 5; ++i){//the part one of the longname
                    if(*(short*)(ln+i*2) == 0x0000){
                        flag = 1;
                        break;
                    } 
                    longname[cnt++] = *(char*)(ln+i*2);
                }
                if(flag) break;

                ln = ln + 0xd;
                for(int i = 0; i < 6; ++i){//the part two of the longname
                    if(*(short*)(ln+i*2) == 0x0000){
                        flag = 1;
                        break;
                    } 
                    longname[cnt++] = *(char*)(ln+i*2);
                }
                if(flag) break;

                ln = ln + 0x2 + 0xc;
                for(int i = 0; i < 2; ++i){//the part three of the longname
                    if(*(short*)(ln+i*2) == 0x0000){
                        flag = 1;
                        break;
                    }
                    longname[cnt++] = *(char*)(ln+i*2);
                }
                if(flag) break;
                
                lncheck = lncheck - 0x20;
            }

#ifdef DEBUG
            //printf("%d\n",cnt);
            printf("%s\n",longname);
#endif
            //get the cluster of the file
            void *hclus = exname + 0x10 - 0x4;
            void *lclus = exname + 0x10 + 0x2;
            short hc = *(short*)hclus;
            short lc = *(short*)lclus;
            uint32_t clusnum = ((uint32_t)(hc) << 16) + (uint32_t)(lc & 0x0000ffff);//pay attention to the type changing
#ifdef DEBUG
            printf("%x\n",hc);
            printf("%x\n",lc);
            printf("%x\n",clusnum);
#endif
            
            char tailcheck[5];
            memset(tailcheck,0,sizeof(tailcheck));
            strncpy(tailcheck,longname+cnt-4,4);
#ifdef DEBUG
            printf("%s\n",tailcheck);
#endif

            if(strcmp(tailcheck,".bmp") == 0){//the file name is right
                if(clusnum != 0){//not be deleted
                    void *phead = (clusnum - BASECLU)*(*BPS) + threecu;
                    assert(*(short*)phead == 0x4d42);
                    uint32_t psize = *(uint32_t*)(phead + 0x2);
#ifdef DEBUG
                    printf("%d\n",psize);
#endif

#ifdef VIPIC
                    char dir[50];
                    memset(dir,0,sizeof(dir));
#ifdef LOCAL
                    strcpy(dir,"./DCIM/");
                    strcat(dir,longname);
#endif

#ifndef LOCAL
                    strcpy(dir,longname);
#endif

#ifdef DEBUG
                    printf("%s\n",dir);
#endif
                    FILE* fp = fopen(dir,"w+b");
                    
                    fwrite(phead,1,psize,fp);

                    fclose(fp);
                    //pipe
                    int fildes[2];
                    int pid;
                    char* nargv[5];
                    memset(nargv,0,sizeof(nargv));

                    if(pipe(fildes) != 0){ perror("pipe"); exit(1); }
                    pid  = fork();
                    if(pid < 0){ perror("fork"); exit(1); }
                    if(pid == 0){//sha1sum
                      // void *buff = malloc(psize);
                      // fread(buff,1,psize,fp);
                     
                       dup2(fildes[1],STDERR_FILENO);
                       close(fildes[0]);
                       nargv[0] = (char*)malloc(10);
                       strcpy(nargv[0],"sha1sum");
                       nargv[1] = (char*)malloc(100);
                       memcpy(nargv[1],dir,strlen(dir) + 1);
                       execve("/usr/bin/sha1sum",nargv,environ);

                       close(fildes[1]);

                    }
                    else{
                       dup2(fildes[0],STDIN_FILENO);
                       close(fildes[1]);
                       char keys[200];
                       memset(keys,0,sizeof(keys));
                       int keycnt = 0;
                       char c;
                       while(read(fildes[0],&c,1) > 0){
                            keys[keycnt++] = c;
                       }
                       printf("%s\n",keys);

                    }
#endif

#ifndef VIPIC
                   int fil1[2],fil2[2];
                   int pid;
                   char *nargv[5];
                   memset(nargv,0,sizeof(nargv));
                   
                   if(pipe(fil1) != 0){ perror("pipe1"); exit(1); }
                   if(pipe(fil2) != 0){ perror("pipe2"); exit(1); }
                   pid = fork();
                   if(pid < 0){ perror("fork"); exit(1); }
                   if(pid == 0){
                       dup2(fil1[0],STDIN_FILENO);
                       close(fil1[1]);
                       dup2(fil2[1],STDOUT_FILENO);
                       close(fil2[0]);
                       nargv[0] = (char*)malloc(10);
                       strcpy(nargv[0],"sha1sum");
                       nargv[1] = NULL;
                       execve("/usr/bin/sha1sum",nargv,environ);
                       
                       close(fil1[0]);
                       close(fil2[1]);
                   }
                   else{
                      // dup2(fil1[1],STDERR_FILENO);
                       close(fil1[0]);
                       write(fil1[1],phead,psize);
                       close(fil1[1]);
                       wait(NULL);
                       dup2(fil2[0],STDIN_FILENO);
                       close(fil2[1]);
                       
                       char keys[200];
                       memset(keys,0,sizeof(keys));
                       int keycnt = 0;
                       char c;
                       while(read(fil2[0],&c,1) > 0){
                           if(c == '-') break;
                           keys[keycnt++] = c;
                       }

                       printf("%s\t%s\n",keys,longname);
                       close(fil1[1]);
                       close(fil2[0]);
                   
                   }
#endif

                }  
            }
        }
#ifdef DEBUG
        if((unsigned long)mindex == (unsigned long)(threecu + 0x80)) break;
#endif
        mindex = mindex + 0x20;
        
    } 

    assert(munmap(fhead,fsize) == 0);

    return 0;
}
