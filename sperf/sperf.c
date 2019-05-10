#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#include <assert.h>
#include <time.h>
//#define DEBUG

typedef struct table{
    char name[20];
    double t;
    double per;
}TAB;
TAB tab[100];
int sys_num = 0;
double sum = 0;

int fildes[2];
char *nargv[50];
char new_argv[50][1000];
int pid;
char oldtmp[20];

clock_t last;
extern char **environ;

double findtime(char *s){
    double ret = 0;
    int status = 0;
    int flag = REG_EXTENDED;
    regmatch_t pmatch[1];
    const size_t nmatch = 1;
    const char *pattern2 = "<[0-9|\\.]+>$";
    regex_t reg;
    //find time
    regcomp(&reg,pattern2,flag);
    status = regexec(&reg, s, nmatch, pmatch, 0);
    if(status == REG_NOMATCH){
     //  printf("%s\n",s);
      // assert(0);
       ret = -1;
       return ret;
    }
    else if(status == 0){//success
         char tmp[20];
         memset(tmp,0,sizeof(tmp));
         int ind = 0;
         for(int i = pmatch[0].rm_so + 1; i < pmatch[0].rm_eo - 1; ++i){
               tmp[ind++] = s[i];
         }
         ret = atof(tmp);
    }
   return ret;
}

void match(char * s){
   int status = 0;
   int flag = REG_EXTENDED;
   regmatch_t pmatch[1];
   const size_t nmatch = 1;
   const char *pattern1 = "^[A-z_]+";
   regex_t reg;
   //get the name
   regcomp(&reg, pattern1, flag);
   status = regexec(&reg, s, nmatch, pmatch, 0);
 
   if(status == REG_NOMATCH){
        return;
   }
   else if(status == 0){//success
      char tmp[20];
      memset(tmp,0,sizeof(tmp));
      int ind = 0;
      for(int i = pmatch[0].rm_so; i < pmatch[0].rm_eo; ++i){
            tmp[ind++] = s[i];
      }
      
      if(sys_num == 0){
           double tt = findtime(s);
           if(tt >= 0){
                strcpy(tab[sys_num].name,tmp);
                tab[sys_num].t = findtime(s);
                sum +=tab[sys_num].t;
                sys_num++;
           }
      }
      else{
          int i;
          for(i = 0; i < sys_num; ++i){
              if(strcmp(tmp,tab[i].name) == 0){
                  double tt = findtime(s);
                  if(tt >= 0){
                     tab[i].t += tt;
                     sum += tt;
                  }
                  break;
              }
          }
          if(i == sys_num){//a new name
              double tt = findtime(s);
              if(tt >= 0){
                  strcpy(tab[sys_num].name,tmp);
                  tab[sys_num].t = findtime(s);
                  sum += tab[sys_num].t;
                  sys_num++;
              } 
          }
      }
   }
}

int mycmp(const void* a,const void* b){
    TAB* aa = (TAB*)a;
    TAB* bb = (TAB*)b;
    return (aa->t > bb->t ? -1 : 1);
}

int main(int argc, char *argv[]) {
    //argv[1] is cmd and the left are args    
    #ifdef DEBUG
    for(int i = 0; i < argc; ++i){
        printf("%s\n",argv[i]);
    }
    #endif
       
    strcpy(new_argv[0],"./strace");
    strcpy(new_argv[1],"-T");
    for(int i = 1; i < argc; ++i)
        strcpy(new_argv[i+1],argv[i]);
     
     #ifdef DEBUG
     for(int i = 0; i <= argc; ++i)
         printf("%s\n",new_argv[i]);
     #endif

     for(int i = 0; i <= argc; ++i)
         nargv[i] = new_argv[i];

     last = clock();
     if(pipe(fildes) != 0) { perror("pipe"); exit(1); }
     pid = fork();
     if(pid < 0){ perror("fork"); exit(1); }
     
     if(pid == 0){//kid
        dup2(fildes[1], STDERR_FILENO);
        int tmp = open("/dev/null",O_WRONLY);
        dup2(tmp,STDOUT_FILENO);//strace 中调用的进程的输出
        close(fildes[0]);
        execve("/usr/bin/strace",nargv,environ); 
        
        close(tmp);
        close(fildes[1]);
     }
     else{
        dup2(fildes[0],STDIN_FILENO);
        close(fildes[1]);
        char c;
        char s[1000];
        memset(s,0,sizeof(s));
        int cnt = 0;
        while(read(fildes[0],&c,1)>0){//getchar
            if(c == '\n'){
             //   printf("%s\n",s);
                match(s);
                cnt = 0;
                memset(s,0,sizeof(s));
           
                clock_t now = clock();
                if(now - last > 10000){
                    system("tput clear");
                    for(int i = 0; i < sys_num; ++i){
                        double tmp = tab[i].t/sum*100;
                        tab[i].per = tmp;
                    }
                    qsort(tab,sys_num,sizeof(tab[0]),mycmp);
                
                    printf("Syscall             Time(s)\t\tpercent\n");
                    printf("--------------      --------\t\t-------\n");
                    for(int i = 0; i< sys_num; ++i){
                    printf("%-20s%-.6lf\t\t%4.2lf%%\n",tab[i].name,tab[i].t,tab[i].per);
                    }
                    printf("--------------      --------\t\t-------\n");
                    printf("Total               %-.6lf\t\t100%%\n",sum);
                    last = now; 
                }
            }
            else{
                s[cnt++] = c;
            }
        }
    
       system("tput clear");
       for(int i = 0; i < sys_num; ++i){
            double tmp = tab[i].t/sum*100;
            tab[i].per = tmp;
       }
       qsort(tab,sys_num,sizeof(tab[0]),mycmp);
       
       printf("Syscall             Time(s)\t\tpercent\n");
       printf("--------------      --------\t\t-------\n");
       for(int i = 0; i< sys_num; ++i){
           printf("%-20s%-.6lf\t\t%4.2lf%%\n",tab[i].name,tab[i].t,tab[i].per);
       }
       printf("--------------      --------\t\t-------\n");
       printf("Total               %-.6lf\t\t100%%\n",sum);

       close(fildes[0]);
     }

    return 0;
}
