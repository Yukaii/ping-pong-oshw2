#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

// constant
#define DELAY 40000 
const char ball_styles[] = {"o"};
// Game Flags
int LOSE_FLAG_1 = 0;
int LOSE_FLAG_2 = 0;
int END_FLAG = 0;
int PRINT_PARAM_FLAG = 0;
int PAUSE_FLAG = 0;
int DISPLAY_HELP = 0;

// global variables 
int key_read = ' ';

// game parameters
int serve_dir = -1;
float speed_state = 0;


typedef struct _vector2 {
  float x;
  float y;
}Vector2;

typedef struct _ball {
  Vector2 now, next, velocity, direction;
  float speed;
  int bound_x, bound_y;
} Ball;

typedef struct _racket
{
  int width;
  int center;
  int bound;
  // 0 or right end
  int side;
} Racket;

void* keyboard_listen();
void draw_field(WINDOW *screen);
int _kbhit(void);
void draw_ball(WINDOW* screen, Ball b);
void move_ball(Ball* b, Racket racket_1, Racket racket_2);
void draw_racket(WINDOW* screen, Racket racket, int bound);
void move_racket(Racket* racket, int dist);
void print_win_str(WINDOW* screen, char* spaces);

int main(int argc, char *argv[]) {
  // window params
  int parent_x, parent_y, new_x, new_y;
  
  // game objects
  Ball ball_1, ball_2;
  Racket racket_1, racket_2;
  int player1_score = 0;
  int player2_score = 0;

  // initial setups
  initscr();
  noecho();
  cbreak();
  curs_set(FALSE);
  keypad(stdscr, TRUE);
  nodelay(stdscr, FALSE);
  getmaxyx(stdscr, parent_y, parent_x);
  WINDOW *field = newwin(parent_y, parent_x, 0, 0);
  wrefresh(field);

RE:
  // setup rackets
  racket_1.width = 7;
  racket_1.center = parent_y / 2;
  racket_1.bound = parent_y;
  racket_1.side = 0;
  racket_2.width = 7;
  racket_2.center = parent_y / 2;
  racket_2.bound = parent_y;  
  racket_2.side = parent_x;

START:
  // setup balls
  ball_1.now.y = parent_y / 2; ball_1.now.x = parent_x / 2;
  ball_1.direction.x = serve_dir; ball_1.direction.y = 1;
  ball_1.speed = 0.7;
  // ball_1.velocity.x = velocity_x; ball_1.velocity.y = velocity_y;
  ball_1.bound_x = parent_x; ball_1.bound_y = parent_y-1;

  // 沒有使用 pthread 因為反應慢orz...
  // pthread_t keyboard_read1, keyboard_read2;
  // int ret1 = pthread_create(&keyboard_read1, NULL, keyboard_listen, NULL);

  while(1)
  {
    wclear(field);

    if (LOSE_FLAG_1) {
       serve_dir = -1;
       player2_score += 1;
       LOSE_FLAG_1 = 0;
       goto START;
    }
    if (LOSE_FLAG_2) {
       serve_dir = 1;
       player1_score += 1;
       LOSE_FLAG_2 = 0;
       
       goto START;
    } 


    // // yet another non-blocking usage, no need using pthread ya~
    if (_kbhit()) {
      key_read = getch();
      // move rackets
      if (key_read == 'q' || key_read == 'Q') break;
      if (key_read == 'd') racket_1.width += 1;
      if (key_read == 'a') racket_1.width -= 1;
      if (key_read == 'l') racket_2.width += 1;
      if (key_read == 'j') racket_2.width -= 1;         
      if (key_read == '+' || key_read == '=') { ball_1.speed += 0.1; }
      if (key_read == '-') { ball_1.speed -= 0.1; }
      if (key_read == 'p') PRINT_PARAM_FLAG = !PRINT_PARAM_FLAG;
      if (!PAUSE_FLAG) {
        if (key_read == 'w') move_racket(&racket_1, -2);
        if (key_read == 's') move_racket(&racket_1, 2);
        if (key_read == 'i') move_racket(&racket_2, -2);
        if (key_read == 'k') move_racket(&racket_2, 2); 
      }
      if (key_read == 'h' || key_read == '`') {
        if (!PAUSE_FLAG) {
          speed_state = ball_1.speed;
          ball_1.speed = 0;
          PAUSE_FLAG = !PAUSE_FLAG;
          if (key_read == 'h')
            DISPLAY_HELP = !DISPLAY_HELP;
        }
        else {
          ball_1.speed = speed_state;
          PAUSE_FLAG = !PAUSE_FLAG;
          if (key_read == 'h')
            DISPLAY_HELP = !DISPLAY_HELP;
        }
      }
    }
    
    // make board -------------
    getmaxyx(stdscr, new_y, new_x);
    // if the screen resize
    if (new_y != parent_y || new_x != parent_x)
    {
      parent_y = new_y;
      parent_x = new_x;

      ball_1.bound_x = parent_x; ball_1.bound_y = parent_y-1;
      racket_1.side = 0; racket_2.side = parent_x;

      wresize(field, new_y, new_x);
      wclear(stdscr);
    }
    draw_field(field);
    
    //-------------------------
    // print game score
    mvwprintw(field, 3, parent_x/2 - 4, "%3d", player1_score);
    mvwprintw(field, 3, parent_x/2+1, "%2d", player2_score );    

    // print game params XD
    if (PRINT_PARAM_FLAG) {
      mvwprintw(field, parent_y-4, 2, " width: %2d", racket_1.width);
      mvwprintw(field, parent_y-4, parent_x/2 + 2, " width: %2d", racket_2.width);
      mvwprintw(field, parent_y-3, 2, "speed: %.2f", ball_1.speed );
    }
    if (DISPLAY_HELP) {
      mvwprintw(field, 5, 3, "1p");
      mvwprintw(field, 6, 3, "  up: w");
      mvwprintw(field, 7, 3, "  down: s");
      mvwprintw(field, 8, 3, "  increase: d");
      mvwprintw(field, 9, 3, "  decrease: a");

      mvwprintw(field, 4, parent_x - 15, "2p");
      mvwprintw(field, 5, parent_x - 15, "  up: i");
      mvwprintw(field, 6, parent_x - 15, "  down: k");
      mvwprintw(field, 7, parent_x - 15, "  increase: j");
      mvwprintw(field, 8, parent_x - 15, "  decrease: l");      

      mvwprintw(field, 12, parent_x/2 - 5, "speed up: +");
      mvwprintw(field, 13, parent_x/2 - 6, "slow down: -");
      mvwprintw(field, 14, parent_x/2 - 10, "display this help: h");
      mvwprintw(field, 15, parent_x/2 - 8, "display param: p");
      mvwprintw(field, 16, parent_x/2 - 4, "pause: `");
    }

    draw_ball(field, ball_1);
    move_ball(&ball_1, racket_1, racket_2);

    draw_racket(field, racket_1, 0);
    draw_racket(field, racket_2, parent_x-1);

    wrefresh(field);

    if (player1_score == 10 || player2_score == 10) {
      char spaces[parent_x/2+2];
      int i;
      for (i = 0; i < parent_x/2+2; i++) spaces[i] = '*';      
      if (player1_score == 10){
        spaces[6] = '\0';
      } 
      print_win_str(field, spaces);
      sleep(5);
      // flush input buffer :D
      fseek(stdin,0,SEEK_END);
      key_read = getch();
      player1_score = 0; 
      player2_score = 0;          
      goto RE;
    }

    usleep(DELAY);
  }
END:

  delwin(field);
  endwin();
  return 0;
}

