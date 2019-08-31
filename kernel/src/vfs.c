#include <devices.h>
#include <common.h>
#include <x86.h>

extern inodeops_t inode_ops;
uint8_t temparray[FILEBLOCK];

struct Proc hgproc;
// =============================== fsops ===================================//
//spinlock_t fslock = {0,"fslock",NULL};
void fsinit(struct filesystem *fs, const char *name, device_t *dev){
    fs->name = name;
    fs->dev = dev;
    memset(fs->inooff,-1,sizeof(fs->inooff));
    fs->blooff = -1;
    return;
}
//return the number of inode in fs
uint32_t fsgetinode(inode_t * cur,const char *path, int flags){
    //cur is a inode of dir
    char dirname[35], rest[130];
    char total[130];
    memset(total,0,sizeof(total));
    strncpy(total,path,strlen(path)+1);
    uint8_t flag1 = 0;//means not the last 
    uint8_t flag2 = 0;//means there not exist dir
    uint8_t judge = 1;//means is file
    uint8_t existbit = 0;
    uint8_t rmbit = 0;
    
    if(flags & O_RM) rmbit = 1;
    if(flags & O_CHECK) existbit = 1;//the access and mkdir to check whether exist
    if(flags & O_DIR) judge = 0;//means is dir

#ifdef L3DEBUG
    my_spinlock(&plock);
    logy("rm: %d, ex: %d, judge: %d",rmbit,existbit,judge);
    my_spinunlock(&plock);
#endif
    filesystem_t *curfs = cur->fs;
    device_t *curdev = (device_t*)cur->ptr;

    while(!flag1){
       //analize path
       flag2 = 0;
       memset(dirname,0,sizeof(dirname));
       memset(rest,0,sizeof(rest));
       dirname[0] = '/';
       int i;
       for(i = 1; i < strlen(total); ++i){
             if(total[i] == '/') break;
             dirname[i] = total[i];
       }
  #ifdef L3DEBUG
       my_spinlock(&plock);
       logg("dirname : %s",dirname);
       my_spinunlock(&plock);
  #endif
       if(i == strlen(total)){
           flag1 = 1;
           flag2 = 1;
       }
       else{
          int j = 0;
          for(;i < strlen(total); ++i)
              rest[j++] = total[i];
   #ifdef L3DEBUG
          my_spinlock(&plock);
          logg("rest : %s",rest);
          my_spinunlock(&plock);
   #endif
       } 

#ifdef L3DEBUG
       my_spinlock(&plock);
       logy("flag1: %d, flag2: %d",flag1,flag2);
       my_spinunlock(&plock);
#endif
       //begin to find
       uint32_t ret = -1;
       uint32_t bnum = cur->nblocks;
       
       ed_t *rec = (ed_t*)pmm->alloc(sizeof(ed_t));
       curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+(FILEBLOCK*bnum),(void*)rec, sizeof(ed_t));
       int ind;
       for(ind = 0; ind < NFILEINFS; ++ind){
           if(flag1){//is the last 
               if(rec->etrs[ind].valid == 1){//is using
                   if(judge == 1){//file
                       if(strcmp(rec->etrs[ind].name,dirname)==0){//which is dir
                             assert(rec->etrs[ind].ftype == EXT2_FT_DIR);
                             if(!existbit){
                                 my_spinlock(&plock);
                                 logr("Cannot get the file as \"%s\"(dir) is already exist!",dirname);
                             
                                my_spinunlock(&plock);
                             }                             
                             pmm->free(rec);
                             return -1;
                       }
                       if(rec->etrs[ind].ftype == EXT2_FT_REG_FILE){
                           if(strcmp(rec->etrs[ind].name,dirname+1) == 0){
                               //check rm or open
                               if(!rmbit){
                                 //get!!!
                                  ret = rec->etrs[ind].ino;
                                  pmm->free(rec);
                                  #ifdef L3DEBUG
                                     my_spinlock(&plock);
                                     logy("get file : %d",ret);
                                     my_spinunlock(&plock);
                                  #endif
                                  return ret;
                               }
                               else{
                                  ret = rec->etrs[ind].ino;
                                  #ifdef L3DEBUG
                                     my_spinlock(&plock);
                                     logy("rm file : %d",ret);
                                     my_spinunlock(&plock);
                                  #endif
                                  //clean the block
                                  memset(temparray,0,sizeof(temparray));
                                  curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+ret*FILEBLOCK,(const void*)temparray,sizeof(temparray));
                                  //clean the ext2_dir
                                  rec->etrs[ind].valid = 0;
                                  curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+(FILEBLOCK*bnum),(const void*)rec, sizeof(ed_t));
                                  pmm->free(rec);
                                  //don't change the cur file size 
                                
                                  //lookup clean the inode
                                  return ret;
                               }
                           }
                       } 
                   }
                   else if(judge == 0){//dir
                       if(strcmp(rec->etrs[ind].name,dirname+1) == 0){
                           assert(rec->etrs[ind].ftype == EXT2_FT_REG_FILE);
                           if(!existbit){
                               my_spinlock(&plock);
                               logr("Cannot get the dir as \"%s\"(file) is already exist!",dirname+1);
                               my_spinunlock(&plock);
                           }
                           pmm->free(rec);
                           return -1;
                       }
                       if(rec->etrs[ind].ftype == EXT2_FT_DIR){
                           if(strcmp(rec->etrs[ind].name,dirname) == 0){
                                //check rm or open
                               if(!rmbit){
                                  //get!!!
                                  ret = rec->etrs[ind].ino;
                                  pmm->free(rec);
                                  #ifdef L3DEBUG
                                     my_spinlock(&plock);
                                     logy("get dir : %d",ret);
                                     my_spinunlock(&plock);
                                  #endif
                                  return ret;
                               }
                               else{
                                 //check whether the dir is empty
                                 ret = rec->etrs[ind].ino;
                                  #ifdef L3DEBUG
                                     my_spinlock(&plock);
                                     logy("rm dir : %d",ret);
                                     my_spinunlock(&plock);
                                  #endif
                                 //第几个inode对应了第几个block
                                 ed_t* rdir = (ed_t*)pmm->alloc(sizeof(ed_t));
                                 curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+ret*FILEBLOCK,(void*)rdir,sizeof(ed_t));
                                 int j;
                                 for(j = 0; j < NFILEINFS; ++j){
                                     if(rdir->etrs[j].valid == 1){//the dir is not empty
                                           pmm->free(rec);
                                           pmm->free(rdir);
                                           my_spinlock(&plock);
                                           logr("The directory is not empty!");
                                           my_spinunlock(&plock);
                                           return -1;
                                     }
                                 }
                                //the dir is empty
                                
                                //clean the block
                                memset(rdir,0,sizeof(ed_t));
                                curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+ret*FILEBLOCK,(const void*)rdir,sizeof(ed_t));
                                pmm->free(rdir);
                                
                                //clean the ext2_dir
                                rec->etrs[ind].valid = 0;
       curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+(FILEBLOCK*bnum),(const void*)rec, sizeof(ed_t));
                                pmm->free(rec);
                                //don't change the cur file size 
                                
                                //lookup clean the inode
                                return ret;
                               }//else rm
                           }
                       } 
                   }
                   else
                       assert(0);
               } 
           }
           else{//not the last 
               if(rec->etrs[ind].valid == 1){
                   if(rec->etrs[ind].ftype == EXT2_FT_DIR){
                       if(strcmp(rec->etrs[ind].name,dirname) == 0){
                              memset(total,0,sizeof(total));
                              strncpy(total,rest,strlen(rest)+1);
                              curdev->ops->read(curdev,curfs->fsoff+curfs->inooff[rec->etrs[ind].ino],(void*)cur,sizeof(inode_t));
                              flag2 = 1;
                              break;
                       }
                   }  
               }
           }
       }//for
       if(!flag2){
           if(!existbit){
              my_spinlock(&plock);
              logr("There no such a file or directory!\n");
              my_spinunlock(&plock);
           }
          pmm->free(rec);
          return -1;
       }
       pmm->free(rec);
    }//while

    //create
    if(! (flags & O_CREAT)){
        if(!existbit){
            my_spinlock(&plock);
            logr("There no such a file or directory!\n");
            my_spinunlock(&plock);
        }
        return -1;
    }
    else {
       uint32_t bnum = cur->nblocks;
       ed_t *rec = (ed_t*)pmm->alloc(sizeof(ed_t));
       curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+(FILEBLOCK*bnum),(void*)rec, sizeof(ed_t));
       int i;
      
       for(i = 0; i < NFILEINFS; ++i){
           if(curfs->inooff[i] == -1){//not used
                  inode_t* newnode = (inode_t*)pmm->alloc(sizeof(inode_t));
                  newnode->mod = O_RDWR;
                  newnode->size = 0;
                  newnode->refcnt = 0;
                  newnode->ptr = (void*)curdev;
                  newnode->fs = curfs;
                  newnode->ops = &inode_ops;
                  curfs->inooff[i] = 4+i*sizeof(inode_t);
                  newnode->offset = curfs->inooff[i];
                  newnode->nblocks = i; 
                
#ifdef L3DEBUG
        my_spinlock(&plock);
        logq("inodenumber :%d",i);
        my_spinunlock(&plock);
#endif
                  curdev->ops->write(curdev,curfs->fsoff+curfs->inooff[i],(void*)newnode,sizeof(inode_t));
                  pmm->free(newnode);
/*
#ifdef L3DEBUG
       newnode = (inode_t*)pmm->alloc(sizeof(inode_t));
       curdev->ops->read(curdev,curfs->fsoff+curfs->inooff[i],(void*)newnode,sizeof(inode_t));
       logb("readmod: %d",newnode->mod);
       logb("readnblocks :%d",newnode->nblocks);
       logb("fsname :%s",curfs->name);
#endif
*/
                  if(judge == 0){//dir
                     ed_t*  newed = (ed_t*)pmm->alloc(sizeof(ed_t));
                     memset(newed,0,sizeof(ed_t));
                     curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+i*FILEBLOCK,(void*)newed,sizeof(ed_t));
                     pmm->free(newed);
                     
                     //update ext2_dir_entry
                     int j;
                     for(j = 0; j < NFILEINFS; ++j){
                         if(rec->etrs[j].valid == 0){
#ifdef L3DEBUG
        my_spinlock(&plock);
        logy("dir entry : %d",j);
        my_spinunlock(&plock);
#endif
                               rec->etrs[j].valid = 1;
                               rec->etrs[j].ino = i;
                               rec->etrs[j].ftype = EXT2_FT_DIR;
                               strncpy(rec->etrs[j].name,dirname,strlen(dirname)+1);
                               break;
                         }
                     }
                     //更新cur文件大小
                     if(j == rec->maxuse){
                         cur->size = cur->size + sizeof(edt_t);
                         curdev->ops->write(curdev,curfs->fsoff+cur->offset,(void*)cur,sizeof(inode_t));
                         rec->maxuse++;
                     }
                     curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+cur->nblocks*FILEBLOCK,(void*)rec,sizeof(ed_t));
                     pmm->free(rec);
                     return i; 
                  }
                  else if(judge == 1){//file
                     memset(temparray,0,sizeof(temparray));
                     curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+i*FILEBLOCK,(void*)temparray,sizeof(temparray));
                     //update ext2_dir
                     int j;
                     for(j = 0; j < NFILEINFS; ++j){
                         if(rec->etrs[j].valid == 0){
#ifdef L3DEBUG
        my_spinlock(&plock);
        logy("file entry : %d",j);
        my_spinunlock(&plock);
#endif
                               rec->etrs[j].valid = 1;
                               rec->etrs[j].ino = i;
                               rec->etrs[j].ftype = EXT2_FT_REG_FILE;
                               strncpy(rec->etrs[j].name,dirname+1,strlen(dirname));
                               break;
                         }
                     }
                     //更新cur文件大小
                     if(j == rec->maxuse){
                         cur->size = cur->size + sizeof(edt_t);
                         curdev->ops->write(curdev,curfs->fsoff+cur->offset,(void*)cur,sizeof(inode_t));
                         rec->maxuse++;
                     }
                     curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+cur->nblocks*FILEBLOCK,(void*)rec,sizeof(ed_t));
                     pmm->free(rec);
                     return i; 
                  }//elseif
           }//if
       }//for
       my_spinlock(&plock);
       logr("You cannot create too many files\n");
       my_spinunlock(&plock);
    }
   return -1;
}

