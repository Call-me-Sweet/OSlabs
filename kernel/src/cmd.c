#include <mycmd.h>
extern struct Proc hgproc;
const char devdir[] = "~/dev: ramdisk0   ramdisk1   tty1   tty2   tty3   tty4\n";
void cmd_ls(int num, char **pars, char* curpath, char *buf){
    for(int i = 0; i <= num; ++i){
        char path[128];
        memset(path,0,sizeof(path));
        if(pars[i][0] == '/'){//绝对路径
           sprintf(path,"%s",pars[i]);
        }
        else{//相对路径
            if(pars[i][0] == '\0'){//当前目录
               if(i != 0) continue;//防止最后空格影响
               if(strcmp(curpath,"~") == 0)
                   sprintf(path,"/");
               else
                   sprintf(path,"%s",curpath+1);
            }
            else if(strcmp(pars[i],"..") == 0 || strcmp(pars[i],"../") == 0){
                  if(strcmp(curpath,"~") == 0)
                      sprintf(path,"/");
                  else{
                    //找父亲路径
                    int ind = strlen(curpath)-1;
                    while(curpath[ind]!='/') ind--;
                    memcpy(path,curpath+1,ind-1);
                    if(path[0] == '\0')
                        sprintf(path,"/");
                  }
            }
            else
               sprintf(path,"%s/%s",curpath+1,pars[i]);
        }
#ifdef shell
        my_spinlock(&plock);
        logy("path %s", path);
        my_spinunlock(&plock);
#endif
        int flags = O_DIR|O_CHECK;
        int fd = vfs->open(path,flags);
        if(fd == -1){
            sprintf(buf,"The path is not a dir.\n");
            continue; 
        }
        inode_t *curnode = cur_task->fildes[fd]->inode;
        assert(vfs->close(fd) == 0);

        if(curnode == NULL){//dev
           if(strlen(buf) + strlen(devdir) > 128)
               return;
           sprintf(buf+strlen(buf),"%s",devdir);
        }
        else{
            ed_t *rec = (ed_t*)pmm->alloc(sizeof(ed_t));
            device_t *curdev = curnode->fs->dev;
            if(curdev == NULL){//proc
                if(pars[i][0] == '/'){//绝对路径
                    if(strcmp(path,"/") == 0)
                         sprintf(buf+strlen(buf),"~:");
                    else
                         sprintf(buf+strlen(buf),"~%s:",path);
                }
                else{
                     if(pars[i][0] == '\0'){
                         if(strcmp(path,"/") == 0)
                              sprintf(buf+strlen(buf),"~:");
                         else
                              sprintf(buf+strlen(buf),"~%s:",path);
                     }
                     else
                         sprintf(buf+strlen(buf),"~%s:",path);
                }
               
                ed_t *nrec = (ed_t*)hgproc.block[0];
                for(int pp = 0; pp < NFILEINFS; ++pp){
                   if(nrec->etrs[pp].valid == 1){
                         sprintf(buf+strlen(buf),"    %s",nrec->etrs[pp].name);
                   }
                }
                pmm->free(rec);
                sprintf(buf+strlen(buf),"\n");
                return;
            }
            else{
                curdev->ops->read(curdev,curnode->fs->fsoff+curnode->fs->blooff+curnode->nblocks*FILEBLOCK,(void*)rec,sizeof(ed_t));
               
                if(strlen(buf)+ strlen(path) +  1 > 128){
                    pmm->free(rec);
                    return;
                }
                if(pars[i][0] == '/'){//绝对路径
                    if(strcmp(path,"/") == 0)
                         sprintf(buf+strlen(buf),"~:");
                    else
                         sprintf(buf+strlen(buf),"~%s:",path);
                }
                else{
                     if(pars[i][0] == '\0'){
                         if(strcmp(path,"/") == 0)
                              sprintf(buf+strlen(buf),"~:");
                         else
                              sprintf(buf+strlen(buf),"~%s:",path);
                     }
                     else
                         sprintf(buf+strlen(buf),"~%s:",path);
                }

                for(int j = 0; j < NFILEINFS; ++j){
                    if(rec->etrs[j].valid == 1){
                        if(strlen(buf) + strlen(rec->etrs[j].name) + 3 > 127){
                            pmm->free(rec);
                            return;
                        }
                        sprintf(buf+strlen(buf),"   %s",rec->etrs[j].name);
                    }
                }
                sprintf(buf+strlen(buf),"\n");
            }
        pmm->free(rec); 
        }
    }//for
}


