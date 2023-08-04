// stb_sdl2graph.h  v0.01 - public domain port of stb_wingraph
//
// stb_sdl2graph.h is far from feature complete. It, indeed, includes only
// the minimal set of features needed to get Promesst playable.
//
// There's no support for this (I know how hacky it is), but you can direct
// complaints, bugs, etc to david@ingeniumdigital.com
//
// in ONE source file, put '#define STB_DEFINE' before including this
// OR put '#define STB_WINMAIN' to define a WinMain that calls stbwingraph_main(void)
//
// @TODO:
//    2d rendering interface (that can be done easily in software)m
//    STB_WINGRAPH_SOFTWARE -- 2d software rendering only
//    STB_WINGRAPH_OPENGL   -- OpenGL only


#ifndef INCLUDE_STB_SDL2GRAPH_H
#define INCLUDE_STB_SDL2GRAPH_H

#ifdef STB_WINMAIN
   #ifndef STB_DEFINE
      #define STB_DEFINE
      #define STB_WINGRAPH_DISABLE_DEFINE_AT_END
   #endif
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef STB_DEFINE
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#endif

typedef void * stbwingraph_hwnd;
typedef void * stbwingraph_hinstance;

enum
{
   STBWINGRAPH_unprocessed = -(1 << 24),
   STBWINGRAPH_do_not_show,
   STBWINGRAPH_winproc_exit,
   STBWINGRAPH_winproc_update,
   STBWINGRAPH_update_exit,
   STBWINGRAPH_update_pause,
};

typedef enum
{
   STBWGE__none=0,

   STBWGE_create,
   STBWGE_create_postshow,
   STBWGE_draw,
   STBWGE_destroy,
   STBWGE_char,
   STBWGE_keydown,
   STBWGE_syskeydown,
   STBWGE_keyup,
   STBWGE_syskeyup,
   STBWGE_deactivate,
   STBWGE_activate,
   STBWGE_size,

   STBWGE_mousemove ,
   STBWGE_leftdown  , STBWGE_leftup  ,
   STBWGE_middledown, STBWGE_middleup,
   STBWGE_rightdown , STBWGE_rightup ,
   STBWGE_mousewheel,
} stbwingraph_event_type;

typedef struct
{
   stbwingraph_event_type type;

   // for input events (mouse, keyboard)
   int mx,my; // mouse x & y
   int dx,dy;
   int shift, ctrl, alt;

   // for keyboard events
   int key;

   // for STBWGE_size:
   int width, height;

   // for STBWGE_crate
   int did_share_lists;  // if true, wglShareLists succeeded

   void *handle;

} stbwingraph_event;

typedef int (*stbwingraph_window_proc)(void *data, stbwingraph_event *event);

extern stbwingraph_hwnd        stbwingraph_primary_window;
extern bool                    stbwingraph_request_fullscreen;
extern bool                    stbwingraph_request_windowed;

#ifdef STB_DEFINE
stbwingraph_hwnd        stbwingraph_primary_window;
bool stbwingraph_request_fullscreen;
bool stbwingraph_request_windowed;

typedef struct
{
   // app data
   stbwingraph_window_proc func;
   void *data;
   // creation parameters
   int   color, alpha, depth, stencil, accum;
   SDL_GLContext *share_context;
   SDL_GLContext *my_context;
   SDL_Window    *window;
   // internal data
   int   hide_mouse;
   int   in_client;
   int   active;
   int   did_share_lists;
   int   mx,my; // last mouse positions
} stbwingraph__window;

static void stbwingraph__key(stbwingraph_event *e, int type, int key, stbwingraph__window *z)
{
   e->type  = type;
   e->key   = key;
   e->shift = (SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT));
   e->ctrl  = (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL));
   e->alt   = (SDL_GetModState() & (KMOD_LALT | KMOD_RALT));
   if  (z) {
      e->mx    = z->mx;
      e->my    = z->my;
   } else {
      e->mx = e->my = 0;
   }
   e->dx = e->dy = 0;
}

static void stbwingraph__mouse(stbwingraph_event *e, int type, int x, int y, int capture, void *wnd, stbwingraph__window *z)
{
   static int captured = 0;
   e->type = type;
   e->mx = (short) x;
   e->my = (short) y;
   if (!z || z->mx == -(1 << 30)) {
      e->dx = e->dy = 0;
   } else {
      e->dx = e->mx - z->mx;
      e->dy = e->my - z->my;
   }
   e->shift = (SDL_GetModState() & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
   e->ctrl  = (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) != 0;
   e->alt   = (SDL_GetModState() & (KMOD_LALT | KMOD_RALT)) != 0;
   if (z) {
      z->mx = e->mx;
      z->my = e->my;
   }
   if (capture) {
           SDL_SetWindowGrab(z->window, capture > 0);
   }
}

#endif

#ifdef STB_WINGRAPH_DISABLE_DEFINE_AT_END
#undef STB_DEFINE
#endif

#endif // INCLUDE_STB_WINGRAPH_H
