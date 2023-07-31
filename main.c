// Copyright 2013 Sean Barrett
//
// 2013-02-22  10:00 pm -  4:30 am  ( ~3 hours) concept, ideas, most of the "art"
//                                              + twitter, Kevin Smith: Too Fat For 40
// 2013-02-23  11:00 am -  2:30 am  (~11 hours) game engine, mechanics, egg art, all but 3 rooms of 1st level, multi-receptacle rooms of 2nd
//                                              + lunch, supermarket, twitter, Making of South Park: Six Days to Air
// 2013-02-24  11:00 am - 11:30 pm  ( ~9 hours) finish levels, undo, alpha test, main menu, checkpoint, save/restore, title
//                                              + twitter, dinner
// 2013-02-25   2:00 am -  2:30 am              additional coding tweaks from beta tests
//             10:45 am - 11:15 am              additional coding tweaks from beta tests
//
// 2013-02-26   7:20 pm -  7:30 pm              fixed bug in checkpoint save-to-disk

#define TIME_SCALE 3//2.5


#define APPNAME "PROMESST"

// default screen size, and required aspect ratio
#define SCREEN_X  1024
#define SCREEN_Y  768

#pragma warning(disable:4244; disable:4305; disable:4018)

#include <assert.h>
#include <ctype.h>
#define STB_DEFINE
#define STB_WINMAIN
#ifdef TARGET_OS_WIN32
#include "stb_wingraph.h"
#else
#include "stb_sdl2graph.h"
#include <sys/stat.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096 // This'll work on OSX as well, in theory.
#endif
#include "stb_image.c"  // before stb_gl so we get stbgl_LoadImage support
#include "stb_gl.h"

static int screen_x, screen_y;
static int tex;

// red green blue orange yellow violet cyan pink

// r g b o y v c p


typedef unsigned char uint8;
typedef   signed char  int8;
typedef unsigned short uint16;

int game_started=0;

enum
{
   M_logo,
   M_menu,
   M_credits,
   M_game,
} main_mode = M_logo;

int menu_selection = 2;


typedef struct
{
   uint8 r,g,b,a;
} color;

color powers[8];
uint8 *colordata;

static int sss_tex, title_tex;

void init_graphics(void)
{
   int w,h,i;
   colordata = stbi_load("data/sprites.png", &w,&h,0,0);
   tex = stbgl_TexImage2D(0, w,h, colordata, "n");
   for (i=0; i < 8; ++i)
      memcpy(&powers[i].r, colordata+96*128*4 + (112+i)*4, 4);
   title_tex = stbgl_LoadTexture("data/title.png", "n");
   sss_tex = stbgl_LoadTexture("data/sss_logo.png", "n");
}

void get_map_color(color *c, int x, int y, int z)
{
   int s = 7*16+8 + (x&7);
   int t = 6*16 + (y&7) + (z ? 8 : 0);
   c->a = 255;
   c->r = colordata[t*128*4 + s*4 + 0];
   c->g = colordata[t*128*4 + s*4 + 1];
   c->b = colordata[t*128*4 + s*4 + 2];
}

enum
{
   DIR_e,
   DIR_n,
   DIR_w,
   DIR_s
};

int xdir[4] = { 1,0,-1,0 };
int ydir[4] = { 0,-1,0,1 };

enum
{
   TILE_wall,
   TILE_door,
   TILE_open_door,
   TILE_floor,
   TILE_receptacle,
   TILE_stone_fragments,
   TILE_arrow_e,
   TILE_arrow_n,
   TILE_arrow_w,
   TILE_arrow_s,
   TILE_stairs,

   TILE_egg,
   TILE_egg_2,
   TILE_egg_3,
   TILE_egg_4,
   TILE_egg_5,
   TILE_egg_6,

   TILE_egg_7,
   TILE_egg_8,
};

typedef struct
{
   int s,t;
} sprite_id;

sprite_id tile_sprite[] =
{
   { 0,0 }, // wall
   { 6,0 }, // door
   { 5,0 }, // open door
   { 5,3 }, // floor
   { 1,4 }, // receptacle
   { 6,2 }, // stone fragments
   { 0,3 }, // arrow
   { 1,3 },
   { 2,3 },
   { 3,3 },
   { 4,3 }, // stairs
   { 5,3 }, { 6,3 }, { 7,3 },
   { 5,4 }, { 6,4 }, { 7,4 },
   { 6,5 }, { 6,6 },
};

enum
{
   T_empty,
   T_stone,
   T_gem,

   T_projector,
   T_wand,
   T_treasure,

   T_rover,
   T_rover_stopped,
};

sprite_id obj_sprite[] =
{
   { 7,2 },
   { 2,1 },
   { 0,4 },

   { 0,2 },
   { 2,4 },
   { 3,4 },

   { 3,1 },
   { 3,1 },
};

int obj_sprite_hasdir[] =
{
   0,0,0,1,
   0,0,2,0,
};

typedef struct
{
   uint8 type:3;
   uint8 dir:2;
   uint8 color:3;
} obj;

#define NUM_X   4
#define NUM_Y   4
#define SIZE_X  6
#define SIZE_Y  6

uint8 tilemap[2][NUM_Y*SIZE_Y][NUM_Y*SIZE_Y];
obj objmap[2][NUM_Y*SIZE_Y][NUM_X*SIZE_X];
uint8 edge[2][NUM_Y*SIZE_Y][NUM_X*SIZE_X];

typedef struct
{
   uint8 player_x, player_y, player_z;
   uint8 num_gems;
   uint8 num_treasures;
   uint8 has_wand;
   uint8 ability_flag;
   uint16 num_zaps;
   uint16 egg_timer;
   uint16 seen[2];
} player_state;

//                                  / down staircase
// r R red: unlock doors            # wall
// g G green: walk through walls    . floor
// y Y yellow: destroy blocks       * stone
// t T teal: unpushable             = gem slot
// o O orange: step 2x              1,2,3,4,5,6,7,8,9  gem in slot
//                                  @,% bare gem
char *rawmap[NUM_Y*SIZE_Y][2] =
{ //     V v              v   v
   "||%||| y||||| |||||| |||||%    ",      "|||||| |||||| |||||| |..||| ", 
   "||v||| |=./.1 ||P=== |||||.    ",      "==P==| |../|| |P|||| |.|||| ", 
   ".|.|.. ...... .*|... .||.D.    ",      "==.==| |...|| |..... ..|..| ", 
   "||&|.| |||.|| .||..| .||||| <<<",      "...... ...... D...|| |.|..| ", 
   "||^..| >&..<| .|||.| ....||    ",      "*...*| |...|| |=..|| |.||9| ", 
   "|=|.|| |||.|| .D...| ||..||    ",      "||D||| ....|| |||||| |...D| ", 
                                                                        
   "|.|*|| |r|... |*||.| ||...|    ",      ".....| .||||| ||v|.| T|D||| ", 
   "|.|.|| ||..|D ||...| .|=..|    ",      ".|G|=| .|>&v. ||&.%| .%.|w| ", 
   "|D.... ....|. ||*|.| .|..O.    ",      ".....| .|.>.v ||.|.| |.|||| ", 
   "|.|.|| ||..|. |||..| .|...|    ",      "*|.||| .|^&&< ||^.|| .|g.|| ", 
   "|.|.|| 5|.=|. |||.|3 .||..|    ",      "|||||| .|.^<. |||||| |||.|| ", 
   "|.|.|| .*.||. |||..| |..*.|    ",      "|||||| .||||| |||*.. ....=| ", 
                                                                        
   "|@|.|| ||.||. ..||.| |@**||    ",      "|||||| ...=== ||..|| ||G|||", 
   "*||.|| ||.||| |.||.| ||4**|    ",      "..|..| =|.?|= ||.|.| %|..=|", 
   "|...|| ||...| |.=|.. .|*|*|    ",      ".|*|.| =|...= ...||| .||..|", 
   "|.|||| ||.|.| |g.|.* .....*    ",      ".|||.| =|(z)= ||..*| ..|..|", 
   "|.||=| 7*.|p| ||||.* ||*=.|    ",      ".....| =||u|= ||...| |||..|", 
   "..|R.| ||||*| |||*.| |||*Y*    ",      "||.||| ====== ||..*. .....|", 
                                                                        
   "..=*.. ...... ...... .|||||    ",      "||.||| |||||| |||||| ||||.| ", 
   "..|||| |.|||| ||D=*| .|...|    ",      "...... ..*.|. |..... ||*|.| ", 
   "t>.&<| ..=|%. .|.|.. .*./.|    ",      ".....| ||.||| |.|||. |||/.| ", 
   "|||@|| .|||.| .|..|| ||.*||    ",      "..-..| ||*... .*|%|. .....| ", 
   ".||||| .>&.<< ..|.|| ||..H|    ",      ".....| |||||| |||||| ||...| ", 
   "||||*| |||T|| r.D.Y| ||||||    ",      ".....| ||.||| |||||| %|.||| ", 
};
                                                                        