void cmd_cd(int num, char **pars, char* curpath, char *buf){
     //只对参数1有效,pars[0];
    char path[128];
    memset(path,0,sizeof(path));
    if(pars[0][0] == '/'){//绝对路径
       sprintf(path,"%s",pars[0]);
    }
    else{//相对路径
       if(pars[0][0] == '\0')//回到根目录
           sprintf(path,"/");
       else if(strcmp(pars[0],"..") == 0 || strcmp(pars[0],"../") == 0){
             if(strcmp(curpath,"~") == 0)
                 sprintf(path,"/");
             else{
               //找父亲路径
               int ind = strlen(curpath)-1;
               while(curpath[ind]!='/') ind--;
               memcpy(path,curpath+1,ind-1);
               if(path[0] == '\0')
                   sprintf(path,"/");
             }
       }
       else
         sprintf(path,"%s/%s",curpath+1,pars[0]);
    }
#ifdef shell
    my_spinlock(&plock);
    logw("path %s", path);
    my_spinunlock(&plock);
#endif
    if(vfs->access(path,DIR_OK) == 0){
          memset(curpath, 0, strlen(curpath));
          if(strcmp(path,"/") == 0)
             sprintf(curpath,"~");
          else if(pars[0][0] == '/')
             sprintf(curpath,"~%s",path);
          else
             sprintf(curpath,"~%s",path);
          return;
    }
    else{
        sprintf(buf,"No such a directory \"%s\".\n",path);
        return;
    }
}