inode_t * fslookup(struct filesystem *fs, const char *path, int flags){
   
    uint32_t inonum;
    inode_t *root = (inode_t*)pmm->alloc(sizeof(inode_t));

    fs->dev->ops->read(fs->dev,fs->fsoff + fs->inooff[0],(void*)root,sizeof(inode_t));

#ifdef L3DEBUG
    my_spinlock(&plock);
    logq("mod : %d", root->mod);
    logq("nblocks : %d",root->nblocks);
    logq("offset : %d,",root->offset);
    my_spinunlock(&plock);
#endif

    inonum = fsgetinode(root,path,flags);

#ifdef L3DEBUG
    my_spinlock(&plock);
    logq("inonum : %d",inonum);
    my_spinunlock(&plock);
#endif
    pmm->free(root);
    
    if(inonum == -1){
        return NULL;
    }
    
    inode_t *ret = (inode_t*)pmm->alloc(sizeof(inode_t));

    if(flags & O_RM){
       //lookup clean the inode
        fs->dev->ops->read(fs->dev,fs->fsoff+fs->inooff[inonum],(void*)ret,sizeof(inode_t));
        
        inode_t *replace = (inode_t*)pmm->alloc(sizeof(inode_t));
        fs->dev->ops->write(fs->dev,fs->fsoff+fs->inooff[inonum],(const void*)replace,sizeof(inode_t));
        pmm->free(replace);
       
        fs->inooff[inonum] = -1;
        return ret;
    }
    else{
       fs->dev->ops->read(fs->dev,fs->fsoff+fs->inooff[inonum],(void*)ret,sizeof(inode_t));
#ifdef L3DEBUG
       my_spinlock(&plock);
       logy("ret->nblocks :%d\n",ret->nblocks);
       my_spinunlock(&plock);
#endif
       return ret;
    }

} 
int fsclose(inode_t *inode){
    TODO();
    return 0;
}