//  8:   % x 8
//  3:   @ x 3
//  6:   1,3,4,5,7,9

// see revision history for puzzle notes

enum
{
   COLOR_red,
   COLOR_green,
   COLOR_blue,
   COLOR_orange,
   COLOR_yellow,
   COLOR_violet,
   COLOR_teal,
   COLOR_pink,
};

char *lights = "rRPwgGHyYtToO(u)?";

// this actually encodes color + direction, not sprite_id
sprite_id lightdata[] =
{
   { COLOR_red   , DIR_e }, { COLOR_red   , DIR_n }, { COLOR_red, DIR_s }, { COLOR_red, DIR_w },
   { COLOR_green , DIR_e }, { COLOR_green , DIR_s }, { COLOR_green, DIR_n },
   { COLOR_yellow, DIR_s }, { COLOR_yellow, DIR_n },
   { COLOR_teal  , DIR_e }, { COLOR_teal  , DIR_n },
   { COLOR_orange, DIR_e }, { COLOR_orange, DIR_w },
   { COLOR_violet, DIR_e }, { COLOR_violet, DIR_n }, { COLOR_violet, DIR_w }, { COLOR_violet, DIR_s },
};




player_state state;
int px,py,pz,pdir;
int egg_x, egg_y;

#define IS_ARROW(x)  ((x) >= TILE_arrow_e && (x) < TILE_arrow_e+4)

void init_game(void)
{
   int x,y,z;
   int foo=0;

   for (z=0; z < 2; ++z) {
      for (y=0; y < NUM_Y*SIZE_Y; ++y) {
         char *data = rawmap[y][z];
         for (x=0; x < NUM_X*SIZE_X; ++x) {
            int d = data[x + x/SIZE_X];
            tilemap[z][y][x] = TILE_floor;
            objmap[z][y][x].type = T_empty;
            switch (d) {
               case 'p':
                  px = x; py = y; pz = z;
                  pdir = DIR_s;
                  break;
               case '>': tilemap[z][y][x] = TILE_arrow_e; break;
               case '^': tilemap[z][y][x] = TILE_arrow_n; break;
               case '<': tilemap[z][y][x] = TILE_arrow_w; break;
               case 'v': tilemap[z][y][x] = TILE_arrow_s; break;
               default:  tilemap[z][y][x] = TILE_wall;    break;
               case '=': tilemap[z][y][x] = TILE_receptacle; break;
               case 'D': tilemap[z][y][x] = TILE_door;    break;
               case '/': tilemap[z][y][x] = TILE_stairs; break;

               case 'z': egg_x = x;
                         egg_y = y;
                         break;

               case '*': objmap[z][y][x].type = T_stone; break;
               case '-': objmap[z][y][x].type = T_wand;  break;
               case '$': objmap[z][y][x].type = T_treasure; break;

               case '1': case '2': case '3': case '4':
               case '5': case '6': case '7': case '8': case '9':
               case '@': case '%':
                         objmap [z][y][x].type = T_gem;
                         objmap [z][y][x].dir = 0;
                         if (d != '@' && d != '%')
                           tilemap[z][y][x] = TILE_receptacle;
                         break;

               case '&': objmap[z][y][x].type = T_rover;
                         objmap[z][y][x].dir = DIR_e;
                         break;

               case 'r': case 'g': case 'y': case 't': case 'o':
               case 'R': case 'G': case 'Y': case 'T': case 'O': 
               case 'P': case 'H': case 'w':
               case '(': case ')': case '?': case 'u':
               {
                  char *s = strchr(lights, d);
                  assert(s != NULL);
                  d = s - lights;
                  objmap[z][y][x].type  = T_projector;
                  objmap[z][y][x].color = lightdata[d].s;
                  objmap[z][y][x].dir   = lightdata[d].t;
                  break;
               }

               case '.':
               case ' ':
                  break;

            }
         }
      }
   }

   for (x=0; x < 3; ++x)
      for (y=0; y < 2; ++y)
         tilemap[1][egg_y-1+y][egg_x-1+x] = TILE_egg + y*3+x;

   // initialize rover directions by finding adjacent arrow and setting initial direction in the arrow dir
   for (z=0; z < 2; ++z) {
      for (y=0; y < NUM_Y*SIZE_Y; ++y) {
         for (x=0; x < NUM_X*SIZE_X; ++x) {
            if (objmap[z][y][x].type == T_rover) {
               int d;
               // find an adjacent arrow
               for (d=0; d < 4; ++d)
                  if (tilemap[z][y-ydir[d]][x-xdir[d]] == TILE_arrow_e + d)
                     objmap[z][y][x].dir = d;
            }
         }
      }
   }
}

typedef struct st_roomsave_big
{
   struct st_roomsave_big *next;
//   uint8 big_flag;
   player_state state;
   uint8 room_x, room_y, room_z;
   uint8 doors[SIZE_Y];
   obj h_objmap[SIZE_Y][NUM_X*SIZE_X];
   obj v_objmap[NUM_Y*SIZE_Y][SIZE_X];
} roomsave_big; // ~316 bytes incl malloc overhead -- full world would be about 2x

// how undo works:
//
//   world state N
//      state of world in N-1 that might be changed by N
//   world state N-1
//      state of world in N-2 that might be changed by N-1
//   world state N-2
//      state of world in N-3 that might be changed by N-2
//   world state N-3
//
// One way to store the above would be to store the *entire*
// state of world N-1, N-2, etc.
//
//  save:
//         1. create a roomsave_big for the room
//            being entered (and save previous player state)
//         2. push on undo stack
//
//  undo:
//         1. pop undo stack


#if 0
// we could try to use this if we know that the change doesn't
// include any out-of-room effects, but the restore logic was
// a lot more complicated so I punted it
typedef struct st_roomsave
{
   struct st_roomsave *next;
   uint8 big_flag;
   player_state state;
   uint8 room_x, room_y, room_z;
   obj objmap[SIZE_Y][SIZE_X];   
} roomsave; // 52 bytes incl malloc overhead
//   when you enter a room:
//      turn previous save into an undo entry:
//         1. if "out-of-room" effects flag set,
//            add roomsave_big to undo chain
//         2. otherwise, add roomsave to undo chain
//      save current state:
//         3. create a roomsave_big for the room
//            being entered (and save previous player state)
//         4. clear the "out-of-room" effects flag.
//         5. during play, set the "out-of-room"
//            effects flag if the player pushes
//            anything out of room
//
//  on undo, say we're in world state N:
//
//     1. restore stored roomsave_big
//
//  this returns us to state N-1. but we can now make further
//  changes, invalidating the N-2 delta IF that delta isn't big.
//  
//     2. pop undo stack
//        2a. if big on stack, copy into roomsave_big
//        2b. if small on stack, combined current world with undo'd room to create roomsave_big
#endif


int room_x, room_y, room_z;

void compute_room(void)
{
   room_x = px / SIZE_X;
   room_y = py / SIZE_Y;
   room_z = pz;
}

int has_power[NUM_Y][NUM_X];

void compute_powered(void)
{
   int x,y;
   memset(has_power, 0, sizeof(has_power));

   // there's power if (a) there's at least one
   // receptacle, and (b) there's no unpowered receptacle
   for (y=0; y < NUM_Y*SIZE_Y; ++y)
      for (x=0; x < NUM_X*SIZE_X; ++x)
         if (tilemap[room_z][y][x] == TILE_receptacle)
            has_power[y/SIZE_Y][x/SIZE_X] = 1;

   for (y=0; y < NUM_Y*SIZE_Y; ++y)
      for (x=0; x < NUM_X*SIZE_X; ++x)
         if (tilemap[room_z][y][x] == TILE_receptacle && objmap[room_z][y][x].type != T_gem)
            has_power[y/SIZE_Y][x/SIZE_X] = 0;
}

int8 light_for_dir[NUM_Y*SIZE_Y][NUM_X*SIZE_X][4];
uint8 any_light_for_dir[NUM_Y*SIZE_Y][NUM_X*SIZE_X];