void cmd_cat(int num, char **pars, char* curpath, char *buf){
        int i;
        for(i = 0; i <= num; ++i){
            if(strcmp(pars[i],">") == 0) break;
        }
        
        if(i == num+1){//no >
             char path[128];
             memset(path,0,sizeof(path));
             if(pars[0][0] == '/'){//绝对路径
                sprintf(path,"%s",pars[0]);
             }
             else{//相对路径
                 if(pars[0][0] == '\0'){//当前目录
                    if(strcmp(curpath,"~") == 0)
                        sprintf(path,"/");
                    else
                        sprintf(path,"%s",curpath+1);
                 }
                 else
                    sprintf(path,"%s/%s",curpath+1,pars[0]);
             }
     #ifdef shell
             my_spinlock(&plock);
             logy("path %s", path);
             my_spinunlock(&plock);
     #endif
           
             int flags = O_CHECK;
             int fd = vfs->open(path,flags);
             if(fd == -1){
                 sprintf(buf,"The file is not exist.\n");
                 return;
             }
             inode_t *curnode = cur_task->fildes[fd]->inode;
             assert(vfs->close(fd) == 0);
          
             if(curnode == NULL){
                 char name[128];
                 int cnt = 0, pp;
                 memset(name,0,sizeof(name));
                 for(pp = strlen(path)-1; pp >= 0; pp-- )
                     if(path[pp] == '/') break;
                 pp++;
                 for(;pp < strlen(path); ++pp)
                     name[cnt++] = path[pp];
                 device_t *dev = dev_lookup(name);
                 dev->ops->read(dev,0,buf,127);
                 return;
             }
             else{
                 device_t *curdev = curnode->fs->dev;
                 if(curdev == NULL){
                      char name[128];
                      int cnt = 0, pp;
                      memset(name,0,sizeof(name));
                      for(pp = strlen(path)-1; pp >= 0; pp-- )
                          if(path[pp] == '/') break;
                      for(;pp < strlen(path); ++pp)
                          name[cnt++] = path[pp];
                      memcpy(buf,hgproc.block[curnode->nblocks],127);
                      buf[strlen(buf)] = '\n';
                      return;
                 }
                 else{
                   curdev->ops->read(curdev,curnode->fs->fsoff+curnode->fs->blooff+curnode->nblocks*FILEBLOCK,(void*)buf,127);
     #ifdef shell 
             my_spinlock(&plock);
             logb("buf %s",(char*)buf);
             my_spinunlock(&plock);
     #endif
                   buf[strlen(buf)] = '\n';
                   return;
                 }
             }
        }
        else if(i == num){
            sprintf(buf,"The format of cmd is Invalid.\n");
            return;
        }
        else{
           int tonum = i + 1;
           int fromnum;
           for(int pp = 0; pp < 3; ++pp){
               if(pp == i || pp == i + 1) continue;
               fromnum = pp;
           }
           char tofile[128],fromfile[128];
           memset(tofile,0,sizeof(128)),memset(fromfile,0,sizeof(128));
           if(pars[tonum][0] == '/'){//绝对路径
              sprintf(tofile,"%s",pars[tonum]);
           }
           else{//相对路径
               if(pars[tonum][0] == '\0'){//当前目录
                  if(strcmp(curpath,"~") == 0)
                      sprintf(tofile,"/");
                  else
                      sprintf(tofile,"%s",curpath+1);
               }
               else
                  sprintf(tofile,"%s/%s",curpath+1,pars[tonum]);
           }
           if(pars[fromnum][0] == '/'){//绝对路径
              sprintf(fromfile,"%s",pars[fromnum]);
           }
           else{//相对路径
               if(pars[fromnum][0] == '\0'){//当前目录
                  if(strcmp(curpath,"~") == 0)
                      sprintf(fromfile,"/");
                  else
                      sprintf(fromfile,"%s",curpath+1);
               }
               else
                  sprintf(fromfile,"%s/%s",curpath+1,pars[fromnum]);
           }
   #ifdef shell
           my_spinlock(&plock);
           logy("tofile %s", tofile);
           logy("fromfile %s", fromfile);
           my_spinunlock(&plock);
   #endif
           int flags = O_CHECK;
           int fd1 = vfs->open(fromfile,flags);
           if(fd1 == -1){
               sprintf(buf,"The file is not exist.\n");
               return;
           }
           inode_t *curnode1 = cur_task->fildes[fd1]->inode;
           
           flags = O_CREAT|O_CHECK;
           
           int fd2 = vfs->open(tofile,flags);
           if(fd2 == -1){
               sprintf(buf,"open file %s fail.\n",tofile);
               assert(vfs->close(fd1) == 0);
               return;
           }
           inode_t *curnode2 = cur_task->fildes[fd2]->inode;
           
           if(curnode1 == NULL || curnode2 == NULL){
               sprintf(buf,"Permission denied.\n");
               assert(vfs->close(fd1) == 0);
               assert(vfs->close(fd2) == 0);
               return;
           }
           else{
               device_t *curdev1 = curnode1->fs->dev;
               device_t *curdev2 = curnode2->fs->dev;
               if(curdev2 == NULL){
                   sprintf(buf,"Permission denied.\n");
                   assert(vfs->close(fd1) == 0);
                   assert(vfs->close(fd2) == 0);
                   return;
               }
               if(curdev1 == NULL){
                    char *temp = pmm->alloc(FILEBLOCK);
                    memcpy(temp,hgproc.block[curnode1->nblocks],FILEBLOCK);
                    curdev2->ops->write(curdev2,curnode2->fs->fsoff+curnode2->fs->blooff+curnode2->nblocks*FILEBLOCK,(void*)temp,FILEBLOCK); 
                    assert(vfs->close(fd1) == 0);
                    assert(vfs->close(fd2) == 0);
                    pmm->free(temp);
                    return;
               }
               else{
                 char *temp = pmm->alloc(FILEBLOCK);
                 curdev1->ops->read(curdev1,curnode1->fs->fsoff+curnode1->fs->blooff+curnode1->nblocks*FILEBLOCK,(void*)temp,FILEBLOCK);
                 curdev2->ops->write(curdev2,curnode2->fs->fsoff+curnode2->fs->blooff+curnode2->nblocks*FILEBLOCK,(void*)temp,FILEBLOCK);
                 assert(vfs->close(fd1) == 0);
                 assert(vfs->close(fd2) == 0);
                 pmm->free(temp);
                 return;
               }
           }
        }
}