void print_win_str(WINDOW* screen, char* spaces) {
  mvwprintw(screen, 1, 0, "%s____    __    ____  __  .__   __. \n%s\\   \\  /  \\  /   / |  | |  \\ |  | \n%s \\   \\/    \\/   /  |  | |   \\|  | \n%s  \\            /   |  | |  . `  | \n%s   \\    /\\    /    |  | |  |\\   | \n%s    \\__/  \\__/     |__| |__| \\__|", spaces, spaces, spaces, spaces, spaces, spaces);
  mvwprintw(screen, 9, 4, "wait for 5s and press any key to restart...");
  wrefresh(screen);
}

int start_of(Racket racket) {
  return racket.center - racket.width / 2;
}
int end_of(Racket racket) {
  return start_of(racket)+ racket.width - 1;
}

void draw_racket(WINDOW* screen, Racket racket, int bound) {
  int i;
  for (i = 0; i < racket.width; i++){
    mvwprintw(screen, racket.center - racket.width / 2 + i, bound, "*");
  }
}
void move_racket(Racket* racket, int dist) {
  // racket endpoints
  int start = start_of(*racket);
  int enddd = end_of(*racket);

  // move upward
  if (dist < 0) {
    // excceed field margin
    if (start + dist < 1) {
      racket->center = 1 + racket->width / 2;
    }
  }
  // move downward
  else {
    // excceed field margin
    if (enddd + dist > racket->bound - 2) {
      racket->center = racket->bound - racket->width / 2 - 1;
    }
  }
  racket->center += dist;
}