void propagate_light(void)
{
   int x,y;
   memset(light_for_dir, -1, sizeof(light_for_dir));
   memset(any_light_for_dir, 0, sizeof(any_light_for_dir));
   for (y=0; y < NUM_Y*SIZE_Y; ++y)
      for (x=0; x < NUM_X*SIZE_X; ++x)
         if (objmap[room_z][y][x].type == T_projector)
            if (has_power[y/SIZE_Y][x/SIZE_X]) {
               obj o = objmap[room_z][y][x];
               int ax = x, dx = xdir[o.dir];
               int ay = y, dy = ydir[o.dir];
               uint8 c = o.color;
               for(;;) {
                  ax += dx; if (ax < 0) ax += NUM_X*SIZE_X; else if (ax >= NUM_X*SIZE_X) ax=0;
                  ay += dy; if (ay < 0) ay += NUM_Y*SIZE_Y; else if (ay >= NUM_Y*SIZE_Y) ay=0;
                  if (tilemap[room_z][ay][ax] == TILE_door)
                     break;
                  if (objmap[room_z][ay][ax].type != T_empty)
                     break;
                  light_for_dir[ay][ax][o.dir] = o.color;
                  any_light_for_dir[ay][ax] = 1;
                  if (room_z == 1 && ax == egg_x && ay == egg_y)
                     break;
               }
            }

}


typedef struct
{
   uint8 tilemap[2][NUM_Y*SIZE_Y][NUM_Y*SIZE_Y];
   obj objmap[2][NUM_Y*SIZE_Y][NUM_X*SIZE_X];
   player_state state;   
} gamestate;


static void save_map(gamestate *g)
{
   memcpy(g->tilemap, tilemap, sizeof(tilemap));
   memcpy(g->objmap, objmap, sizeof(objmap));
   state.player_x = px;
   state.player_y = py;
   state.player_z = pz;
   g->state = state;
}

static void restore_map(gamestate *g)
{
   memcpy(tilemap, g->tilemap, sizeof(tilemap));
   memcpy(objmap, g->objmap, sizeof(objmap));
   state = g->state;
   px = state.player_x;
   py = state.player_y;
   pz = state.player_z;

   compute_room();
   compute_powered();
   propagate_light();
}

roomsave_big *undo_chain;

static void save_state(int rx, int ry, int rz)
{
   int x,y;
   roomsave_big *save_big;
   save_big = malloc(sizeof(*save_big));
   for (y=0; y < SIZE_Y; ++y)
      for (x=0; x < SIZE_X*NUM_X; ++x)
         save_big->h_objmap[y][x] = objmap[rz][ry * SIZE_Y + y][x];
   for (y=0; y < SIZE_Y*NUM_Y; ++y)
      for (x=0; x < SIZE_X; ++x)
         save_big->v_objmap[y][x] = objmap[rz][y][rx * SIZE_X + x];

   for (y=0; y < SIZE_Y; ++y) {
      uint8 doors=0;
      for (x=0; x < SIZE_X; ++x)
            if (tilemap[rz][ry*SIZE_Y+y][rx*SIZE_X+x] == TILE_door)
               doors |= 1 << x;
      save_big->doors[y] = doors;
   }
            
   save_big->next = undo_chain;
   undo_chain = save_big;
   save_big->room_x = rx;
   save_big->room_y = ry;
   save_big->room_z = rz;
   save_big->state = state;
}

static void restore_undo(void)
{
   int x,y,rx,ry,rz;
   roomsave_big *save = undo_chain;
   if (save == NULL) return;
   undo_chain = save->next;

   rx = save->room_x;
   ry = save->room_y;
   rz = save->room_z;
   for (y=0; y < SIZE_Y; ++y)
      for (x=0; x < SIZE_X*NUM_X; ++x)
         objmap[rz][ry * SIZE_Y + y][x] = save->h_objmap[y][x];
   for (y=0; y < SIZE_Y*NUM_Y; ++y)
      for (x=0; x < SIZE_X; ++x)
         objmap[rz][y][rx * SIZE_X + x] = save->v_objmap[y][x];
   // restore closed doors in case any got opened
   for (y=0; y < SIZE_Y; ++y)
      for (x=0; x < SIZE_X*NUM_X; ++x)
         if (save->doors[y] & (1 << x))
            tilemap[rz][ry*SIZE_Y+y][rx*SIZE_X+x] = TILE_door;
   state = save->state;
   free(save);

   px = state.player_x;
   py = state.player_y;
   pz = state.player_z;
   pdir = DIR_s;

   compute_room();
   compute_powered();
   propagate_light();
}

static void flush_undo(void)
{
   while (undo_chain) {
      roomsave_big *save = undo_chain;
      undo_chain = save->next;
      free(save);
   }
}

static void set_player_pos(int x, int y, int z)
{
   int new_room_x = x / SIZE_X;
   int new_room_y = y / SIZE_Y;

   if (new_room_x != room_x || new_room_y != room_y || z != pz) {
      // entering a new room!
      state.player_x = px;
      state.player_y = py;
      state.player_z = pz;
      save_state(new_room_x, new_room_y, pz);
   }
   px = x;
   py = y;
   pz = z;

   compute_room();
   compute_powered();
   propagate_light();

   state.seen[pz] |= 1 << (room_y * NUM_X + room_x);
}

#define POWER_doors    COLOR_red
#define POWER_walls    COLOR_green
#define POWER_destroy  COLOR_yellow
#define POWER_immobile COLOR_teal
#define POWER_double   COLOR_orange

static void get_abilities(uint8 enabled[8])
{
   int d;
   memset(enabled, 0, 8);
   for (d=0; d < 4; ++d)
      if (light_for_dir[py][px][d] >= 0)
         enabled[light_for_dir[py][px][d]] += 1;

   //enabled[POWER_immobile] = 1;
}

#define WRAP_X(x)  (((x) + SIZE_X*NUM_X) % (SIZE_X*NUM_X))
#define WRAP_Y(y)  (((y) + SIZE_Y*NUM_Y) % (SIZE_Y*NUM_Y))

static int push_rovers(int x, int y, int dir, int n)
{
   int i, ax = x, ay = y, j;
   // check if there is room to push the rover to
   for (i=0; i < n; ++i) {
      ax = WRAP_Y(ax + xdir[dir]);
      ay = WRAP_Y(ay + ydir[dir]);
      if (tilemap[pz][ay][ax] == TILE_wall)
         return 0;
      if (tilemap[pz][ay][ax] == TILE_door)
         return 0;
      if (objmap[pz][ay][ax].type == T_empty)
         break;
      if (objmap[pz][ay][ax].type != T_rover && objmap[pz][ay][ax].type != T_rover_stopped)
         return 0;
   }
   if (i == n)  // needed to push too many rovers
      return 0;

   // now push them
   assert(objmap[pz][ay][ax].type == T_empty);
   assert(objmap[pz][y][x].type == T_rover || objmap[pz][y][x].type == T_rover_stopped);
   for (j=i; j >= 0; --j) {
      int bx,by;
      bx = WRAP_X(x + xdir[dir]*j);
      by = WRAP_Y(y + ydir[dir]*j);
      ax = WRAP_X(bx + xdir[dir]);
      ay = WRAP_Y(by + ydir[dir]);
      objmap[pz][ay][ax] = objmap[pz][by][bx];
      objmap[pz][by][bx].type = T_empty;
   }
   return 1;
}

#define PLAYER_MOVE_TIME    280
#define PLAYER_TRAVEL_TIME  150
int player_timer;

#define ROVER_MOVE_TIME   230

int noclip;

gamestate checkpoint;
int checkpoint_timer = 0;

