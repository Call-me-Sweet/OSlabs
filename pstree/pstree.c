#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

void do_pstree();
void do_print(pid_t p,int whe,char *blank);

int sel = 0;//n
int show = 0;//p
int ver = 0;//V

int main(int argc, char *argv[]) {
 // printf("Hello, World!\n");
  int i;
  assert(argv[0]);
  for (i = 1; i < argc; i++) {
    assert(argv[i]); // always true
   // printf("argv[%d] = %s\n", i, argv[i]);
    if(argv[1] == NULL)
        sel = 0;
    if((strcmp(argv[i],"-n")) == 0 || (strcmp(argv[i],"--numeric-sort")) == 0){
        sel = 1;
    }
    if((strcmp(argv[i],"-p")) == 0 || (strcmp(argv[i],"--show-pids")) == 0)
    {
        show = 1;
    }
    if((strcmp(argv[i],"-V")) == 0 || (strcmp(argv[i],"--version")) == 0)
    {
        ver = 1;
    }
  }
 
  do_pstree();

  assert(!argv[argc]); // always true
  return 0;
}

typedef struct Proc{
    char name[20];
    pid_t pid;
    struct Proc *khead; 
    struct Proc *ktail;
    struct Proc *next;
}proc;
proc nodes[400000];

void do_pstree()
{
  //first get the process id
    DIR *dir;
    struct dirent *ptr;
    FILE *fp = NULL;
    char filepath[300];
    char filetex[100];

    if(ver == 1)
    {
        printf("pstree\t1.0\nCopyright (C) 2019 Jarvis Zhang\n");
        printf("\n");
        printf("Parameter description:\n");
        printf("[-p]|[--show-pids]:\tshow the pid of process\n");
        printf("[-n]|[--numeric-sort]:\tshow the pstree in order of pid\n");
        printf("[-V]|[--version]:\tshow the version of pstree\n");
        printf("The default parameter is [-n]\n");
        printf("PS: This version supports multiple parameters like [-p -n -V] or [--show-pids --numeric-sort --version]\n    but doesn't support combined parameters like [-pnV]\n");
        return;
    }

    dir = opendir("/proc");
    if(NULL != dir)
    {
        while((ptr = readdir(dir)) != NULL)  // repeated reading
        {
           if(DT_DIR != ptr->d_type) continue;
           if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..")) == 0) continue;
           
           int flag=0;
           for(int i = 0; i < strlen(ptr->d_name); i++)
           {
               if(!isdigit(ptr->d_name[i]))
               {
                   flag = 1;
                   break;
               }
           }
           if(flag) continue;

         //open the status file
         //  printf("%s\n",ptr->d_name);
         sprintf(filepath, "/proc/%s/status", ptr->d_name);
         // printf("%s\n",filepath);
           
           fp = fopen(filepath,"r");
           if(fp == NULL)
           {
              //printf("No such a file!\n");
               continue;//the process is killed meanwhile;
               //assert(0);
           }

           char name[30];
           while(fscanf(fp,"Name:\t%s",name) != 1)
               fgets(filetex,sizeof(filetex),fp);
         //  printf("%s\n",name);
           pid_t pid;
           while(fscanf(fp,"Pid:\t%d",&pid) != 1)
               fgets(filetex,sizeof(filetex),fp);
         //  printf("%d\n",pid);
           pid_t ppid;
           while(fscanf(fp,"PPid:\t%d",&ppid) != 1)
               fgets(filetex,sizeof(filetex),fp);
         //  printf("%d\n",ppid);

           if(sel == 0)  // means no -n 
           {
               // construct a tree
               strcpy(nodes[pid].name,name);
               nodes[pid].pid = pid;
               if(ppid <= 0) continue;

               if(nodes[ppid].khead  ==  NULL)
               {
                   nodes[ppid].khead = nodes[ppid].ktail = &nodes[pid];
               }
               else 
               {
                   proc *ind = nodes[ppid].khead;
                   while(ind != NULL)
                   {
                       if(ind->next != NULL)
                       {
                           if((strcmp(ind->name,nodes[pid].name)) <= 0 && (strcmp(ind->next->name,nodes[pid].name)) >= 0)
                           {
                               nodes[pid].next = ind->next;
                               ind->next = &nodes[pid];
                               break;
                           }
                       }
                       else // now ind = ktail 
                       {
                           if(strcmp(ind->name,nodes[pid].name) <= 0){
                               nodes[pid].next = ind->next;
                               ind->next = &nodes[pid];
                               nodes[ppid].ktail = &nodes[pid];
                               break;
                           }
                           else{ // now ind = ktail = khead
                               nodes[pid].next = ind;
                               nodes[ppid].khead = &nodes[pid];
                               break;
                           }
                       }
                       ind = ind->next;
                   }
               }
           }

           if(sel == 1) //means it's -n
           {
               strcpy(nodes[pid].name,name);
               nodes[pid].pid  = pid;
               if(ppid <= 0) continue;

               if(nodes[ppid].khead == NULL)
               {
                    nodes[ppid].khead = nodes[ppid].ktail = &nodes[pid];
               }
               else
               {
                   nodes[ppid].ktail->next = &nodes[pid];
                   nodes[ppid].ktail = &nodes[pid];
               }
           }
           fclose(fp);
           
           DIR *dirr;
           struct dirent *ptrr;
           char taskpath[300];
           
           sprintf(taskpath,"/proc/%s/task",ptr->d_name);
           dirr = opendir(taskpath);
           if(NULL == dirr) continue;
           while((ptrr = readdir(dirr)) != NULL)
           {
               if((strcmp(ptrr->d_name,".")) == 0 || (strcmp(ptrr->d_name,"..")) == 0) continue;
               if((strcmp(ptrr->d_name,ptr->d_name)) == 0) continue;
               
               pid_t tpid = atoi(ptr->d_name);
               pid_t Pid = atoi(ptrr->d_name);

               char file[600];
               sprintf(file,"/proc/%s/task/%s/status",ptr->d_name,ptrr->d_name);
               fp = fopen(file,"r");
               if(fp == NULL)
               {
                  // printf("No such a file!\n");
                  // assert(0);
                  continue;
               }

               char nn[30];
               while(fscanf(fp,"Name:\t%s",nn) != 1)
                   fgets(filetex,sizeof(filetex),fp);
               char proname[40];
               sprintf(proname,"{%s}",nn);

               if(sel == 0)
               {
                   strcpy(nodes[Pid].name,proname);
                   nodes[Pid].pid = Pid;
                   
                   if(nodes[tpid].khead == NULL)
                     nodes[tpid].khead = nodes[tpid].ktail = &nodes[Pid];
                   else
                   {
                       proc *ind = nodes[tpid].khead;
                       while(ind != NULL)
                       {
                           if(ind->next != NULL)
                           {
                               if((strcmp(ind->name,nodes[Pid].name)) <= 0 && (strcmp(ind->next->name,nodes[Pid].name)) >= 0)
                               {
                                   nodes[Pid].next = ind->next;
                                   ind->next = &nodes[Pid];
                                   break;
                               }
                           }
                           else
                           {
                               if(strcmp(ind->name,nodes[Pid].name) <= 0)
                               {
                                   nodes[Pid].next = ind->next;
                                   ind->next = &nodes[Pid];
                                   nodes[tpid].ktail = &nodes[Pid];
                                   break;
                               }
                               else
                               {
                                   nodes[Pid].next = ind;
                                   nodes[tpid].khead = &nodes[Pid];
                                   break;
                               }
                           }
                           ind = ind->next;
                       }
                   }
               }

               if(sel == 1)
               {
                   strcpy(nodes[Pid].name,proname);
                   nodes[Pid].pid = Pid;

                   if(nodes[tpid].khead == NULL)
                       nodes[tpid].khead = nodes[tpid].ktail = &nodes[Pid];
                   else 
                   {
                       nodes[tpid].ktail->next = &nodes[Pid];
                       nodes[tpid].ktail = &nodes[Pid];
                   }
               }
              fclose(fp);
           }
          closedir(dirr); 
        }
    }
    else
        assert(0); //shouldnot reach here
    //print the tree
    do_print(1,1,"");
    closedir(dir);
}