void cmd_mkdir(int num, char **pars, char* curpath, char *buf){
    for(int i = 0; i <= num; ++i){
        char path[128];
        memset(path,0,sizeof(path));
        if(pars[i][0] == '/'){//绝对路径
           sprintf(path,"%s",pars[i]);
        }
        else{//相对路径
            if(pars[i][0] == '\0'){//当前目录
               if(i != 0) continue;//防止最后空格影响
               if(strcmp(curpath,"~") == 0)
                   sprintf(path,"/");
               else
                   sprintf(path,"%s",curpath+1);
            }
            else
               sprintf(path,"%s/%s",curpath+1,pars[i]);
        }
#ifdef shell
        my_spinlock(&plock);
        logy("path %s", path);
        my_spinunlock(&plock);
#endif
        if(vfs->mkdir(path) != 0){
            sprintf(buf,"Permisiion denied.\n");
        }
   }
}

void cmd_rmdir(int num, char **pars, char* curpath, char *buf){
    for(int i = 0; i <= num; ++i){
        char path[128];
        memset(path,0,sizeof(path));
        if(pars[i][0] == '/'){//绝对路径
           sprintf(path,"%s",pars[i]);
        }
        else{//相对路径
            if(pars[i][0] == '\0'){//当前目录
               if(i != 0) continue;//防止最后空格影响
               if(strcmp(curpath,"~") == 0)
                   sprintf(path,"/");
               else
                   sprintf(path,"%s",curpath+1);
            }
            else
               sprintf(path,"%s/%s",curpath+1,pars[i]);
        }
#ifdef shell
        my_spinlock(&plock);
        logy("path %s", path);
        my_spinunlock(&plock);
#endif
        if(vfs->rmdir(path) !=0)
            sprintf(buf,"Permission denied or the path is not exist.\n");
   }
}

void cmd_rm(int num, char **pars, char* curpath, char *buf){
    for(int i = 0; i <= num; ++i){
        char path[128];
        memset(path,0,sizeof(path));
        if(pars[i][0] == '/'){//绝对路径
           sprintf(path,"%s",pars[i]);
        }
        else{//相对路径
            if(pars[i][0] == '\0'){//当前目录
               if(i != 0) continue;//防止最后空格影响
               if(strcmp(curpath,"~") == 0)
                   sprintf(path,"/");
               else
                   sprintf(path,"%s",curpath+1);
            }
            else
               sprintf(path,"%s/%s",curpath+1,pars[i]);
        }
#ifdef shell
        my_spinlock(&plock);
        logy("path %s", path);
        my_spinunlock(&plock);
#endif
        vfs->unlink(path);
   }
}