static void move(int x, int y)
{
   uint8 used_flag = state.ability_flag;
   int got_wand=0;
   uint8 abilities[8];
   int gx,gy;

   pdir = x ? x<0 ? DIR_w : DIR_e : y<0 ? DIR_n : DIR_s;

   get_abilities(abilities);
   if (abilities[POWER_double]) {
      x *= (1 << abilities[POWER_double]);
      y *= (1 << abilities[POWER_double]);
   }

   gx = WRAP_X(px+x);
   gy = WRAP_Y(py+y);

   if (noclip) {
      set_player_pos(gx,gy,pz);
      return;
   }

   // make them get stuck if they move into a wall and have no projector
   if (tilemap[pz][py][px] == TILE_wall) {
      if (!abilities[POWER_walls])
         return;
      used_flag |= (1 << POWER_walls);
   }
      
   if (objmap[pz][gy][gx].type == T_projector)
      return;

   if (tilemap[pz][gy][gx] == TILE_wall) {
      if (!abilities[POWER_walls])
         return;
      used_flag |= (1 << POWER_walls);
   }

   if (tilemap[pz][gy][gx] == TILE_door) {
      if (!abilities[POWER_doors])
         return;
      state.ability_flag |= (1 << POWER_doors);
      tilemap[pz][gy][gx] = TILE_open_door;
      return; // don't move, just open
   }

   if (objmap[pz][gy][gx].type >= T_rover) {
      if (abilities[POWER_immobile] < 2) {
         if (!abilities[POWER_destroy])
            return;
         state.ability_flag |= (1 << POWER_destroy);
         objmap[pz][gy][gx].type = T_empty;
         return;
      }
      // in double teal, can push a rover
      if (!push_rovers(gx, gy, pdir, abilities[POWER_immobile]-1))
         return;
      used_flag |= (1 << POWER_immobile);
   }

   if (objmap[pz][gy][gx].type == T_stone) {
      if (!abilities[POWER_destroy])
         return;
      state.ability_flag |= (1 << POWER_destroy);
      objmap[pz][gy][gx].type = T_empty;
      return; // don't move, just destroy
   }

   if (objmap[pz][gy][gx].type == T_wand) {
      state.has_wand = 1;
      objmap[pz][gy][gx].type = 0;
      got_wand = 1;
   }

   if (abilities[POWER_double])
      used_flag |= (1 << POWER_double);

   state.ability_flag = used_flag;

   if (tilemap[pz][gy][gx] == TILE_stairs)
      set_player_pos(gx,gy,(pz+1)%2);
   else
      set_player_pos(gx,gy,pz);

   player_timer = PLAYER_MOVE_TIME;
   if (got_wand) {
      save_map(&checkpoint);
      checkpoint_timer = 1600;
   }
}

static void drop(void)
{
   if (objmap[pz][py][px].type == T_gem) {
      objmap[pz][py][px].type = T_empty;
      ++state.num_gems;
   } else
#if 0
   // drop anywhere: this lets you hack things by dropping beams
   if (state.num_gems && objmap[pz][py][px].type == T_empty && tilemap[pz][py][px] != TILE_wall)
#else
   if (state.num_gems && objmap[pz][py][px].type == T_empty && tilemap[pz][py][px] == TILE_receptacle)
#endif
   {
      --state.num_gems;
      objmap[pz][py][px].type = T_gem;
      objmap[pz][py][px].dir = 1;
   }
   compute_powered();
   propagate_light();
}

int shot_x, shot_y, shot_timer;

static void shoot(void)
{
   int x = WRAP_X(px+xdir[pdir]);
   int y = WRAP_Y(py+ydir[pdir]);

   if (!state.has_wand)
      return;
   if (tilemap[pz][y][x] == TILE_wall || tilemap[pz][y][x] == TILE_door)
      return;

   shot_x = x;
   shot_y = y;
   shot_timer = 50;
   if (objmap[pz][y][x].type != T_empty) {
      int start_x = x, start_y = y;
      while (x != px || y != py) {
         x = WRAP_X(x+xdir[pdir]);
         y = WRAP_Y(y+ydir[pdir]);
         if (tilemap[pz][y][x] == TILE_wall || tilemap[pz][y][x] == TILE_door)
            continue;
         if (objmap[pz][y][x].type != T_empty)
            continue;
         break;
      }

      if (x != px || y != py) {
         // move last thing to x,y, then iterate back
         ++state.num_zaps;
         while (x != start_x || y != start_y) {
            int ax = x;
            int ay = y;
            for(;;) {
               ax = WRAP_X(ax - xdir[pdir]);
               ay = WRAP_Y(ay - ydir[pdir]);
               if (tilemap[pz][ay][ax] == TILE_wall || tilemap[pz][ay][ax] == TILE_door)
                  continue;
               break;
            }
            objmap[pz][y][x] = objmap[pz][ay][ax];
            objmap[pz][ay][ax].type = 0;
            x = ax;
            y = ay;
         }
      }
   }
}


int queued_key;

static void do_key(int ch)
{
   switch (ch) {
      case 's' : move(0, 1); break;
      case 'a' : move(-1,0); break;
      case 'd' : move( 1,0); break;
      case 'w' : move(0,-1); break;
      case 'x' : drop(); break;
      case 'z' : shoot(); break;
#ifdef _DEBUG
      case 'g' : state.num_gems = 99; state.has_wand = 1; break;
      case 'n' : noclip = !noclip; break;
#endif
      case  8  : restore_undo(); break;
      case 27  : {
         main_mode = M_menu;
         menu_selection = 0; // continue
         break;
      }
   }
}

static int player_allowed(int x, int y)
{
   uint8 abilities[8];
   get_abilities(abilities);
   if (tilemap[pz][y][x] == TILE_wall) {
      if (abilities[POWER_walls])
         return 1;
      return 0;
   }
   if (objmap[pz][y][x].type != T_empty)
      return 0;
   return 1;
}

static int rove_allowed(int x, int y, int d, int rx, int ry)
{
   if (x < rx || x >= rx+SIZE_X)
      return 0;
   if (y < ry || y >= ry+SIZE_Y)
      return 0;
   if (tilemap[pz][y][x] == TILE_wall)
      return 0;
   if (tilemap[pz][y][x] == TILE_door)
      return 0;
   if (x == px && y == py) {
      uint8 abilities[8];
      get_abilities(abilities);
      // check if there's room to push the player
      if (!player_allowed(WRAP_X(x+xdir[d]), WRAP_Y(y+ydir[d])))
         return 0;
      if (abilities[POWER_immobile]) {
         state.ability_flag |= (1 << POWER_immobile);
         return 0;
      }
   }
   // @TODO: push gems? then we need to do chained pushing
   // below includes a rover, even if it's GOING to move
   if (objmap[pz][y][x].type != T_empty)
      return 0;
   return 1;
}


static int preferred_rove_direction(uint8 occupied[SIZE_Y][SIZE_X], int x, int y, int rx, int ry)
{
   int cx = rx + x;
   int cy = ry + y;

   // compute the preferred direction
   int d = objmap[pz][cy][cx].dir;
   if (IS_ARROW(tilemap[pz][cy][cx])) {
      // if there's an arrow, that's the ONLY direction allowed
      d = tilemap[pz][cy][cx] - TILE_arrow_e;
   } else {
      if (!rove_allowed(cx+xdir[d],cy+ydir[d], d, rx,ry) || occupied[y][x] > 1)
         d = (d+2)&3;
   }
   if (!rove_allowed(cx+xdir[d],cy+ydir[d], d, rx,ry) || occupied[y][x] > 1)
      return -1;

   return d;
}

int rover_timer = 0;


static void move_rovers(int rx, int ry)
{
   int x,y;
   sprite_id rovers[SIZE_X*SIZE_Y];
   int num_rovers=0, i;

   uint8 occupied[SIZE_Y][SIZE_X] = {0};
   // we want to move rovers simultaneously, so we need to get each one to tell us
   // where it WANTS to go

   ry = ry * SIZE_Y;
   rx = rx * SIZE_X;

   for (y=0; y < SIZE_Y; ++y) {
      for (x=0; x < SIZE_X; ++x) {
         if (objmap[pz][ry+y][rx+x].type >= T_rover) {
            int d = preferred_rove_direction(occupied, x,y, rx,ry);
            if (d < 0)
               ++occupied[y][x];
            else
               ++occupied[y+ydir[d]][x+xdir[d]];
            rovers[num_rovers].s = x;
            rovers[num_rovers].t = y;
            ++num_rovers;
         }
      }
   }

   // then we decide where to go based on nobody else going there.
   // note that we really need to keep iterating this to a fixed point

   for(;;) {
      int any_change = 0;
      for (i=0; i < num_rovers; ++i) {
         if (rovers[i].s >= 0) {
            int d;
            x = rovers[i].s;
            y = rovers[i].t;
            d = preferred_rove_direction(occupied, x,y, rx,ry);
            if (d < 0) {
               objmap[pz][ry+y][rx+x].type = T_rover_stopped;
            } else {
               // it might currently be blocked by another rover that's moving away
               if (objmap[pz][ry+y+ydir[d]][rx+x+xdir[d]].type >= T_rover)
                  continue;
               // @TODO: push player
               if (px == rx+x+xdir[d] && py == ry+y+ydir[d]) {
                  player_timer = rover_timer;
                  px = rx+x+xdir[d]*2;
                  py = ry+y+ydir[d]*2;
                  pdir = d;
               }
               objmap[pz][ry+y][rx+x].type = 0;
               objmap[pz][ry+y+ydir[d]][rx+x+xdir[d]].type = T_rover;
               objmap[pz][ry+y+ydir[d]][rx+x+xdir[d]].dir = d;
            }
            rovers[i].s = -1;
            any_change = 1;
         }
      }
      if (!any_change)
         break;
   }
}