void do_print(pid_t p,int whe,char *blank)
{
    int len = 0;//record how many bit 
    if(whe == 0)//the same line is tight,whe = 0 means not the same line
       printf("%.*s",(int)strlen(blank)-1,blank);//except the line
    if(p == 1)//the root neednot draw lines before
    {
       len += printf("%s",nodes[p].name);
       if(show == 1)
       {
           len += printf("(%d)",p);
       }
       if(nodes[p].khead == NULL)
           printf("\n");
    }
    else
    {
      if(whe == 1) //whether in the same line with its father
      {
         if(nodes[p].next == NULL)//no brother
         {
             int l = strlen(blank);
             blank[l-1]=' ';//reduce the extra line
             printf("———"); }
         else
            printf("—|—");
      }
      else if(whe == 0)
      {
          if(nodes[p].next == NULL)//no brother
          {
              int l = strlen(blank);
              blank[l-1]=' ';
              printf("|__");
          }
          else
              printf("|——");
      }

      len += printf("%s",nodes[p].name);
      if(show == 1)
      {
          len += printf("(%d)",p); 
      }
      if(nodes[p].khead == NULL)
          printf("\n");
    }

// construct new blank
    char NB[500]="";
    char tem[100]="";
    strcpy(NB,blank);
    for(int i=0;i<len+3;++i)
    {
        tem[i]=' ';
    }
    tem[len+2]='|';
    strcat(NB,tem);

    proc *ind = nodes[p].khead;
    
    while(ind != NULL)
    {
        if(ind == nodes[p].khead)
           do_print(ind->pid,1,NB);
        else
           do_print(ind->pid,0,NB);
        ind = ind->next;
    }
}

