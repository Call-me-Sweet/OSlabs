#include <game.h>
#include "klib.h"

void init_screen();
void splash();
void read_keys();
void init_cursor();
void update_screen();
void do_key(int key_code);
void draw_rects(int x, int y, int w, int h, uint32_t color);
void check();


int chess_color[240][240];//the color of chess[x][y]C
Cursor cur[2];//0 means white,1 means black
int turn;//0 means white,1 means black
int begins;//1 means begin;
int flag;//1 means WHITE wins,2 means BLACK wins,0 continue
const uint32_t gray = 0x9d8f86;
const uint32_t line = 0x9d8f86 ^ 0xffffff;//the color of lines on chessboard
int SIDE;// the size of block on chessboard

int cntchess;//cnt how many chess_pieces already on the chessboard
int sum;// record how many "seats" on the chessboard
int cntbegin;//cnt how many tims the beginning tips puts(we just let it be put once)
int w, h;//record the w and h of the screen
int stop;// 1 means the game is over or the chessboard is full
int R;//reset the game

int main() {
  // Operating system is a C program
  _ioe_init();

  while(1){
  
    init_screen();
    splash();
    init_cursor();

    unsigned long long current;
    unsigned long long last = 0;

    while (1) {
         read_keys();

        if(!begins) {
             if(!cntbegin){
                Log("Welcome to the Quadrate-Gomoku designed by Zhang Lingyu.");
                Log("Please press space key to begin the game!");
                Log("Press up, down, left, right key to control the cursors");
                Log("Press space key to put the chess where the cursor is");
                cntbegin = 1;
            }
            continue;
        }

        if(stop){
             if(R)
                break;
            else
                continue;
        }//the game is over;

        //control the twinkling of cursor;
        current = uptime();
        if(current - last > 500){//every 500ms 
             switch(turn){
                 case 0: cur[turn].color = (cur[turn].color == WHITE ? GRAY : WHITE); break;//WHITE chess
                case 1: cur[turn].color = (cur[turn].color == BLACK ? GRAY : BLACK); break;//BLACK chess
            }
            last = current;
        }

        update_screen();
     
        //try to find the winner
        check();
    
        if(flag == 1){
             Log("WHITE wins the game!");
             Log("Press R to have a new game again!");
             stop = 1;
        }
        else if(flag == 2){
             Log("BLACK wins the game!");
             Log("Press R to have a new game again!");
             stop = 1;
        }

        if(cntchess == sum && !flag){
             Log("The chessboard is full but the dust is still settling!");
             Log("Press R to have a new game again!");
             stop = 1; 
        }
    }
 }
  return 0;
}




void read_keys() {
  _DEV_INPUT_KBD_t event = { .keycode = _KEY_NONE };
  #define KEYNAME(key) \
    [_KEY_##key] = #key,
 /* static const char *key_names[] = {
    _KEYS(KEYNAME)
  };*/
  _io_read(_DEV_INPUT, _DEVREG_INPUT_KBD, &event, sizeof(event));
  if (event.keycode != _KEY_NONE && event.keydown) {
    //puts("Key pressed: ");
    //puts(key_names[event.keycode]);
    //puts("\n");
    //printf("%d\n",event.keycode);
    do_key(event.keycode);
  }
}


void init_screen() {
  _DEV_VIDEO_INFO_t info = {0};
  _io_read(_DEV_VIDEO, _DEVREG_VIDEO_INFO, &info, sizeof(info));
  w = info.width;
  h = info.height;
  printf("w:%d\nh:%d\n",w,h);
  SIDE = 20;

  //find the best size of SIDE
  for(int i = 21; i <= 32; ++i)
      if(w%i == 0&& h%i == 0)
      {
          SIDE = i;
          sum = (w/i)*(h/i);
          break;
      }
  printf("SIDE:%d\n",SIDE);
}

void draw_rects(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: allocated on stack
  _DEV_VIDEO_FBCTL_t event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
   // pixels[i] = color;
   //chess
   int tem = i/w;//which line
   if(tem > 0 ){
      if( (i >= (tem)*w+w/2-tem) && (i <= (tem)*w+w/2+tem-2) && (i < w*(h/2+1)))
         pixels[i] = color;
      else if((i >= w*(h/2+1)) && (i >= ((tem)*w-w/2+tem)) && (i <= ((tem)*w+3*w/2-tem-3)))
          pixels[i] = color;
      else
         pixels[i] = gray;
   }
   else
       pixels[i] = gray;

   //lines
    if(i > (h-1)*w-1)
        pixels[i] = line;
    
    if((i+1)%w == 0)
        pixels[i] = line;
    //
  }
  _io_write(_DEV_VIDEO, _DEVREG_VIDEO_FBCTL, &event, sizeof(event));
}