static void timestep(uint ms)
{
   if (rover_timer >= 0) {
      rover_timer -= ms;
      while (rover_timer < 0) {
         int x,y;
         rover_timer += ROVER_MOVE_TIME;
         for (y=0; y < NUM_Y; ++y)
            for (x=0; x < NUM_X; ++x)
               move_rovers(x,y);
      }
   }

   if (player_timer > 0) {
      player_timer -= ms;
   }
   if (shot_timer > 0) {
      shot_timer -= ms;
   }
   if (checkpoint_timer > 0) {
      checkpoint_timer -= ms;
   }
   if (player_timer <= 0 && shot_timer <= 0 && queued_key) {
      do_key(queued_key);
      queued_key = 0;
   }

   if (pz == 1 || state.egg_timer > 8000) {
      int i;
      for (i=0; i < 4; ++i)
         if (light_for_dir[egg_y][egg_x][i] != COLOR_violet)
            break;

      if (state.egg_timer > 8000) i = 4;

      if (i == 4) {
         state.egg_timer += ms;
      }
   }

   tilemap[1][egg_y][egg_x] = TILE_egg_5;
   if (state.egg_timer > 2000) {
      tilemap[1][egg_y][egg_x] = TILE_egg_7;
   }
   if (state.egg_timer > 5000) {
      tilemap[1][egg_y][egg_x] = TILE_egg_8;
   }
}

static void restart_game(void)
{
   memset(&state, 0, sizeof(state));
   init_game();
   compute_room();
   compute_powered();
   state.seen[pz] |= 1 << (room_y * NUM_X + room_x);
}

char *choices[] =
{
   "CONTINUE GAME",
   "RESTORE FROM WAND GET",
   "START NEW GAME",
   "CREDITS",
   "SAVE AND QUIT",
};
#define NUM_MENU  (sizeof(choices)/sizeof(choices[0]))

struct {
   int color;
   float scale;
   char *text;
} credits[] =
{
   COLOR_red, 3, "INSPIRED BY",
   0,0.5,0,
   COLOR_teal,4, "THE GAMES OF",
   COLOR_teal,4, "MICHAEL BROUGH",
   0,3.0,0,
   COLOR_green, 3, "PLAYTEST BY",
   0,0.5,0,
   COLOR_violet, 4, "ZACH SAMUELS",
   COLOR_violet, 4, "ADMIRAL JOTA",
   COLOR_violet, 4, "CASEY MURATORI",
   COLOR_violet, 4, "JONATHAN BLOW",
   0,3.0,0,
   COLOR_blue, 3, "GAME BY",
   0,0.5,0,
   COLOR_yellow, 4, "SEAN BARRETT",
};
#define NUM_CREDITS  (sizeof(credits)/sizeof(credits[0]))

void get_choices(int enabled[8])
{
   int i;
   for (i=0; i < 8; ++i)
      enabled[i] = 1;
   if (!game_started)
      enabled[0] = 0;
   if (!state.has_wand)
      enabled[1] = 0;
}

static void move_selection(int dir)
{
   int enabled[8];
   get_choices(enabled);
   for(;;) {
      menu_selection = (menu_selection + dir + NUM_MENU) % NUM_MENU;
      if (enabled[menu_selection])
         break;
   }
}

gamestate loaded_game;

static char *get_savegame_path(void)
{
	char *root_dir = SDL_getenv("XDG_DATA_HOME");
	static int gotpath = FALSE;
	static char save_dir[PATH_MAX] = {0};
	if (gotpath) return save_dir;
	if (!root_dir)
	{
		root_dir = SDL_getenv("HOME");
		if (!root_dir)
		{
			strcpy(save_dir, "data/save.dat");
			gotpath = TRUE;
			return save_dir;
		}
		else
		{
			strcpy(save_dir, root_dir);
			strcat(save_dir, "/.local/share/Promesst/save.dat");
		}
	}
	else
	{
		strcpy(save_dir, root_dir);
		strcat(save_dir, "/Promesst/save.dat");
	}

	// Make all of the directories leading up to this.
	int i;
	for (i = 0; i < strlen(save_dir); ++i)
	{
		if (save_dir[i] == '/')
		{
			save_dir[i] = '\0';
			mkdir(save_dir, S_IRWXU);
			save_dir[i] = '/';
		}
	}
	gotpath = TRUE;
	return save_dir;
}

static void load_game(void)
{
   roomsave_big *tail=0;
   FILE *f = fopen(get_savegame_path(), "rb");
   if (!f) return;
   if (!fread(&loaded_game, sizeof(loaded_game), 1, f))
      goto end;
   restore_map(&loaded_game);
   fread(&checkpoint, sizeof(checkpoint), 1, f);
   flush_undo();
   for(;;) {
      roomsave_big r;
      roomsave_big *p;
      if (!fread(&r, sizeof(r), 1, f))
         break;
      p = malloc(sizeof(*p));
      *p = r;
      p->next = 0;
      if (tail)
         tail->next = p;
      else
         undo_chain = p;
      tail = p;
   }
 end:
   fclose(f);
   game_started = 1;
}

static void save_game(void)
{
   roomsave_big *r;
   FILE *f;
   if (!game_started)
      return;
   f = fopen(get_savegame_path(), "wb");
   if (!f) return;
   save_map(&loaded_game);
   fwrite(&loaded_game, sizeof(loaded_game), 1, f);
   fwrite(&checkpoint, sizeof(checkpoint), 1, f);
   r = undo_chain;
   while (r) {
      fwrite(r, sizeof(*r), 1, f);
      r = r->next;
   }
   fclose(f);
}

void do_metagame_key(int key)
{
   if (main_mode != M_menu) {
      main_mode = M_menu;
   } else {
      switch (key) {
         case 'w': move_selection(-1); break;
         case 's': move_selection( 1); break;

         case '\n':
         case '\r':
         case ' ': {
            switch (menu_selection) {
               case 0:
                  main_mode = M_game;
                  game_started = 1;
                  break;
               case 1:
                  restore_map(&checkpoint);
                  game_started = 1;
                  flush_undo();
                  main_mode = M_game;
                  break;
               case 2:
                  restart_game();
                  game_started = 1;
                  main_mode = M_game;
                  break;
               case 3:
                  main_mode = M_credits;
                  break;
               case 4:
                  save_game();
                  exit(0);
            }
         }
      }
   }
}

#define MAX_LOGO 1500 // ms
int logo_time = MAX_LOGO;

void process_metagame(float dt)
{
   if (queued_key) {
      do_metagame_key(queued_key);
      queued_key = 0;
   }
   if (main_mode == M_logo && logo_time > 0) {
      logo_time -= 1000*dt;
      if (logo_time <= 0) {
         main_mode = M_menu;
      }
   }
}



int draw_initialized=0;

void draw_init(void)
{
   draw_initialized = 1;

//   glEnable(GL_CULL_FACE);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   glViewport(0,0,screen_x,screen_y);
   glClearColor(0,0,0,0);
   glClear(GL_COLOR_BUFFER_BIT);

   // translate so we draw centered
   if (screen_x*SCREEN_Y > screen_y*SCREEN_X) {
      int w;
      // pixel size is sy/SCREEN_Y, so extent of x is SCREEN_X * sy / SCREEN_y
      w = SCREEN_X * screen_y / SCREEN_Y;
      glViewport((screen_x - w)/2.0, 0, w, screen_y);
      // transform function is:
      // window_x = (sx-w)/2.0 + SCREEN_X * logical_x / w;
      // window_y = SCREEN_Y * logical_y / sy;

      //xs_p2v = (float) SCREEN_X/w;
      //ys_p2v = (float) SCREEN_Y/sy;
      //xoff_p2v = (sx-w)/2.0f;
      //yoff_p2v = 0;
   } else {
      int h = SCREEN_Y * screen_x / SCREEN_X;
      glViewport(0, (screen_y - h)/2, screen_x,h);

      //xs_p2v = (float) SCREEN_X / sx;
      //ys_p2v = (float) SCREEN_Y / h;
      //xoff_p2v = 0;
      //yoff_p2v = (sy-h)/2.0f;
   }

   // force a viewport that matches our size

}

static int blinn_8x8(int p1, int p2)
{
   int m = p1*p2 + 128;
   return (m + (m>>8)) >> 8;
}