void cmd_link(int num, char **pars, char* curpath, char *buf){
    char oldpath[128];
    memset(oldpath,0,sizeof(oldpath));
    if(pars[0][0] == '/'){//绝对路径
       sprintf(oldpath,"%s",pars[0]);
    }
    else{//相对路径
        if(pars[0][0] == '\0'){//当前目录
           if(strcmp(curpath,"~") == 0)
               sprintf(oldpath,"/");
           else
               sprintf(oldpath,"%s",curpath+1);
        }
        else
           sprintf(oldpath,"%s/%s",curpath+1,pars[0]);
    }

    char newpath[128];
    memset(newpath,0,sizeof(newpath));
    if(pars[1][0] == '/'){//绝对路径
       sprintf(newpath,"%s",pars[1]);
    }
    else{//相对路径
        if(pars[1][0] == '\0'){//当前目录
           if(strcmp(curpath,"~") == 0)
               sprintf(newpath,"/");
           else
               sprintf(newpath,"%s",curpath+1); }
        else
           sprintf(newpath,"%s/%s",curpath+1,pars[1]);
    }

    vfs->link(oldpath,newpath);
    return;
}

void cmd_touch(int num, char **pars, char* curpath, char *buf){
    for(int i = 0; i <= num; ++i){
        char path[128];
        memset(path,0,sizeof(path));
        if(pars[i][0] == '/'){//绝对路径
           sprintf(path,"%s",pars[i]);
        }
        else{//相对路径
            if(pars[i][0] == '\0'){//当前目录
               if(i != 0) continue;//防止最后空格影响
               if(strcmp(curpath,"~") == 0)
                   sprintf(path,"/");
               else
                   sprintf(path,"%s",curpath+1);
            }
            else
               sprintf(path,"%s/%s",curpath+1,pars[i]);
        }
#ifdef shell
        my_spinlock(&plock);
        logy("path %s", path);
        my_spinunlock(&plock);
#endif
        char mntname[128];
        int mnt = 0;
        memset(mntname,0,sizeof(mntname));
        for(int pp = 1; pp < strlen(path); ++pp){
            if(path[pp] == '/') break;
            mntname[mnt++] = path[pp];
        }
        if(strcmp(mntname,"dev") == 0 || strcmp(mntname, "proc") == 0){
            sprintf(buf,"Permission denied.\n");
            continue;
        }

        if(vfs->access(path,F_OK) == 0 || vfs->access(path,DIR_OK) == 0)
            continue;
        else{
            int flags = O_CREAT|O_RDWR;
            int fd = vfs->open(path,flags);
            if(fd > 0)
                assert(vfs->close(fd) == 0);
        }
   
    }//for
}