void splash() {
  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
   //   if ((x & 1) ^ (y & 1)) {
        draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0x9d8f86); //gray 
     // }
    }
  }
  memset(chess_color,GRAY,sizeof(chess_color));
}





void init_cursor(){
   cur[0].cx = cur[0].cy = 0;
   cur[0].color = cur[0].chess = WHITE;
   cur[1].cx = cur[1].cy = 0;
   cur[1].color = cur[1].chess = BLACK;
   turn = 0;
   //reset the game
   begins = 0;
   flag = 0;
   cntchess = 0;
   cntbegin = 0;
   stop = 0;
   R = 0;
}

void update_screen(){
    for (int x = 0; x * SIDE <= w; x ++) {
       for (int y = 0; y * SIDE <= h; y++) {
           if(x != cur[turn].cx || y != cur[turn].cy){
               switch(chess_color[x][y]){
                        case GRAY: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0x9d8f86); break;//gray
                        case WHITE: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); break;//white 
                        case BLACK: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0x000000); break;//black
               }
           }
           else{
               switch(cur[turn].color){
                        case GRAY: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0x9d8f86); break;//gray
                        case WHITE: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); break;//white 
                        case BLACK: draw_rects(x * SIDE, y * SIDE, SIDE, SIDE, 0x000000); break;//black
               } 
           }
       }
     }
     
}


void do_key(int key_code){
    if(!begins){
        if(key_code != 70)
            return;
        else{
            begins = 1;
            return;  
        }
    }
    else{
        switch(key_code){
            case 32: if(stop) {R = 1;} break;
            case 73: cur[turn].cy -= (cur[turn].cy == 0 ? 0 : 1); break;//up
            case 74: cur[turn].cy += (cur[turn].cy == h/SIDE-1 ? 0 : 1); break;//down
            case 75: cur[turn].cx -= (cur[turn].cx == 0 ? 0 : 1);break;//left
            case 76: cur[turn].cx += (cur[turn].cx == w/SIDE-1 ? 0 : 1); break;//right
            case 70: {
                         if(chess_color[cur[turn].cx][cur[turn].cy] == GRAY)
                         { 
                             chess_color[cur[turn].cx][cur[turn].cy] = cur[turn].chess; 
                             cntchess++;
                             turn = (turn == 0 ? 1 : 0); 
                         }
                         else
                         {
                             if(!stop)
                                Log("You cannot put a chess at here!");
                         }
                       //  cur[turn].cx = cur[turn].cy = 0; 
                         break;  
                     }
        }
    }
        
}

void check(){//already change turn ,so we need to check turn ^ 1
    int ch_turn = turn ^ 1;
    int wcnt[8];
    memset(wcnt,0,sizeof(wcnt));
    int tx,ty;
    
    for(int i = 0; i < 8; ++i){//dx[i],dy[i]choose the direction
        tx = cur[ch_turn].cx, ty = cur[ch_turn].cy;
        while(wcnt[i] < wins){
            tx += dx[i], ty += dy[i];
            if(tx < 0 || tx > w/SIDE-1 || ty < 0 || ty > h/SIDE-1)
                break;
            if(chess_color[tx][ty] == cur[ch_turn].chess)
                wcnt[i]++;
            else
                break;
        }
        if(wcnt[i] == wins){
            flag = (cur[ch_turn].chess == WHITE ? 1 : 2);
            return;
        }
        
        if((i % 2 == 1) && (wcnt[i] + wcnt[i-1] >= wins)){
            flag = (cur[ch_turn].chess == WHITE ? 1 : 2);
            return;
        }
    }
}