static void draw_rect(int x, int y, int w, int h, int is0, int it0, int is1, int it1)
{
   float s0,s1,t0,t1;

   s0 = (is0+0.05) / 128.0;
   t0 = (it0+0.05) / 128.0;
   s1 = (is1-0.05) / 128.0;
   t1 = (it1-0.05) / 128.0;

   glTexCoord2f(s0,t0); glVertex2i(x   ,y   );
   glTexCoord2f(s1,t0); glVertex2i(x+w ,y   );
   glTexCoord2f(s1,t1); glVertex2i(x+w ,y+h );
   glTexCoord2f(s0,t1); glVertex2i(x   ,y+h );
}

static void draw_sprite(int x, int y, int s, int t, color *c, float a)
{
   if (c)
      glColor4ub(c->r, c->g, c->b, (int) (c->a * a));
   else
      glColor4f(1,1,1,a);

   x = x*16 - 12;
   y = y*16 - 12;

   draw_rect(x, y, 16, 16, s*16, t*16, (s+1)*16,(t+1)*16);
}

static void draw_subsprite(int x, int y, int dx, int dy, int s, int t, color *c, float a)
{
   if (c)
      glColor4ub(c->r, c->g, c->b, (int) (c->a * a));
   else
      glColor4f(1,1,1,a);

   x = x*16 - 12 + dx;
   y = y*16 - 12 + dy;

   draw_rect(x, y, 16, 16, s*16, t*16, (s+1)*16,(t+1)*16);
}

static char *font = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789>^<v/ ";
static int draw_text(int x, int y, int size, char *text)
{
   while (*text) {
      char *p = strchr(font, *text);
      if (p) {
         int ch = p - font;
         int s = 1 + (ch % 21)*6;
         int t = 113 + (ch/21)*8;
         draw_rect(x, y, size*5, size*7, s, t, s+5,t+7);
         x += size*6;
      }
      ++text;
   }
   return x;
}

unsigned int animcycle;


void draw_metagame(float flicker)
{
   int i;
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0,800,600,0,-1,1);


// draw background

   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glEnable(GL_BLEND);

   if (main_mode == M_logo)
      glBindTexture(GL_TEXTURE_2D, sss_tex);
   else 
      glBindTexture(GL_TEXTURE_2D, tex);  // sprite sheet

   glEnable(GL_TEXTURE_2D);
   glBegin(GL_QUADS);

   for (i=0; i < 24; ++i) {
      int s,t;
      int x = (rand() % 32)-32;
      int y = (rand() % 32)-32;
      int r = (rand() % 32)+32;
      int g = (rand() % 32)+32;
      int b = (rand() % 32)+32;
      int z = 6;//(rand() % 3) + 3;
      glColor4ub(r,g,b, 32);
      s = rand() % 128;
      t = rand() % 128;
      if (main_mode == M_logo) {
         float t = (logo_time-100) / (float) MAX_LOGO;
         if (t < 0) t = 0;
         t = 1-sqrt(t);
         x += 16;
         y += 16;
         x = x * (t+1)*1;
         y = y * (t+1)*1;
         glColor4ub(r,g,b, 48 * (1-t));
         draw_rect(x,y+100,832,432, 0,0,128,128);
      } else
         draw_rect(x,y,832,632, s+8*z,t+6*z,s,t); // random fragments of sprite sheet

   }
   rand(); // above computes 7*24 = 168, we go to 169 here to go more out of phase with 2^15
   glEnd();

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   if (main_mode == M_logo) {
      float t = (logo_time-250) / (float) MAX_LOGO;
      if (t < 0) t = 0;
      //t = sqrt(t);
      glColor4f(1,1,1,1-t);
      glBegin(GL_QUADS);
      draw_rect(0,0+100,832,432, 0,0,128,128);
      glEnd();

      return;
   }

// draw title


// draw menu or credits

   glBindTexture(GL_TEXTURE_2D, title_tex);
   glColor3f(1,1,1);
   glBegin(GL_QUADS);
   draw_rect(100, main_mode == M_menu ? 0 : -85, 600,300, 0,0,128,128);
   glEnd();

   glBindTexture(GL_TEXTURE_2D, tex);
   glBegin(GL_QUADS);

   if (main_mode == M_menu) {
      int sx = 400, sy = 320;
      int i;
      int enabled[8];
      get_choices(enabled);

      for (i=0; i < NUM_MENU; ++i) {
         if (enabled[i]) {
            int len = strlen(choices[i]);
            int selected = (menu_selection == i);
            int scale = selected ? 5 : 4;
            int width = len * 6 * scale;

            if (!selected) {
               glColor4f(0.55,0.35,0.55,1.0);
            } else {
               glColor4f(1,0.5,1,0.5+0.5*flicker);
            }
            draw_text(sx - width/2, sy, scale, choices[i]);
            sy += 50;
         }
      }
   } else {
      int sx = 400, sy = 200;
      int i;
      for (i=0; i < NUM_CREDITS; ++i) {
         if (credits[i].text) {
            int len = strlen(credits[i].text);
            int selected = (menu_selection == i);
            int scale = credits[i].scale;
            int width = len * 6 * scale;
            color c = powers[credits[i].color];
            if ((animcycle/500) % NUM_CREDITS == (i*7)%NUM_CREDITS) { //(NUM_CREDITS*7) == (i*71) % (NUM_CREDITS*7)) {
               glColor4ub(c.r,c.g,c.b, (int) (255*flicker));
            } else {
               glColor4ubv(&c.r);
            }
            draw_text(sx - width/2, sy, scale, credits[i].text);
         }
         sy += 8 * credits[i].scale;
      }
   }
   glEnd();
}