// ================================= vfs ====================================//

filesystem_t *blkfs,*devfs,*procfs;
mpt_t mntpoint[MNTNUM];

#ifndef SHELL
spinlock_t vfslock = {0,"vfslock",NULL};
#endif

int vfsmount(const char *path, filesystem_t *fs){
#ifndef SHELL
    my_spinlock(&vfslock);
#endif
    //检查是否已经挂载了
    for(int j = 0; j < MNTNUM; ++j){
        if(mntpoint[j].fs == fs){
            my_spinlock(&plock);
            logr("The fs is already mnt at \"%s\"",mntpoint[j].name);
            my_spinunlock(&plock);
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return -1;
        }
    }

    int i = 0;
    while(mntpoint[i].fs != NULL && i < MNTNUM) i++;
    if(i == MNTNUM){
     #ifndef SHELL
         my_spinunlock(&vfslock);
     #endif
     return -1;
    }
    strncpy(mntpoint[i].name, path, strlen(path)+1);
    mntpoint[i].fs = fs;
    if(fs == devfs) 
        blkfs->ops->lookup(blkfs,"/dev",O_DIR|O_CREAT|O_RDWR);
    else if(fs == procfs)
        blkfs->ops->lookup(blkfs,"/proc",O_DIR|O_CREAT|O_RDWR);

    #ifdef L3DEBUG
    my_spinlock(&plock);
    logy("name : %s\n",mntpoint[i].name);
    my_spinunlock(&plock);
    #endif
    
    #ifndef SHELL
        my_spinunlock(&vfslock);
    #endif
    return 0;
}


int vfsunmount(const char *path){
#ifndef SHELL
    my_spinlock(&vfslock);
#endif
    int i;
    for(i = 0; i < MNTNUM; ++i){
        if(strcmp(path,mntpoint[i].name) == 0){
            if(mntpoint[i].fs == devfs)
                 blkfs->ops->lookup(blkfs,"/dev",O_DIR|O_RM);
            else if(mntpoint[i].fs == procfs)
                 blkfs->ops->lookup(blkfs,"/proc",O_DIR|O_RM);
            mntpoint[i].fs = NULL;
            memset(mntpoint[i].name,0,sizeof(mntpoint[i].name));
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return 0;
        }
    }

    my_spinlock(&plock);
    logr("No fs at \"%s\"",path);
    my_spinunlock(&plock);
    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
    return -1;
}

int vfsaccess(const char *path, int mode){
#ifndef SHELL
   my_spinlock(&vfslock);
#endif
    if(strcmp(path,"/") == 0){
        if(mode & DIR_OK){
           #ifndef SHELL
            my_spinunlock(&vfslock);
           #endif
           return 0;
        }
        else{
           #ifndef SHELL
            my_spinunlock(&vfslock);
           #endif
           return -1;
        } 
    }
    //separate the path
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(path); ++i){
        if(path[i] == '/') break;
        mntname[i] = path[i];
    }

    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
   
    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }

    if(ind == -1 && (strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0)){
         //dev或者proc没有挂载
         my_spinlock(&plock);
         logr("The filesystem isnot mount yet");
         my_spinunlock(&plock);
           #ifndef SHELL
            my_spinunlock(&vfslock);
           #endif
         return -1;
    }
    
    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
        #ifndef SHELL
          my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    for(;i < strlen(path); ++i)
        dirname[j++] = path[i];

#ifdef L3DEBUG
    my_spinlock(&plock);
    logb("mntname: %s",mntname);
    logb("dirname: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef L3DEBUG
   my_spinlock(&plock);
   logb("mntindex: %d",ind);
   my_spinunlock(&plock);
#endif
   
   if(strcmp(mntname,"/dev") == 0){
       if(dirname[0] == '\0' && (mode & DIR_OK)){
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           return 0;
       }
       else if(mode & DIR_OK){
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           return -1;
       }
       for(int pp = 0; pp < strlen(dirname)+1; ++pp)
           dirname[pp] = dirname[pp+1];
       if(dev_lookup(dirname) != NULL){
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return 0;
       }
       else{
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return -1;
       }
   }

   if(strcmp(mntname,"/proc") == 0){
       if(dirname[0] == '\0' && (mode & DIR_OK)){
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
           return 0;
       }
       else if(mode & DIR_OK){
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return -1;
       }
       else{
           for(int pp = 0; pp < strlen(dirname)+1; ++pp)
               dirname[pp] = dirname[pp+1];
           
           ed_t *rec = (ed_t*)hgproc.block[0];
           for(int pp = 0; pp < NFILEINFS; ++pp){
               if(rec->etrs[pp].valid == 1){
                   if(strcmp(rec->etrs[pp].name,dirname) == 0){
                    #ifndef SHELL
                        my_spinunlock(&vfslock);
                    #endif
                    return 0;
                   }
               }
           }
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           return -1;
       }
   }

   int flags = -1;
   if(mode & F_OK)
       flags = 0|O_CHECK;
   else if(mode & DIR_OK){
       flags = O_DIR|O_CHECK;
   }

   inode_t *node = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);

   if(node == NULL){
      pmm->free(node);
      #ifndef SHELL
          my_spinunlock(&vfslock);
      #endif
      return -1;
   }
   else{
       pmm->free(node);
       #ifndef SHELL
           my_spinunlock(&vfslock);
       #endif
       return 0;
   }
}

int vfsmkdir(const char *path){
    #ifndef SHELL
        my_spinlock(&vfslock);
    #endif
    if(strcmp(path,"/") == 0){
      my_spinlock(&plock);
      logr("The directory is already exist!\n");
      my_spinunlock(&plock);
       #ifndef SHELL
           my_spinunlock(&vfslock);
       #endif
      return -1;
    }
    //separate the path
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(path); ++i){
        if(path[i] == '/') break;
        mntname[i] = path[i];
    }

    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
   
    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }

    if(strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0){
         //dev或者proc没有挂载
         my_spinlock(&plock);
         logr("Permission denied");
         my_spinunlock(&plock);
         #ifndef SHELL
             my_spinunlock(&vfslock);
         #endif
         return -1;
    }

    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
         #ifndef SHELL
             my_spinunlock(&vfslock);
         #endif
        return -1;
    }
    
    for(;i < strlen(path); ++i)
        dirname[j++] = path[i];

