#include <mycmd.h>
int sp_cmd(const char *buf, char *cmd, char** pars){
   int i;
   int cmdcnt = 0;
   int parcnt = 0;
   int num = 0;
   for(i = 0; i < strlen(buf); ++i){
        if(buf[i] == ' ') break;
        if(buf[i] == '\n')
             return num;
        cmd[cmdcnt++] = buf[i];
   }
   i++;
   while(1){
       parcnt = 0;
       if(i == strlen(buf)) break;
       for(; i < strlen(buf); ++i){
         if(buf[i] == ' ') break;
         if(buf[i] == '\n')
             return num;
         pars[num][parcnt++] = buf[i];
       }
       if(i == strlen(buf)) break;
       i++;
       num++;
   }
   return num;
}

extern spinlock_t plock;
void shell_thread(void*args){
    intptr_t ttyid = (intptr_t)args;
    char buf[128];
    char name[128];
    memset(name,0,sizeof(name));
    sprintf(name,"/dev/tty%ld",ttyid);
    int stdin = vfs->open(name,O_RDONLY);
    int stdout = vfs->open(name,O_WRONLY);
    char curpath[128];
    memset(curpath,0,sizeof(curpath));
    sprintf(curpath,"~");

    ssize_t nread;
    while(1){
        memset(buf,0,sizeof(buf));
        sprintf(buf,"(%s) %s$ ",name,curpath);
        vfs->write(stdout,buf,strlen(buf));
        memset(buf,0,sizeof(buf));
        nread = vfs->read(stdin,buf,sizeof(buf));
        if(nread > 0){
            //seperate cmd
            char cmd[20];
            char *pars[25];
            memset(cmd,0,sizeof(cmd));
            memset(pars,0,sizeof(pars));
            for(int i = 0; i < 25; ++i)
                pars[i] = (char*)pmm->alloc(128);
            int num = sp_cmd(buf, cmd, pars);
#ifdef shell
        logw("num %d",num);
#endif
            for(int i = num + 1; i < 25; ++i){
                pmm->free(pars[i]);
                pars[i] = NULL;
            }

            memset(buf,0,sizeof(buf));
            //find the operation
            if(strcmp(cmd,"ls") == 0){
                cmd_ls(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"cd") == 0){
                cmd_cd(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"cat") == 0){
                cmd_cat(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"mkdir") == 0){
                cmd_mkdir(num,pars,curpath,buf);
            } 
            else if(strcmp(cmd,"rmdir") == 0){
                cmd_rmdir(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"rm") == 0 || strcmp(cmd,"unlink") == 0){
                cmd_rm(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"link") == 0){
                cmd_link(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"touch") == 0){
                cmd_touch(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"echo") == 0){
                cmd_echo(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"mnt") == 0){
                cmd_mnt(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"unmnt") == 0){
                cmd_unmnt(num,pars,curpath,buf);
            }
            else if(strcmp(cmd,"help") == 0){
                  char *temp = pmm->alloc(1024);
                  sprintf(temp,"Welcome to the jarvis's shell and the CMDs:\nls: list items in dir.\ncd: go into dir.\ncat: show the file on tty or into file.\nmkdir: create dir.\nrmdir: remove empty dir.\nrm: remove file.\nlink: link files.\nunlink: unlink files.\ntouch: create new file.\necho: print something on tty or into file.\nmnt: mount fs(devfs and procfs).\nunmnt: unmount fs(devfs and procfs).\n");
            vfs->write(stdout,temp,strlen(temp));
            pmm->free(temp);
            }
            else{
               sprintf(buf,"Unkown cmd %s\n",cmd);
            }           
            //write out
            vfs->write(stdout,buf,strlen(buf));
              
            for(int i = 0; i <= num; ++i)
                pmm->free(pars[i]); 

        }
    }

    return ;
}