static uint8_t temparr[FILEBLOCK];
void cmd_echo(int num, char **pars, char* curpath, char *buf){
    int i;
    for(i = 0; i <= num; ++i){
        if(strcmp(pars[i],">") == 0) break;
    }
    
    if(i == num+1){//no >
        for(i = 0; i <= num; ++i){
            if(strlen(buf) + strlen(pars[i]) + 1 > 127) break;
            sprintf(buf+strlen(buf),"%s ",pars[i]);
        }
        sprintf(buf+strlen(buf),"\n"); 
        return;
    }
    if(i == num){
        sprintf(buf,"The format of cmd is Invalid.\n");
        return;
    }
    int fileind = i + 1;
    char path[128];
    memset(path,0,sizeof(path));
    if(pars[fileind][0] == '/'){//绝对路径
       sprintf(path,"%s",pars[fileind]);
    }
    else{//相对路径
        if(pars[fileind][0] == '\0'){//当前目录
           if(strcmp(curpath,"~") == 0)
               sprintf(path,"/");
           else
               sprintf(path,"%s",curpath+1);
        }
        else
           sprintf(path,"%s/%s",curpath+1,pars[fileind]);
    }
#ifdef shell
    my_spinlock(&plock);
    logy("path %s", path);
    my_spinunlock(&plock);
#endif
    int check =  vfs->access(path,DIR_OK);
    if(check == 0){
        sprintf(buf,"Cannot echo to a directory or a dev.\n");
        return;
    }
    inode_t *filenode;
    int fd;
    check = vfs->access(path,F_OK);
    if(check == 0){//already exist
        fd = vfs->open(path,O_RDWR);
        assert(fd >= 0);
        filenode = cur_task->fildes[fd]->inode;
        if(filenode == NULL){//dev
             char filename[32];
             memset(filename,0,sizeof(filename));
             sprintf(filename,"%s",path+5);
         //    if(strcmp(filename,"tty1") != 0 && strcmp(filename,"tty4") != 0 && strcmp(filename,"tty2") != 0 && strcmp(filename,"tty3") != 0){
         //        sprintf(buf,"Permission denied.\n");
          //       return;
         //    }
             device_t *dev = dev_lookup(filename);
             for(int j = 0; j <= num; ++j){
                 if(j == i || j == i + 1) continue;
                 if(strlen(buf) + strlen(pars[j]) + 1 > 128) break;
                 sprintf(buf+strlen(buf),"%s ",pars[j]);
             }
             buf[strlen(buf)] = '\n';
             dev->ops->write(dev,0,(void*)buf,strlen(buf));
             memset(buf,0,sizeof(buf));
             return;
        }
        
        if(filenode->fs->dev == NULL){
            sprintf(buf,"Permission denied.\n");
            return;
        }
        
        //clean the block
        memset(temparr,0,FILEBLOCK);
        filenode->fs->dev->ops->write(filenode->fs->dev,filenode->fs->fsoff+filenode->fs->blooff+filenode->nblocks*FILEBLOCK,(void*)temparr,FILEBLOCK);
    }
    else{
        fd = vfs->open(path,O_RDWR|O_CREAT);
        if(fd < 0){
            sprintf(buf,"Open file fail.\n");
            return;
        }
        filenode = cur_task->fildes[fd]->inode;
        if(filenode == NULL || filenode->fs->dev == NULL){
            sprintf(buf,"Permission denied.\n");
            return;
        }
    }

    //write new message
    for(int j = 0; j <= num; ++j){
        if(j == i || j == i + 1) continue;
        if(strlen(buf) + strlen(pars[j]) + 1 > 128) break;
        sprintf(buf+strlen(buf),"%s ",pars[j]);
    } 
    filenode->fs->dev->ops->write(filenode->fs->dev,filenode->fs->fsoff+filenode->fs->blooff+filenode->nblocks*FILEBLOCK,(void*)buf,strlen(buf));
    filenode->size = strlen(buf);
    filenode->fs->dev->ops->write(filenode->fs->dev,filenode->fs->fsoff+filenode->offset,(void*)filenode,sizeof(inode_t));
    assert(vfs->close(fd) == 0);
    memset(buf,0,128);
    return;
}

extern filesystem_t *devfs,*procfs;
void cmd_mnt(int num, char **pars, char* curpath, char *buf){
        if(strcmp(pars[0],"devfs") == 0){
            if(vfs->mount("/dev",devfs)==-1){
                sprintf(buf,"mount fail.\n");
                return;
            }
        }
        else if(strcmp(pars[0],"procfs") == 0){
            if(vfs->mount("/proc",procfs)==-1){
                sprintf(buf,"mount fail.\n");
                return;
            }
        }
        else if(strcmp(pars[0],"blkfs") == 0){
            sprintf(buf,"Permisiion denied.\n");
            return;
        }
        else{
            sprintf(buf,"No such a filesystem.\n");
            return;
        }
}

void cmd_unmnt(int num, char **pars, char* curpath, char *buf){
        if(strcmp(pars[0],"devfs") == 0){
            if(vfs->unmount("/dev")==-1){
                sprintf(buf,"unmount fail.\n");
                return;
            }
        }
        else if(strcmp(pars[0],"procfs") == 0){
            if(vfs->unmount("/proc")==-1){
                sprintf(buf,"unmount fail.\n");
                return;
            }
        }
        else if(strcmp(pars[0],"blkfs") == 0){
            sprintf(buf,"Permisiion denied.\n");
            return;
        }
        else{
            sprintf(buf,"No such a filesystem.\n");
            return;
        }
}