#ifdef L3DEBUG
    my_spinlock(&plock);
    logb("mntname: %s",mntname);
    logb("dirname: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef L3DEBUG
   my_spinlock(&plock);
   logb("mntindex: %d",ind);
   my_spinunlock(&plock);
#endif

   int flags = O_DIR|O_CHECK;

   inode_t *node = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);

   if(node != NULL){
       my_spinlock(&plock);
       logr("The directory is already exist!\n");
       my_spinunlock(&plock);
       pmm->free(node);
       #ifndef SHELL
           my_spinunlock(&vfslock);
       #endif
       return -1;
   }

   flags = O_DIR|O_CREAT;

   node = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   
   if(node == NULL){
       my_spinlock(&plock);
       logr("Create directory failed\n");
       my_spinunlock(&plock);
       pmm->free(node);
       #ifndef SHELL
           my_spinunlock(&vfslock);
       #endif
       return -1;
   }

   pmm->free(node);
   #ifndef SHELL
       my_spinunlock(&vfslock);
   #endif
   return 0;
}

int vfsrmdir(const char *path){
   #ifndef SHELL
       my_spinlock(&vfslock);
   #endif
    if(strcmp(path,"/") == 0){
         my_spinlock(&plock);
         logr("Permission denied");
         my_spinunlock(&plock);
         #ifndef SHELL
            my_spinunlock(&vfslock);
         #endif
         return -1;
    }
    //separate the path
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(path); ++i){
        if(path[i] == '/') break;
        mntname[i] = path[i];
    }

    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
   
    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }

    if(strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0){
         //dev或者proc没有挂载
         my_spinlock(&plock);
         logr("Permission denied");
         my_spinunlock(&plock);
         #ifndef SHELL
            my_spinunlock(&vfslock);
         #endif
         return -1;
    }
    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
         #ifndef SHELL
            my_spinunlock(&vfslock);
         #endif
        return -1;
    }
    
    for(;i < strlen(path); ++i)
        dirname[j++] = path[i];

#ifdef L3DEBUG
    my_spinlock(&plock);
    logb("mntname: %s",mntname);
    logb("dirname: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef L3DEBUG
   my_spinlock(&plock);
   logb("mntindex: %d",ind);
   my_spinunlock(&plock);
#endif
   
   int flags = O_DIR|O_RM;
   inode_t *node = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   //对于rmdir,lookup传回来是覆盖之前的inode和未找到文件做区分
   if(node == NULL){//means not succeed
        pmm->free(node);
         #ifndef SHELL
            my_spinunlock(&vfslock);
         #endif
        return -1;
   }
#ifdef L3DEBUG
   my_spinlock(&plock);
   logy("\n");
   my_spinunlock(&plock);
#endif
    pmm->free(node);
    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
    return 0;
}


int vfslink(const char *oldpath, const char *newpath){
    #ifndef SHELL
       my_spinlock(&vfslock);
    #endif
    //separate the oldpath
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(oldpath); ++i){
        if(oldpath[i] == '/') break;
        mntname[i] = oldpath[i];
    }
    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
  
    //refuse /dev and /proc
    if(strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0){
        my_spinlock(&plock);
        logr("Permisiion denied");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }

    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }
    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    for(;i < strlen(oldpath); ++i)
        dirname[j++] = oldpath[i];

#ifdef l3DEBUG
    my_spinlock(&plock);
    logb("mntname1: %s",mntname);
    logb("dirname1: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef l3DEBUG
   my_spinlock(&plock);
   logb("mntindex1: %d",ind);
   my_spinunlock(&plock);
#endif

   int flags = 0|O_CHECK; 
   inode_t *oldnode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   if(oldnode == NULL){
       my_spinlock(&plock);
       logr("The oldpath: %s is not exist.",oldpath);
       my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
       return -1;
   }

// -------------------------- seperate newpath ----------------------------//   
    j = 0, ind = -1, root = -1;
    memset(mntname,0,sizeof(mntname));
    memset(dirname,0,sizeof(dirname));
    
    mntname[0] = '/';
    for(i = 1; i < strlen(newpath); ++i){
        if(newpath[i] == '/') break;
        mntname[i] = newpath[i];
    }
    
     //refuse /dev and /proc
    if(strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0){
        my_spinlock(&plock);
        logr("Permisiion denied");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
   
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }
    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
    
        pmm->free(oldnode);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    for(;i < strlen(newpath); ++i)
        dirname[j++] = newpath[i];

#ifdef l3DEBUG
    my_spinlock(&plock);
    logb("mntname2: %s",mntname);
    logb("dirname2: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef l3DEBUG
   my_spinlock(&plock);
   logb("mntindex2: %d",ind);
   my_spinunlock(&plock);
#endif
   
   flags = 0|O_CHECK; 
   inode_t *newnode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   if(newnode != NULL){
       my_spinlock(&plock);
       logr("The newpath:%s is already exist.",newpath);
       my_spinunlock(&plock);

       pmm->free(newnode);
       pmm->free(oldnode);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
       return -1;
   }
  
   // -------------------------- begin to link --------------------------//
   
   //get the dirinode 
   char filename[35];
   int filecnt = 0;
   memset(filename,0,sizeof(filename));
   int index;
   for(index = strlen(dirname)-1; index >= 0; --index){
           if(dirname[index] == '/') break;
   }
   for(int pp = index+1; pp < strlen(dirname); ++pp){
        filename[filecnt++] = dirname[pp]; 
   }
   dirname[index] = '\0';
#ifdef l3DEBUG
   logg("index :%d, filecnt :%d",index,filecnt);
   logg("filename %s",filename);
   logg("dirname %s",dirname);
#endif
   
    flags = O_DIR|O_CHECK;
    newnode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
    if(newnode == NULL){
        my_spinlock(&plock);
        logr("The dir of newpath is not exist. so cannot link");
        my_spinunlock(&plock);
        
        pmm->free(oldnode);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }

    ed_t *newrec = (ed_t*)pmm->alloc(sizeof(ed_t));
    filesystem_t *curfs = newnode->fs;
    device_t *curdev = curfs->dev;
    curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+newnode->nblocks*FILEBLOCK,(void*)newrec,sizeof(ed_t));
    for(int pp = 0; pp < NFILEINFS; ++pp){
        if(newrec->etrs[pp].valid == 0){
            newrec->etrs[pp].valid = 1;
            newrec->etrs[pp].ino = oldnode->nblocks;
            newrec->etrs[pp].ftype = EXT2_FT_REG_FILE;
            strncpy(newrec->etrs[pp].name,filename,strlen(filename)+1);
            if(pp == newrec->maxuse){
                newrec->maxuse++;
                newnode->size = newnode->size + sizeof(edt_t);
            }
            //change the refcnt
            oldnode->refcnt++;
#ifdef l3DEBUG
       logp("refcnt :%d",oldnode->refcnt);
#endif
            curdev->ops->write(curdev,curfs->fsoff+oldnode->offset,(void*)oldnode,sizeof(inode_t)); 
            //write into ramdisk
            curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+newnode->nblocks*FILEBLOCK,(void*)newrec,sizeof(ed_t));
            curdev->ops->write(curdev,curfs->fsoff+newnode->offset,(void*)newnode,sizeof(inode_t));
             
            pmm->free(newrec);
            pmm->free(newnode);
            pmm->free(oldnode);
            #ifndef SHELL
               my_spinunlock(&vfslock);
            #endif
            return 0;
        }
    }
    
    pmm->free(newrec);
    pmm->free(newnode);
    pmm->free(oldnode);
    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
    return -1;
}