int in_racket_y(Racket racket, Vector2 pos) {
  if (floor(start_of(racket))+0.7 <= pos.y &&
      floor(end_of(racket))+0.7   >= pos.y)
    return 1;

  return 0;
}

void draw_ball(WINDOW* screen, Ball b) {
  // float v_x = b.velocity.x, v_y = b.velocity.y;
  // // calculate last frame state
  // float last_x = b.now.x - v_x, last_y = b.now.y - v_x;
  
  // // 四捨五入最漂亮的比例
  // float unit_x = v_x, unit_y = v_y; int_ratio(&unit_x, &unit_y); 

  // if ()

  // if (v_x * v_y < 0) {
  //   mvwprintw(screen, (int)ceil(b.now.y), (int)floor(b.now.x), "o");
  // }
  // else
  // 算了就四捨五入吧...orz...
  mvwprintw(screen, (int)floor(b.now.y + 0.5), (int)floor(b.now.x + 0.5), "o");
}


void move_ball(Ball* b, Racket racket_1, Racket racket_2) {
  b->next.x = b->now.x + b->direction.x * b->speed;
  b->next.y = b->now.y + b->direction.y * b->speed;

  // player 1 lose this point yeah
  if (b->next.x < -5) {
    LOSE_FLAG_1 = 1;
  }
  // player 2 lose
  if (b->next.x >= b->bound_x + 5) {
    LOSE_FLAG_2 = 1;
  }

  // 碰撞偵測
  // 神奇數字 0.5 用來緩衝，別太嚴格哈
  if (in_racket_y(racket_1, b->next) && (b->next.x <= 1.5) && (b->next.x >= -0.5) ) {
    b->direction.x *= - 1;
  }

  if (in_racket_y(racket_2, b->next) && (b->next.x <= racket_2.side + 0.5) && (b->next.x >= racket_2.side - 1- 0.5) ) {
    b->direction.x *= - 1; 
  }

  b->now.x += b->direction.x * b->speed;


  if (b->next.y >= b->bound_y || b->next.y < 0) {
    b->direction.y *= -1;
    if (b->next.y >= b->bound_y) b->now.y = b->bound_y-1;
    if (b->next.y < 1) b->now.y = 1;    
  }
  b->now.y += b->direction.y * b->speed;
}



void draw_field(WINDOW *screen) {
  int x, y, i;
  getmaxyx(screen, y, x);
  // 4 corners
  mvwprintw(screen, 0, 0, "+");
  mvwprintw(screen, y - 1, 0, "+");
  mvwprintw(screen, 0, x - 1, "+");
  mvwprintw(screen, y - 1, x - 1, "+");
  
  // sides
  // for (i = 1; i < (y - 1); i++) {
  //   mvwprintw(screen, i, 0, "|");
  //   mvwprintw(screen, i, x - 1, "|");
  // }

  // top and bottom
  for (i = 1; i < (x - 1); i++) {
    mvwprintw(screen, 0, i, "-");
    mvwprintw(screen, y - 1, i, "-");
  }

  // draw nets
  int middle = x / 2;
  for (i = 1; i < y - 1; i+=1) {
    if (x % 2 == 0)
      mvwprintw(screen, i, middle-1, "ii");
    else
      mvwprintw(screen, i, middle, "i");
  }
}



int _kbhit(void){
  struct timeval tv; fd_set read_fd;

  /* Do not wait at all, not even a microsecond */
  tv.tv_sec=0;
  tv.tv_usec=0;

  /* Must be done first to initialize read_fd */
  FD_ZERO(&read_fd);

  /* Makes select() ask if input is ready: 0 is the file descriptor for stdin */
  FD_SET(0, &read_fd);

  /* The first parameter is the number of the largest file descriptor to check + 1. */
  if (select(1, &read_fd, NULL /*No writes*/, NULL /*No exceptions*/, &tv) == -1)
  return 0; /* An error occured */

  /* read_fd now holds a bit map of files that are readable.
  * We test the entry for the standard input (file 0). */
  if (FD_ISSET(0, &read_fd)) /* Character pending on stdin */
  return 1;

  /* no characters were pending */
  return 0;
} 



// void* keyboard_listen() {
//   while(1) {
//     key_read = getch();
//   }
//   return NULL;
// }

// void int_ratio(float* x, float* y) {
//   if (*x > *y){
//     *x = floor(*x / *y + 0.5);
//     *y = 1;
//   }
//   else {
//     *y = floor(*y / *x + 0.5);
//     *x = 1;
//   }
// }