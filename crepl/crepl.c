#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dlfcn.h>
//#define DEBUG

char c;
char txt[200];
char ntxt[3000];
int cnt = 0;
int cntexpr = 0;
int flag;//0 means func, 1 means expr

char arg[20][100];
char *nargv[20];//must static
struct H{
    void *handle;
}handler[500];
int cnth = 0;

struct F{
    char name[20];
    int fd;
}ff[500];
int cntf = 0;

struct N{
    char fname[200];
}sfunc[500];
int cntsf = 0;
extern char **environ;

void init(){
    strcpy(arg[0],"gcc");
    strcpy(arg[1],"-x");
    strcpy(arg[2],"c");
    //3 is var
    strcpy(arg[4],"-fPIC");
    strcpy(arg[5],"-shared");
    if(sizeof(long) == 4)//32
        strcpy(arg[6],"-m32");
    else if(sizeof(long) == 8)//64
        strcpy(arg[6],"-m64");
    else assert(0);
    strcpy(arg[7],"-Werror=implicit-function-declaration");
    strcpy(arg[8],"-o");
    //7 is var
}

void myclean(){
    for(int i = 0; i < cnth; ++i){
        dlclose(handler[i].handle);
    }
    for(int i = 0; i < cntf; ++i){
        unlink(ff[i].name);
        close(ff[i].fd);
    }
    system("find . -name \"func*\" | xargs rm");
}

int main(int argc, char *argv[]) {
    init();
    atexit(myclean);
    // func or expr
    while(1){
        write(STDERR_FILENO,">>> ",4);
      
        int finish = 0;//0 means expr isnot finished, otherwise it's 1
        char juge[10];
        int cnttem = 0;
        memset(juge,0,sizeof(juge));
        while(read(STDIN_FILENO,&c,1) > 0){
            if((c == '\n') && (strcmp(juge,"EOF") != 0)){
                flag = 1;
                finish = 1;
                strcpy(txt,juge);
                cnt = cnttem;
                break;
            }
            if( (cnttem == 3) && (strcmp(juge,"EOF") == 0) )
                return 0;
            else if(cnttem == 3){
                if((strcmp(juge,"int") == 0) && c == ' '){
                      flag = 0;
                }
                else
                    flag = 1;
                juge[cnttem++] = c;//the first cha of name
                strcpy(txt,juge);
                cnt = cnttem;
                break;
            }
            else
                juge[cnttem++] = c;
        }
      
        if(finish == 0){
            while(read(STDIN_FILENO,&c,1) > 0 ){
                if(c == '\n')//get the whole
                    break;
                else if(c == '{'){
                    strcpy(sfunc[cntsf++].fname,txt);
                    txt[cnt++] = c;
                }
                else 
                    txt[cnt++] = c;
            }
        }
       
        strcpy(ff[cntf++].name,"funcXXXXXX");
        ff[cntf-1].fd = mkstemp(ff[cntf-1].name);

       // char template[] = "funcXXXXXX";
       // int fd = mkstemp(template);
       char temp[100];
       for(int i = 0; i < cntsf; ++i){
           memset(temp,0,sizeof(temp));
           sprintf(temp,"extern %s; ",sfunc[i].fname);
           strcat(ntxt,temp);
       }
        

        if(flag == 1){
            memset(temp,0,sizeof(temp));
            sprintf(temp,"int __expr_wrap_%d() {return (%s);}",cntexpr++,txt);
            strcat(ntxt,temp); 
        }
        else
           strcat(ntxt,txt);

        write(ff[cntf-1].fd,ntxt,strlen(ntxt));
        //begin to compile
        // gcc -x c name(template) -fPIC -shared -m32/64 -o name(template.so)
                
        sprintf(arg[3],"%s",ff[cntf-1].name);
        sprintf(arg[9],"%s.so",ff[cntf-1].name);
        for(int i = 0; i <= 9; ++i)
            nargv[i] = arg[i];

        int pid;//inside main       
        pid  = fork();
        if(pid == -1){perror("fork"); exit(1);}
                
        if(pid == 0){//kid
           execve("/usr/bin/gcc",nargv,environ);
           assert(0);
        }
        else{//father
            int sta;
            wait(&sta);

            if((sta == 0) && (flag == 0)){//cout add
              write(STDERR_FILENO,"Added: ",7);
              write(STDERR_FILENO,txt,strlen(txt));
              write(STDERR_FILENO,"\n",1);
            }

            char path[50];
            memset(path,0,sizeof(path));
            sprintf(path,"./%s.so",ff[cntf-1].name);
            handler[cnth++].handle = dlopen(path,RTLD_LAZY|RTLD_GLOBAL);

            if(!handler[cnth-1].handle){
                fprintf(stderr,"%s\n",dlerror());
                exit(EXIT_FAILURE);
            }

            if(flag == 1){
                 char  funname[50];
                 memset(funname,0,sizeof(funname));
                 sprintf(funname,"__expr_wrap_%d",cntexpr-1);

                 int (*ex_fun)();
                 ex_fun = (int (*)())dlsym(handler[cnth-1].handle,funname);
                 int val = ex_fun();
                 fprintf(stderr,"(%s) == %d\n",txt,val);
                 dlclose(handler[--cnth].handle);
            }

            memset(arg[3],0,sizeof(arg[3]));
            memset(arg[9],0,sizeof(arg[9]));
            cnt = 0;
            memset(txt,0,sizeof(txt));
            memset(ntxt,0,sizeof(ntxt));
        }
    
    }
    return 0;
}