int vfsunlink(const char *path){
#ifndef SHELL
   my_spinlock(&vfslock);
#endif
    //separate the path
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(path); ++i){
        if(path[i] == '/') break;
        mntname[i] = path[i];
    }
    
    //refuse /dev and /proc
    if(strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0){
        my_spinlock(&plock);
        logr("Permisiion denied");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
   
    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }
    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    for(;i < strlen(path); ++i)
        dirname[j++] = path[i];

#ifdef l3DEBUG
    my_spinlock(&plock);
    logb("mntname: %s",mntname);
    logb("dirname: %s",dirname);
    my_spinunlock(&plock);
#endif
#ifdef l3DEBUG
   my_spinlock(&plock);
   logb("mntindex: %d",ind);
   my_spinunlock(&plock);
#endif

   int flags = 0|O_CHECK;
   inode_t *filenode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   if(filenode == NULL){
       my_spinlock(&plock);
       logr("The file :%s is not exist",path);
       my_spinunlock(&plock);
      
       pmm->free(filenode);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
       return -1;
   }
   //判断引用量是否为0
   if(filenode->refcnt == 0){
       pmm->free(filenode);
       flags = 0|O_RM;
       filenode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
       if(filenode == NULL){
           my_spinlock(&plock);
           logr("remove file in unlink failed");
           my_spinunlock(&plock);
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           return -1;
       }
       else{
           pmm->free(filenode);
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           return 0;
       }
   }
   //引用量不为零
   filenode->refcnt--;
   device_t *ndev = (device_t*)filenode->fs->dev;
   ndev->ops->write(ndev,filenode->fs->fsoff+filenode->offset,(void*)filenode,sizeof(inode_t)); 

   char filename[35];
   int filecnt = 0;
   memset(filename,0,sizeof(filename));
   int index;
   for(index = strlen(dirname)-1; index >= 0; --index){
           if(dirname[index] == '/') break;
   }
   for(int pp = index+1; pp < strlen(dirname); ++pp){
        filename[filecnt++] = dirname[pp]; 
   }
   dirname[index] = '\0';
#ifdef l3DEBUG
   logg("index :%d, filecnt :%d",index,filecnt);
   logg("filename %s",filename);
   logg("dirname %s",dirname);
#endif
   
   flags = O_DIR|O_CHECK;
   inode_t *recnode = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
   //因为上面文件存在,所以这个不会为空
   assert(recnode);
   filesystem_t *curfs = recnode->fs;
   device_t *curdev = curfs->dev;
   ed_t *rec = (ed_t*)pmm->alloc(sizeof(ed_t));
   curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+recnode->nblocks*FILEBLOCK,(void*)rec,sizeof(ed_t));
   for(int pp = 0; pp < NFILEINFS; ++pp){
       if(strcmp(filename,rec->etrs[pp].name) == 0){
           rec->etrs[pp].valid = 0;
           curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+recnode->nblocks*FILEBLOCK,(void*)rec,sizeof(ed_t));
           pmm->free(filenode);
           pmm->free(recnode);
           pmm->free(rec);
           #ifndef SHELL
               my_spinunlock(&vfslock);
           #endif
           
           return 0;
       }
   }  
   pmm->free(filenode);
   pmm->free(recnode);
   pmm->free(rec);
   #ifndef SHELL
       my_spinunlock(&vfslock);
   #endif
   return -1;
}

