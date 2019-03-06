#include <am.h>
#include <amdev.h>

//#define SIDE 16
#define WHITE 1
#define BLACK 2
#define GRAY 0
#define wins 4

#define Log(format, ...)\
    printf("\33[1;34m[%s,%d,%s] " format "\33[0m\n",\
            __FILE__,__LINE__,__func__, ## __VA_ARGS__)


typedef struct cc{
    int cx,cy;
    int color;//the color of cursor at different time
    int chess;//the color of the chess of the cursor
}Cursor;

//up,down,left,right,lup,rdown,rup,ldown
const int dx[8] = {0,0,-1,1,-1,1,1,-1};
const int dy[8] = {-1,1,0,0,-1,1,-1,1};

static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}
