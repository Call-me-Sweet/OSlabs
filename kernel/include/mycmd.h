#include <common.h>
#include <vfs.h>
#include <devices.h>
//#define shell
void cmd_ls(int num, char ** pars, char *curpath, char *buf);
void cmd_cat(int num, char ** pars, char *curpath, char *buf);
void cmd_cd(int num, char ** pars, char *curpath, char *buf);
void cmd_mkdir(int num, char ** pars, char *curpath, char *buf);
void cmd_rmdir(int num, char ** pars, char *curpath, char *buf);
void cmd_rm(int num, char ** pars, char *curpath, char *buf);
void cmd_link(int num, char ** pars, char *curpath, char *buf);
void cmd_touch(int num, char ** pars, char *curpath, char *buf);
void cmd_echo(int num, char ** pars, char *curpath, char *buf);
void cmd_mnt(int num, char ** pars, char *curpath, char *buf);
void cmd_unmnt(int num, char ** pars, char *curpath, char *buf);