int vfsopen(const char *path, int flags){
   #ifndef SHELL
       my_spinlock(&vfslock);
   #endif
    if(strcmp(path,"/") == 0){
        inode_t *node = (inode_t*)pmm->alloc(sizeof(inode_t)); 
        blkfs->dev->ops->read(blkfs->dev,blkfs->fsoff+blkfs->inooff[0],(void*)node,sizeof(inode_t));

        //find the smallest fd
        int ret = -1;
        for(int i = 0; i < NFILE; ++i){
            if(cur_task->fildes[i] == NULL){
                 cur_task->fildes[i] = (file_t*)pmm->alloc(sizeof(file_t));//remember to free
                 if(cur_task->fildes[i] == NULL){
                     my_spinlock(&plock);
                     logr("file_t alloc failed\n");
                     my_spinunlock(&plock);
                     assert(0);
                 }
    
                 ret = i;
                 cur_task->fildes[i]->inode = node;
                 cur_task->fildes[i]->offset = 0;
                 strncpy(cur_task->fildes[i]->name,path,strlen(path)+1);//use for dev 
                 break;
            }
        }
   #ifndef SHELL
       my_spinunlock(&vfslock);
   #endif
        return ret;
    }

    //separate the path
    char mntname[35];
    memset(mntname,0,sizeof(mntname));
    mntname[0] = '/';
    int i;
    for(i = 1; i < strlen(path); ++i){
        if(path[i] == '/') break;
        mntname[i] = path[i];
    }

    int  j = 0;
    char dirname[130];
    memset(dirname,0,sizeof(dirname));
   
    int ind = -1;
    int root = -1;
    for(int pp = 0; pp < MNTNUM; ++pp){
        if(strcmp(mntname,mntpoint[pp].name) == 0){
             ind = pp;
        }
        if(strcmp(mntpoint[pp].name,"/")==0){
            root = pp;
        }
    }

    if(ind == -1 && (strcmp(mntname,"/dev") == 0 || strcmp(mntname,"/proc") == 0)){
         //dev或者proc没有挂载
         my_spinlock(&plock);
         logr("The filesystem isnot mount yet");
         my_spinunlock(&plock);
         #ifndef SHELL
            my_spinunlock(&vfslock);
         #endif
         return -1;
    }

    if(ind == -1 && root != -1){//means '/'
        ind = root;
        strncpy(dirname,mntname,strlen(mntname)+1);
        j = strlen(mntname);
        for(int qq = 1; qq < strlen(mntname); ++qq){
            mntname[qq] = 0;
        }
    }
    if(root == -1 && ind == -1){
        my_spinlock(&plock);
        logr("No such a mntpoint");
        my_spinunlock(&plock);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    
    for(;i < strlen(path); ++i)
        dirname[j++] = path[i];

#ifdef l3DEBUG
    my_spinlock(&plock);
    logb("mntname: %s",mntname);
    logb("dirname: %s",dirname);
    my_spinunlock(&plock);
#endif

#ifdef l3DEBUG
   my_spinlock(&plock);
   logb("mntindex: %d",ind);
   my_spinunlock(&plock);
#endif
   
   inode_t *node = NULL;
   if(strcmp(mntname,"/dev")!=0 && strcmp(mntname,"/proc") != 0){
          // --------blkfs--------//
          node = mntpoint[ind].fs->ops->lookup(mntpoint[ind].fs,dirname,flags);
          if(node == NULL){
            #ifndef SHELL
                my_spinunlock(&vfslock);
            #endif
            return -1;
          }
       
       #ifdef L3DEBUG
           my_spinlock(&plock);
           logb("file size : %x",node->size);
           my_spinunlock(&plock);
       #endif
         
    }
   else{
          // ------- procfs & devfs ---------//
       if(strcmp(mntname,"/dev") == 0){
           if(dirname[0] == '\0' && (flags & O_DIR)){
               node = NULL;
           }
           else{
                if(strcmp(dirname,"/ramdisk0") == 0 || strcmp(dirname, "/ramdisk1") == 0 || strcmp(dirname,"/tty1") == 0 || strcmp(dirname, "/tty2") == 0 || strcmp(dirname, "/tty3") == 0 || strcmp(dirname, "/tty4") == 0)
                 node = NULL; 
                else{
                    my_spinlock(&plock);
                    logr("Permission denied");
                    my_spinunlock(&plock);
                    #ifndef SHELL
                        my_spinunlock(&vfslock);
                    #endif
                    return -1;
                }
           }
       }
       else if(strcmp(mntname,"/proc") == 0){
           if(dirname[0] == '\0' && (flags & O_DIR)){
               node = hgproc.ino[0];
           }
           else{
               for(int pp = 0; pp < strlen(dirname)+1; ++pp)
                    dirname[pp] = dirname[pp+1];
               ed_t *rec = (ed_t*)hgproc.block[0];
               for(int pp = 0; pp < NFILEINFS; ++pp){
                   if(rec->etrs[pp].valid == 1){
                       if(strcmp(dirname,rec->etrs[pp].name) == 0){
                           node = hgproc.ino[rec->etrs[pp].ino];
                           break;
                       }
                   }
               }//for
               if(node == NULL){
                   if(flags & O_CREAT){
                       for(int pp = 0; pp < NFILEINFS; ++pp){
                           if(hgproc.ino[pp] == NULL){
                               hgproc.ino[pp] = (inode_t*)pmm->alloc(sizeof(inode_t));
                               hgproc.block[pp] = pmm->alloc(FILEBLOCK);
                               node = hgproc.ino[pp];
                               node->size = 0, node->mod = O_RDONLY, node->nblocks = pp;
                               node->offset = pp, node->fs = procfs, node->ops = &inode_ops;
                               ed_t *rec = (ed_t*)hgproc.block[0];
                               for(int qq = 0; qq < NFILEINFS; ++qq){
                                   if(rec->etrs[qq].valid == 0){
                                       rec->etrs[qq].valid = 1;
                                       rec->etrs[qq].ino = pp;
                                       rec->etrs[qq].ftype = EXT2_FT_REG_FILE;
                                       strncpy(rec->etrs[qq].name,dirname,strlen(dirname)+1);
                                       if(qq == rec->maxuse){
                                           hgproc.ino[0]->size += sizeof(edt_t);
                                       }
                                       break;
                                   }
                               }
                               break;
                           }
                       } 
                       if(node == NULL){
                           #ifndef SHELL
                              my_spinunlock(&vfslock);
                           #endif
                           return -1;
                       }
                   }
                   else{
                       #ifndef SHELL
                           my_spinunlock(&vfslock);
                       #endif
                       return -1;
                   }
               }
           }
           
       }
       else{
         #ifndef SHELL
             my_spinunlock(&vfslock);
         #endif
         return -1;
       }
   }
    
    //find the smallest fd
    int ret = -1;
    for(int i = 0; i < NFILE; ++i){
        if(cur_task->fildes[i] == NULL){
             cur_task->fildes[i] = (file_t*)pmm->alloc(sizeof(file_t));//remember to free
             if(cur_task->fildes[i] == NULL){
                 my_spinlock(&plock);
                 logr("file_t alloc failed\n");
                 my_spinunlock(&plock);
                 assert(0);
             }

             ret = i;
             cur_task->fildes[i]->inode = node;
             cur_task->fildes[i]->offset = 0;
             strncpy(cur_task->fildes[i]->name,path,strlen(path)+1);//use for dev 
             break;
        }
    }
    #ifndef SHELL
        my_spinunlock(&vfslock);
    #endif
    return ret;
}

int vfsread(int fd, void *buf, size_t nbyte){
    if(fd < 0){
        return -1;
    }
    uint64_t curoff = cur_task->fildes[fd]->offset;
    inode_t *curnode = cur_task->fildes[fd]->inode;
    ssize_t rbyte;
    if(curnode == NULL){//devfs
        char mntname[35], devname[32];
        memset(mntname,0,sizeof(mntname));
        memset(devname,0,sizeof(devname));
        mntname[0] = '/';
        int i;
        for(i = 1; i < strlen(cur_task->fildes[fd]->name); ++i){
            if(cur_task->fildes[fd]->name[i] == '/') break;
            mntname[i] = cur_task->fildes[fd]->name[i];
        }
        assert(strcmp(mntname,"/dev") == 0);
        int j = 0;
        i++;//remove /
        for(;i < strlen(cur_task->fildes[fd]->name); ++i)
            devname[j++] = cur_task->fildes[fd]->name[i];
       
        device_t *curdev = dev_lookup(devname);
        rbyte = curdev->ops->read(curdev,curoff,buf,nbyte); 
        
    }
    else{
        filesystem_t *curfs = curnode->fs;
        device_t *curdev = curfs->dev;
        if(curdev == NULL){//procfs
            if(curoff > curnode->size){
                buf = NULL;
                return 0;
            }
            rbyte = 0;
            if(nbyte + curoff > curnode->size)
                rbyte = curnode->size - curoff;
            else
                rbyte = nbyte;
            int num = curnode->nblocks;
            char* block = hgproc.block[num];
            memcpy(buf,block+curoff,rbyte);
        }
        else{//blkfs
#ifdef l3DEBUG
        my_spinlock(&plock);
        logr("in read curoff:%d",curoff);
        logr("in read size %d",curnode->size);
        my_spinunlock(&plock);
#endif
            if(curoff > curnode->size){
                buf = NULL;
                return 0;
            }
            
            rbyte = 0;
            if(nbyte + curoff > curnode->size)
                rbyte = curnode->size - curoff;
            else
                rbyte = nbyte;
            ssize_t check = curdev->ops->read(curdev,curfs->fsoff+curfs->blooff+curnode->nblocks*FILEBLOCK+curoff,buf,rbyte);
            assert(check == rbyte);
        }
    }
    
    cur_task->fildes[fd]->offset = curoff+rbyte;
    return rbyte;
}


int vfswrite(int fd, void *buf, size_t nbyte){
    if(fd < 0){
        return -1;
    }
    uint64_t curoff = cur_task->fildes[fd]->offset;
    inode_t *curnode = cur_task->fildes[fd]->inode;
    ssize_t rbyte;
    if(curnode == NULL){//devfs
        char mntname[35], devname[32];
        memset(mntname,0,sizeof(mntname));
        memset(devname,0,sizeof(devname));
        mntname[0] = '/';
        int i;
        for(i = 1; i < strlen(cur_task->fildes[fd]->name); ++i){
            if(cur_task->fildes[fd]->name[i] == '/') break;
            mntname[i] = cur_task->fildes[fd]->name[i];
        }
#ifdef l3DEBUG
        my_spinlock(&plock);
        logr("mntname %s",mntname);
        my_spinunlock(&plock);
#endif
        assert(strcmp(mntname,"/dev") == 0);
        int j = 0;
        i++;//remove '/'
        for(;i < strlen(cur_task->fildes[fd]->name); ++i)
            devname[j++] = cur_task->fildes[fd]->name[i];
#ifdef l3DEBUG
        my_spinlock(&plock);
        logr("devname %s",devname);
        my_spinunlock(&plock);
#endif
        device_t *curdev = dev_lookup(devname);
        rbyte = curdev->ops->write(curdev,curoff,buf,nbyte); 
    
    }
    else{
        filesystem_t *curfs = curnode->fs;
        device_t *curdev = curfs->dev;
        if(curdev == NULL){//procfs
            rbyte = 0;
            if(nbyte + curoff > FILEBLOCK)
                rbyte = FILEBLOCK - curoff;
            else
                rbyte = nbyte;
            int num = curnode->offset;
            void* block = hgproc.block[num];
            memcpy(block+curoff,buf,rbyte);
            //update filesize
            if(hgproc.ino[num]->size < curoff + rbyte)
                hgproc.ino[num]->size = curoff + rbyte;
            
        }
        else{//blkfs
            rbyte = 0;
            if(nbyte + curoff > FILEBLOCK)
                rbyte = FILEBLOCK - curoff;
            else
                rbyte = nbyte;
            ssize_t check = curdev->ops->write(curdev,curfs->fsoff+curfs->blooff+curnode->nblocks*FILEBLOCK+curoff,buf,rbyte);
            assert(check == rbyte);
            //update filesize
            if(curnode->size < curoff + rbyte){
                curnode->size = curoff + rbyte;
                curdev->ops->write(curdev,curfs->fsoff+curnode->offset,(void*)curnode,sizeof(inode_t));
            } 
#ifdef l3DEBUG
        my_spinlock(&plock);
        logr("in write off :%d",curoff);
        logr("in write size :%d",curnode->size);
        my_spinunlock(&plock);
#endif
        }
    }

    cur_task->fildes[fd]->offset = curoff+rbyte;
    return rbyte;
}


int vfslseek(int fd, off_t offset, int whenc){
    #ifndef SHELL
        my_spinlock(&vfslock);
    #endif
    if(cur_task->fildes[fd] == NULL || fd < 0){
        my_spinlock(&plock);
        logr("The fd doesn't represent any file!");
        my_spinunlock(&plock); 
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return -1;
    }
    inode_t *node = cur_task->fildes[fd]->inode;
    if(node == NULL){//the file is in dev
       char mntname[35], devname[32];
       memset(mntname,0,sizeof(mntname));
       memset(devname,0,sizeof(devname));
       mntname[0] = '/';
       int i;
       for(i = 1; i < strlen(cur_task->fildes[fd]->name); ++i){
           if(cur_task->fildes[fd]->name[i] == '/') break;
           mntname[i] = cur_task->fildes[fd]->name[i];
       }
       assert(strcmp(mntname,"/dev") == 0);
        #ifndef SHELL
            my_spinunlock(&vfslock);
        #endif
        return 0;
    }
    else{//blkfs or procfs
         uint64_t curoff = cur_task->fildes[fd]->offset;
         inode_t* oldnode = cur_task->fildes[fd]->inode;
         
         size_t size = oldnode->size;
         size_t newoff = -1;
         if(whenc == LSEEK_CUR)
             newoff = curoff + offset;
         else if(whenc == LSEEK_SET){
             newoff = offset;
         }
         else if(whenc == LSEEK_END){
             newoff = size + offset;
         }
         
         if(newoff >= FILEBLOCK){
             my_spinlock(&plock);
             logr("The offset is out of bound!");
             my_spinunlock(&plock);
             #ifndef SHELL
                my_spinunlock(&vfslock);
             #endif
             return -1;
         }
         else if(newoff < 0){
             #ifndef SHELL
                my_spinunlock(&vfslock);
             #endif
            return -1;
         }
         else{
            cur_task->fildes[fd]->offset = newoff;
         }
    
    }

    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
    return offset;
}

int vfsclose(int fd){
    #ifndef SHELL
       my_spinlock(&vfslock);
    #endif
    if(fd < 0){
    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
        return -1;
    }
    if(cur_task->fildes[fd] == NULL){
    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
        return -1;
    }

    pmm->free(cur_task->fildes[fd]->inode);//free open node
    pmm->free(cur_task->fildes[fd]);
    cur_task->fildes[fd] = NULL;

    #ifndef SHELL
       my_spinunlock(&vfslock);
    #endif
    return 0;
}


void vfsinit(){
    for(int i = 0; i < MNTNUM; ++i){
        memset(mntpoint[i].name,0,sizeof(mntpoint[i].name));
        mntpoint[i].fs = NULL;
    }
    
    device_t *curdev = NULL;
    uintptr_t valByte;
    int cnt;
    inode_t *newnode = (inode_t*)pmm->alloc(sizeof(inode_t));
    ed_t *newed = (ed_t*)pmm->alloc(sizeof(ed_t));
#ifdef l3DEBUG
    logw("INOSIZE: %ld",sizeof(inode_t));
    logw("edt_tsize : %ld",sizeof(edt_t));
    logw("ed_tsize: %ld",sizeof(ed_t));
#endif
    // ----------------------------- blkfs -----------------------------//
    //create and init
    blkfs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if(blkfs == NULL){
       logr("fs alloc failed\n");
       assert(0);
    } 
    blkfs->ops->init = fsinit;
    blkfs->ops->lookup = fslookup;
    blkfs->ops->close = fsclose;
    curdev = dev_lookup("ramdisk1");
    blkfs->ops->init(blkfs,"blkfs",curdev);
    //update the root
    
    cnt = 0;
    valByte = -1;
    while(1){
        if(cnt == MAXFS) break;
        curdev->ops->read(curdev,cnt*RAMBLOCK,(void*)&valByte,4); 
#ifdef L3DEBUG
        logb("valByte : %ld\n",valByte);
#endif
        assert(valByte != -1);
        if(valByte == 0){
            blkfs->fsoff = cnt*RAMBLOCK;
            break;
        }
        else 
            cnt++;
    }
 
    if(cnt == MAXFS){
        logr("The ramdisk is full!");
        assert(0);
    }
    //update root
    valByte = 1;
    curdev->ops->write(curdev,blkfs->fsoff,(const void*)&valByte,4);
    blkfs->inooff[0] = 4;
    blkfs->blooff = blkfs->inooff[0] + INOSIZE; 
#ifdef L3DEBUG
    logb("fsoff : %lx",blkfs->fsoff);
    logb("inooff[0] : %lx",blkfs->inooff[0]);
    logb("blooff : %lx",blkfs->blooff);
#endif
    //the inode
    newnode->mod = O_RDWR;
    newnode->size = 0;
    newnode->nblocks = 0;
    newnode->offset = blkfs->inooff[0];
    newnode->refcnt = 0;
    newnode->ptr = (void *)curdev;
    newnode->fs = blkfs;
    newnode->ops = &inode_ops;

    curdev->ops->write(curdev,blkfs->fsoff+blkfs->inooff[0],(const void*)newnode,sizeof(inode_t)); 
    //in dir block
    
    memset(newed,0,sizeof(ed_t));
    curdev->ops->write(curdev,blkfs->fsoff+blkfs->blooff,(const void*)newed,sizeof(ed_t));

    //mnt
    if( vfs->mount("/",blkfs) == -1){
        logr("mount failed\n");
        assert(0);
    }
   
    //数据
    blkfs->ops->lookup(blkfs,"/jarvis",O_DIR|O_CREAT|O_RDWR);
    inode_t *t1 = blkfs->ops->lookup(blkfs,"/jarvis/test.c",O_CREAT|O_RDWR);
    blkfs->ops->lookup(blkfs,"/jarvis/lyrics",O_DIR|O_CREAT|O_RDWR);
   inode_t *t2 = blkfs->ops->lookup(blkfs,"/jarvis/lyrics/Try",O_CREAT|O_RDWR);
    inode_t *t3 = blkfs->ops->lookup(blkfs,"/jarvis/lyrics/Lucky",O_CREAT|O_RDWR);
    char buf[128];
    memset(buf,0,sizeof(buf));
    sprintf(buf,"int main(){\n    printf(\"Hello,world\");\n    return 0;\n}");
    blkfs->dev->ops->write(blkfs->dev,blkfs->fsoff+blkfs->blooff+t1->nblocks*FILEBLOCK,(void*)buf,strlen(buf)+1); 
    
    memset(buf,0,sizeof(buf));
    sprintf(buf,"If I walk would you run, If I stop would you come");
    blkfs->dev->ops->write(blkfs->dev,blkfs->fsoff+blkfs->blooff+t2->nblocks*FILEBLOCK,(void*)buf,strlen(buf)+1); 

    memset(buf,0,sizeof(buf));
    sprintf(buf,"Do you hear me? I'm talking to you");
    blkfs->dev->ops->write(blkfs->dev,blkfs->fsoff+blkfs->blooff+t3->nblocks*FILEBLOCK,(void*)buf,strlen(buf)+1); 

    // ------------------------------ devfs -----------------------------//
    //create and init
    devfs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if(devfs == NULL){
       logr("fs alloc failed\n");
       assert(0);
    } 
    devfs->ops->init = fsinit;
    devfs->ops->lookup = fslookup;
    devfs->ops->close = fsclose;
    devfs->ops->init(devfs,"devfs",NULL);
   //mnt
    if( vfs->mount("/dev",devfs) == -1){
        logr("mount devfs failed\n");
    }

    // ------------------------------- procfs -----------------------------// 
    //create and init
    procfs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if(procfs == NULL){
       logr("fs alloc failed\n");
       assert(0);
    }
    procfs->ops->init = fsinit;
    procfs->ops->lookup = fslookup;
    procfs->ops->close = fsclose;
    procfs->ops->init(procfs,"procfs",NULL);
   //mnt
    if( vfs->mount("/proc",procfs) == -1){
        logr("mount procfs failed\n");
    }
  
    //update the root
    memset(&hgproc,0,sizeof(hgproc));
    hgproc.ino[0] = (inode_t*)pmm->alloc(sizeof(inode_t));
    hgproc.block[0] = pmm->alloc(FILEBLOCK); 
    hgproc.ino[0]->mod = O_RDONLY;
    hgproc.ino[0]->size = 2*sizeof(edt_t);
    hgproc.ino[0]->nblocks = 0;
    hgproc.ino[0]->offset = 0;
    hgproc.ino[0]->refcnt = 0;
    hgproc.ino[0]->ptr= NULL;
    hgproc.ino[0]->fs= procfs;
    hgproc.ino[0]->ops= &inode_ops;
   //cpuinfo
    hgproc.ino[1] = (inode_t*)pmm->alloc(sizeof(inode_t));
    hgproc.block[1] = pmm->alloc(FILEBLOCK); 
    hgproc.ino[1]->mod = O_RDONLY;
    hgproc.ino[1]->size = 4;
    hgproc.ino[1]->nblocks = 1;
    hgproc.ino[1]->offset = 1;
    hgproc.ino[1]->refcnt = 0;
    hgproc.ino[1]->ptr= NULL;
    hgproc.ino[1]->fs= procfs;
    hgproc.ino[1]->ops= &inode_ops;
    int num = _ncpu() + 48;
    memcpy(hgproc.block[1],(void*)&num,sizeof(int));
    ed_t*rec = (ed_t*)hgproc.block[0];
    rec->etrs[0].valid = 1, rec->etrs[0].ino = 1, rec->etrs[0].ftype = EXT2_FT_REG_FILE;
    strncpy(rec->etrs[0].name,"cpuinfo",8);
    //meminfo
    hgproc.ino[2] = (inode_t*)pmm->alloc(sizeof(inode_t));
    hgproc.block[2] = pmm->alloc(FILEBLOCK); 
    hgproc.ino[2]->mod = O_RDONLY;
    hgproc.ino[2]->nblocks = 2;
    hgproc.ino[2]->offset = 2;
    hgproc.ino[2]->refcnt = 0;
    hgproc.ino[2]->ptr= NULL;
    hgproc.ino[2]->fs= procfs;
    hgproc.ino[2]->ops= &inode_ops;
    char tt[128];
    memset(tt,0,sizeof(tt));
    sprintf(tt,"MemTotal:   8031096 KB\nMemFree:   5068480 KB");
    hgproc.ino[2]->size = strlen(tt);
    memcpy(hgproc.block[2],(void*)tt,strlen(tt));
    rec = (ed_t*)hgproc.block[0];
    rec->etrs[1].valid = 1, rec->etrs[1].ino = 2, rec->etrs[1].ftype = EXT2_FT_REG_FILE;
    strncpy(rec->etrs[1].name,"meminfo",8);


    pmm->free(newnode);
    pmm->free(newed);

    return;
}

MODULE_DEF(vfs){
    .init = vfsinit,
    .access = vfsaccess,
    .mount = vfsmount,
    .unmount = vfsunmount,
    .mkdir = vfsmkdir,
    .rmdir = vfsrmdir,
    .link = vfslink,
    .unlink = vfsunlink,
    .open = vfsopen,
    .read = vfsread,
    .write = vfswrite,
    .lseek = vfslseek,
    .close = vfsclose,
};