void draw_world(void)
{
   color c;
   int x,y,sx;
   int epx,epy;
   int power = has_power[room_y][room_x];
   int a = animcycle / 10;

   float flicker = fabs(sin(animcycle/100.0))*0.5 + 0.5;

   if (a % 130 == 0 || a % 217 == 0 || a % 252 == 0)
      flicker += 0.5;
   if (a % 103 == 0 || a % 117 == 0 || a % 501 == 0)
      flicker *= 0.7;
   if (a % 88 == 0 || a % 281 == 0)
      flicker *= 0.5;      
   if (flicker > 1) flicker = 1;
   if (flicker < 0.2) flicker = 0.2;

   if (main_mode != M_game) {
      draw_metagame(flicker);
      return;
   }

   tile_sprite[0].s = (room_x ^ room_y) & 1 ? 7 : 0;

   propagate_light();

   glBindTexture(GL_TEXTURE_2D, tex);
   glEnable(GL_TEXTURE_2D);

   get_map_color(&c, room_x, room_y, room_z);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0,140,104,0,-1,1);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   glBegin(GL_QUADS);
   for (y=-1; y <= SIZE_Y; ++y) {
      for (x=-1; x <= SIZE_X; ++x) {
         color *z, lit;
         int mx = ((room_x+NUM_X)*SIZE_X + x) % (SIZE_X*NUM_X);
         int my = ((room_y+NUM_Y)*SIZE_Y + y) % (SIZE_Y*NUM_Y);
         // draw map
         int t = tilemap[room_z][my][mx];

         if (any_light_for_dir[my][mx]) {
            int d;
            float r=255,g=255,b=255,a=1;
            for (d=0; d < 4; ++d)
               if (light_for_dir[my][mx][d] >= 0) {
                  int c = light_for_dir[my][mx][d];
                  r += powers[c].r;
                  g += powers[c].g;
                  b += powers[c].b;
                  a += 1;
               }
            lit.r = r/a;
            lit.g = g/a;
            lit.b = b/a;
            lit.a = 255;
            z = &lit;
         } else
            z = &c;

         draw_sprite(x+1,y+1, tile_sprite[t].s,tile_sprite[t].t, z, 1);
         if (t >= TILE_floor) {
            int r;
            r = (mx + 1); if (r == SIZE_X*NUM_X) r = 0;
            if (tilemap[room_z][my][r] == TILE_wall) draw_sprite(x+1,y+1, 1,0, z,1);
            r = (mx - 1); if (r < 0) r = SIZE_X*NUM_X - 1;
            if (tilemap[room_z][my][r] == TILE_wall) draw_sprite(x+1,y+1, 3,0, z,1);
            r = (my + 1); if (r == SIZE_Y*NUM_Y) r = 0;
            if (tilemap[room_z][r][mx] == TILE_wall) draw_sprite(x+1,y+1, 4,0, z,1);
            r = (my - 1); if (r < 0) r = SIZE_Y*NUM_Y - 1;
            if (tilemap[room_z][r][mx] == TILE_wall) draw_sprite(x+1,y+1, 2,0, z,1);
         }
      }
   }

   for (y=-1; y <= SIZE_Y; ++y) {
      for (x=-1; x <= SIZE_X; ++x) {
         color *z, lit;
         obj o;
         int mx = ((room_x+NUM_X)*SIZE_X + x) % (SIZE_X*NUM_X);
         int my = ((room_y+NUM_Y)*SIZE_Y + y) % (SIZE_Y*NUM_Y);
         int ds,sx,sy, t;

         if (any_light_for_dir[my][mx]) {
            int d;
            float r=255,g=255,b=255,a=1;
            for (d=0; d < 4; ++d)
               if (light_for_dir[my][mx][d] >= 0) {
                  int c = light_for_dir[my][mx][d];
                  r += powers[c].r;
                  g += powers[c].g;
                  b += powers[c].b;
                  a += 1;
               }
            lit.r = r/a;
            lit.g = g/a;
            lit.b = b/a;
            lit.a = 255;
            z = &lit;
         } else
            z = NULL;

         // draw obj
         o = objmap[room_z][my][mx];
         t = o.type;
         switch (obj_sprite_hasdir[t]) {
            case 1: ds = o.dir; break;
            case 2: ds = (animcycle>>7) % 3; break;
            default: ds = 0;
         }
         if (t == T_rover) {
            //if (x >= 0 && y >= 0 && x < SIZE_X && y < SIZE_Y) {
            int t = rover_timer - ROVER_MOVE_TIME/2;
            if (t > 0) {
               sx = -xdir[o.dir] * 16 * t / (ROVER_MOVE_TIME/2);
               sy = -ydir[o.dir] * 16 * t / (ROVER_MOVE_TIME/2);
            }
            //} else {
            //   sx = sy = 0;
            //}
         } else
            sx = sy = 0;
         draw_subsprite(x+1,y+1, sx,sy, obj_sprite[t].s+ds, obj_sprite[t].t, z, 1);

         if (x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y) {
            if (t == T_projector) {
               int p = has_power[my/SIZE_Y][mx/SIZE_X];
               draw_sprite(x+1,y+1, 4+p, 2, &powers[o.color], 0.5);
            }
         }
      }
   }

   // draw player
   if (player_timer > PLAYER_MOVE_TIME - PLAYER_TRAVEL_TIME) {
      epx = - (16 * xdir[pdir] * (player_timer - (PLAYER_MOVE_TIME - PLAYER_TRAVEL_TIME)) / PLAYER_TRAVEL_TIME);
      epy = - (16 * ydir[pdir] * (player_timer - (PLAYER_MOVE_TIME - PLAYER_TRAVEL_TIME))/ PLAYER_TRAVEL_TIME);
   } else {
      epx = epy  =0;
   }

   draw_subsprite(px - room_x*SIZE_X+1, py - room_y*SIZE_Y+1, epx, epy, pdir,6, 0,1);
   if (0) {
      int colors[4], num_colors = 0;
      int d = 0;
      color *c;
      for (d=0; d < 4; ++d)
         if (light_for_dir[py][px][d])
            colors[num_colors++] = light_for_dir[py][px][d];
      if (num_colors) {
         float a = (512 - abs((animcycle&1023) - 512)) / 512.0;
         c = &powers[colors[(animcycle >> 10) % num_colors]];
         draw_sprite(epx - room_x*SIZE_X+1, epy - room_y*SIZE_Y+1, 1,1, c,a*0.5);
      }
   }

   if (shot_timer > 0) {
      x = shot_x - room_x*SIZE_X + 1;
      y = shot_y - room_y*SIZE_Y + 1;
      x = x*16 - 12;
      y = y*16 - 12;
      glColor3f(1,1,1);
      draw_rect(x+4,y+4,8,8, 64,64,80,80);
   }

   glEnd();


   // do all additive rendering

   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glEnable(GL_BLEND);

   glBegin(GL_QUADS);
   for (y= -1; y <= SIZE_Y; ++y) {
      for (x= -1; x <= SIZE_X; ++x) {
         int mx = ((room_x+NUM_X)*SIZE_X + x) % (SIZE_X*NUM_X);
         int my = ((room_y+NUM_Y)*SIZE_Y + y) % (SIZE_Y*NUM_Y);
         obj o = objmap[room_z][my][mx];
         int t = o.type;
         if (t == T_projector) {
            if (has_power[my / SIZE_Y][mx / SIZE_X]) {
               draw_sprite(x+1,y+1, 4+1    , 2, &powers[o.color], flicker * 0.5);
               draw_sprite(x+1,y+1, 0+o.dir, 5, &powers[o.color], flicker * 0.25);
            }
         }
         if (any_light_for_dir[my][mx]) {
            int d;
            for (d=0; d < 4; ++d) {
               draw_sprite(x+1,y+1, 4+(d&1), 5, &powers[light_for_dir[my][mx][d]], flicker * 0.25);
            }
         }
      }
   }

   if (1) {
      int colors[4], num_colors = 0;
      int d = 0;
      color *c;
      for (d=0; d < 4; ++d)
         if (light_for_dir[py][px][d] >= 0)
            colors[num_colors++] = light_for_dir[py][px][d];
      if (num_colors) {
         float a = (512 - abs((animcycle&1023) - 512)) / 512.0;
         c = &powers[colors[(animcycle >> 10) % num_colors]];
         draw_subsprite(px - room_x*SIZE_X+1, py - room_y*SIZE_Y+1, epx,epy, 1,1, c,a*0.5);
      }
   }
   glEnd();

   sx = (SIZE_X+1) * 16 - 12 + 4;

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   glBegin(GL_QUADS);
      glColor3f(0,0,0);
      draw_rect(sx, 0, 120,160, 112,16,127,31);
   glEnd();


   sx = sx*800/140;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0,800,600,0,-1,1);

   glBegin(GL_QUADS);
   {
      char buffer[64];
      int x, y, sy;
      glColor3f(1,1,1);
      //draw_rect(0,0, sx,1, 127,31,127,31);
      //draw_rect(0,415, sx,1, 127,31,127,31);
      //draw_rect(0,0, 1,415, 127,31,127,31);
      //draw_rect(sx,0, 1,415, 127,31,127,31);
      draw_rect(sx,0,   210,2, 127,31,127,31);
      draw_rect(sx,598, 210,2, 127,31,127,31);
      draw_rect(sx,0,   2,600, 127,31,127,31);
      draw_rect(798,0,  2,600, 127,31,127,31);

      y = 15;
      glColor3f(0.5,0.5,0.5);
      draw_text(sx+30, y, 3, "PROMESST");

      y += 35;

      glColor3f(0.75,0.5,0.75);

      draw_text(sx+20, y, 2, "<>^v TO MOVE");
      y += 20;
      draw_text(sx+20, y, 2, "ESC FOR MENU");
      y += 20;
      draw_text(sx+20, y, 2, "BACKSPACE TO");
      y += 15;
      draw_text(sx+50, y, 2, "UNDO 1 ROOM");
      y += 20;
      if (state.has_wand) {
         float n = fabs((animcycle % 4000)/2000.0 - 1);
         glColor3f(0.75,n,0.75);
      } else
         glColor3f(0.25,0.25,0.25);
      draw_text(sx+20, y, 2, "Z TO FIRE");
      y += 20;
      if ((state.num_gems && tilemap[pz][py][px] == TILE_receptacle) || objmap[pz][py][px].type == T_gem)
         glColor3f(0.75,0.5,0.75);
      else
         glColor3f(0.25,0.25,0.25);
      x = draw_text(sx+20, y, 2, "X TO GET/PUT");
      glColor3f(1,1,1);
      draw_rect(x+2,y-3,16,16, 0,64,0+16,64+16);
      y += 35;

      if (state.num_gems) {
         sprintf(buffer, "%d X ", state.num_gems);
         glColor3f(0.75,0.5,0.75);
         x = draw_text(sx + 46, y, 3, buffer);
         glColor3f(1,1,1);
         draw_rect(x,y-7,28,28, 0,64,0+16,64+16);
      }

      y += 50;

      {
         uint8 abilities[8] = { 1,1,1,1, 1,1,1,1 };
         int i;
         int which[5] = { COLOR_red, COLOR_green, COLOR_teal, COLOR_yellow, COLOR_orange };
         char *cname[5] = { "RED" , "GREEN"  , "TEAL"  , "YELLOW", "ORANGE" };
         char *aname[5] = { "OPEN", "THROUGH", "STRONG", "SMASH" , "DOUBLE" };
         float ccol[5][3] = { 
            1.0,0.25,0.25,
            0.25,1.0,0.25,
            0.25,1.0,1.0 ,
            1.0,1.0,0.25 ,
            1.0,0.5,0.15 ,
         };
         get_abilities(abilities);
         glColor3f(0.5,0.5,0.5);
         draw_text(sx+8,y, 3, "/ABILITIES/");
         y += 10;

         for (i=0; i < 5; ++i) {
            int a = which[i];
            y += 25;
            if (abilities[a] || (state.ability_flag & (1 << a))) {
               char *s;
               glColor4f(ccol[i][0], ccol[i][1], ccol[i][2], abilities[a] ? 1 : 0.35);
               s = (state.ability_flag & (1 << a)) ? aname[i] : cname[i];
               draw_text(sx+100 - 3*strlen(s)*6/2, y, 3, s);
            }
         }

         #if 0
         #define SHOW_ABILITY(x)  (abilities[x] || (state.ability_flag & (1 << (x))))
         #define FLAG(x) (state.ability_flag & (1 << (x)))

         y += 25; if (SHOW_ABILITY(COLOR_red   )) {  a = abilities[COLOR_red   ] ? 1 : 0.25; glColor4f(1.0,0.25,0.25,a); draw_text(sx+80,y,3, !FLAG(COLOR_red   ) ? "RED"    : "DOOR" ); }
         y += 25; if (SHOW_ABILITY(COLOR_green )) {  a = abilities[COLOR_green ] ? 1 : 0.25; glColor4f(0.25,1.0,0.25,a); draw_text(sx+62,y,3, !FLAG(COLOR_green ) ? "GREEN"  : "THROUGH" ); }
         y += 25; if (SHOW_ABILITY(COLOR_teal  )) {  a = abilities[COLOR_teal  ] ? 1 : 0.25; glColor4f(0.25,1.0,1.0 ,a); draw_text(sx+71,y,3, !FLAG(COLOR_teal  ) ? "TEAL"   : "STRONG" ); }
         y += 25; if (SHOW_ABILITY(COLOR_yellow)) {  a = abilities[COLOR_yellow] ? 1 : 0.25; glColor4f(1.0,1.0,0.25 ,a); draw_text(sx+53,y,3, !FLAG(COLOR_yellow) ? "YELLOW" : "DESTROY" ); }
         y += 25; if (SHOW_ABILITY(COLOR_orange)) {  a = abilities[COLOR_orange] ? 1 : 0.25; glColor4f(1.0,0.5,0.15 ,a); draw_text(sx+53,y,3, !FLAG(COLOR_orange) ? "ORANGE" : "DOUBLE" ); }
         #endif
      }

      y += 33;
      glColor3f(0.5,0.5,0.5);
      draw_text(sx + 62, y, 3, "/MAP/");

      sy = y+25;
      for (y=0; y < NUM_Y*SIZE_Y; ++y) {
         int ry = y / SIZE_Y;
         for (x=0; x < NUM_X*SIZE_X; ++x) {
            int rx = x / SIZE_X;
            if (state.seen[room_z] & (1 << (ry*NUM_X+rx))) {
               float a = 0.5;
               if (rx == room_x && ry == room_y)
                  a = 1.0;
               if (room_z == pz && x == px && y == py)
                  glColor4f(0.7,0.5,0.7,a);
               else if (objmap[room_z][y][x].type == T_projector) {
                  color *c = &powers[objmap[room_z][y][x].color];
                  uint8 ia;
                  if (has_power[y/SIZE_Y][x/SIZE_X])
                     ia = (a < 1) ? 128 : 255;
                  else
                     ia = (a < 1) ?  64 : 192;
                     
                  glColor4ub(c->r, c->g, c->b, ia);
               } else if (tilemap[room_z][y][x] == TILE_wall || tilemap[room_z][y][x] == TILE_door) {
                  if (room_z)
                     glColor4f(0.4,0.5,0.6,a);
                  else
                     glColor4f(0.6,0.5,0.4,a);
               } else if (tilemap[room_z][y][x] == TILE_stairs) {
                  glColor4f(0.65,0.7,0.95,a);
               } else if (room_z == 1 && x == egg_x && y == egg_y) {
                  glColor4f(1,1,1,a);
               } else if (objmap[room_z][y][x].type == T_gem) {
                  if (objmap[room_z][y][x].dir)
                     glColor4f(0.8,0.7,0.8,a*(3+flicker)/4);
                  else
                     glColor4f(0.8,0.7,0.8,a);
               } else
                  glColor4f(0.25,0.25,0.25,a);
               draw_rect(sx+20+x*7,sy+y*7,7,7, 127,31,127,31);
            }
         }
      }
   }

   if (checkpoint_timer > 0) {
      glColor4f(0.5,1.0,0.5,flicker);
      draw_text(60, checkpoint_timer/2 - 120, 8, "CHECKPOINT");
      draw_text(160, checkpoint_timer/2 - 40, 8, "SAVED");
   }
   glEnd();


   if (state.egg_timer > 8000) {
      int a = (state.egg_timer - 8000) >> 4;
      int r = powers[COLOR_violet].r;
      int g = powers[COLOR_violet].g;
      int b = powers[COLOR_violet].b;
      if (a >= 128) {
         r += a-128; if (r >= 255) r = 255;
         g += a-128; if (g >= 255) g = 255;
         b += a-128; if (b >= 255) b = 255;
         if (a >= 255) a = 255;
      }
      glBlendFunc(GL_SRC_ALPHA, GL_ADD);
      glEnable(GL_BLEND);
      glColor4ub(r,g,b,a);
      glBegin(GL_QUADS);
      draw_rect(0,0, 800,600, 112,16,127,31);
      glEnd();
      if (state.egg_timer > 14000) {
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glEnable(GL_BLEND);
         glBegin(GL_QUADS);
         glColor3f(0,0,0);
         draw_text(150,250, 12, "YOU WIN");
         if (state.egg_timer > 17000) {
            float a = (state.egg_timer - 17000)/2000.0;
            char buffer[64];
            sprintf(buffer, "USED %d ZAPS", state.num_zaps);
            if (a > 1) a = 1;
            glColor4f(0.5,0,0.5, a);
            draw_text(250,400,4, buffer);
         }
         glEnd();
      }
      if (state.egg_timer > 22000)
         exit(0);  // shouldn't call save!
   }
}

void draw()
{
   draw_init();
   draw_world();
   stbwingraph_SwapBuffers(NULL);
}

static int initialized=0;
static float last_dt;

int active;

int loopmode(float dt, int real, int in_client)
{
   uint eff_time;
   float actual_dt = dt;

   if (!initialized) return 0;

   animcycle += (uint) ((dt) * 1000 * TIME_SCALE);

   if (main_mode != M_game) {
      process_metagame(dt);
   } else if (dt) {
      if (dt > 0.25) dt = 0.25;
      if (dt < 0.01) dt = 0.01;

      eff_time = (uint) ((dt) * 1000 * TIME_SCALE);
      animcycle += eff_time;

      timestep(eff_time);
   }

   if (active || !real)
      draw();

#ifdef _DEBUG
   return 0;
#else
   return active ? 0 : STBWINGRAPH_update_pause;
#endif
}

int winproc(void *data, stbwingraph_event *e)
{
   switch (e->type) {
      case STBWGE_create:
         active = 1;
         break;

      case STBWGE_char:
         switch(e->key) {
#if 0
            case 27:
               stbwingraph_ShowCursor(NULL,1);
                  return STBWINGRAPH_winproc_exit;
               break;
#endif
            default:
               queued_key = e->key;
               break;
         }
         break;

      case STBWGE_destroy:
         save_game();
         break;

      case STBWGE_keydown:
         switch (e->key) {
            case SDLK_DOWN : queued_key = 's'; break;
            case SDLK_LEFT : queued_key = 'a'; break;
            case SDLK_RIGHT: queued_key = 'd'; break;
            case SDLK_UP   : queued_key = 'w'; break;
         }
         break;
      case STBWGE_keyup:
         break;

      case STBWGE_deactivate:
         active = 0;
         break;
      case STBWGE_activate:
         active = 1;
         break;

      case STBWGE_size:
         screen_x = e->width;
         screen_y = e->height;
         loopmode(0,1,0);
         break;

      case STBWGE_draw:
         if (initialized)
            loopmode(0,1,0);
         break;

      default:
         return STBWINGRAPH_unprocessed;
   }
   return 0;
}

void stbwingraph_main(void)
{
   stbwingraph_Priority(2);
   stbwingraph_CreateWindow(1, winproc, NULL, APPNAME, SCREEN_X,SCREEN_Y, 0, 1, 0, 0);
   stbwingraph_ShowCursor(NULL, 0);

   init_graphics();

   restart_game(); // necessary to populate the edge table on restore
   load_game();
   if (game_started)
      menu_selection = 0;
   initialized = 1;
   stbwingraph_MainLoop(loopmode, 0.016f);   // 30 fps = 0.033
}