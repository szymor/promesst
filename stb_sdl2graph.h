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

#ifdef __cplusplus
#define STB_EXTERN extern "C"
#else
#define STB_EXTERN
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef STB_DEFINE
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
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

extern stbwingraph_hinstance   stbwingraph_app;
extern stbwingraph_hwnd        stbwingraph_primary_window;
extern int                     stbwingraph_request_fullscreen;
extern int                     stbwingraph_request_windowed;

STB_EXTERN void stbwingraph_ods(char *str, ...);
STB_EXTERN int stbwingraph_MessageBox(stbwingraph_hwnd win, unsigned int type,
                                              char *caption, char *text, ...);
STB_EXTERN int stbwingraph_ChangeResolution(unsigned int w, unsigned int h,
                                      unsigned int bits, int use_message_box);
STB_EXTERN int stbwingraph_SetPixelFormat(stbwingraph_hwnd win, int color_bits,
            int alpha_bits, int depth_bits, int stencil_bits, int accum_bits);
STB_EXTERN int stbwingraph_DefineClass(void *hinstance, char *iconname);
STB_EXTERN void stbwingraph_SwapBuffers(void *win);

STB_EXTERN void stbwingraph_MakeFonts(void *window, int font_base);
STB_EXTERN float stbwingraph_GetTimestep(float minimum_time);
STB_EXTERN void stbwingraph_SetGLWindow(void *win);

#ifdef STB_DEFINE
stbwingraph_hinstance   stbwingraph_app;
stbwingraph_hwnd        stbwingraph_primary_window;
int stbwingraph_request_fullscreen;
int stbwingraph_request_windowed;

void stbwingraph_ods(char *str, ...)
{
   va_list v;
   va_start(v,str);
   vfprintf(stderr, str, v);
   va_end(v);
}

int stbwingraph_MessageBox(stbwingraph_hwnd win, unsigned int type, char *caption, char *text, ...)
{
   va_list v;
   char buffer[1024];
   va_start(v, text);
   vsprintf(buffer, text, v);
   va_end(v);
   return SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, buffer, win);
}

static void stbwingraph_ResetResolution(void)
{
    /* STUB: SDL ties resolution to a window, and resets automatically. */
}

static void stbwingraph_RegisterResetResolution(void)
{
    /* STUB: As above, SDL does not need this. */
}

int stbwingraph_ChangeResolution(unsigned int w, unsigned int h, unsigned int bits, int use_message_box)
{
}

int stbwingraph_SetPixelFormat(stbwingraph_hwnd win, int color_bits, int alpha_bits, int depth_bits, int stencil_bits, int accum_bits)
{
}

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

static void stbwingraph__inclient(stbwingraph__window *win, int state)
{
   if (state != win->in_client) {
      win->in_client = state;
      if (win->hide_mouse)
         SDL_ShowCursor(!state);
   }
}

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

float stbwingraph_GetTimestep(float minimum_time)
{
   float elapsedTime;
   double thisTime;
   static double lastTime = -1;

   if (lastTime == -1)
      lastTime = SDL_GetTicks() / 1000.0 - minimum_time;

   for(;;) {
      thisTime = SDL_GetTicks() / 1000.0;
      elapsedTime = (float) (thisTime - lastTime);
      if (elapsedTime >= minimum_time) {
         lastTime = thisTime;
         return elapsedTime;
      }
      #if 1
      SDL_Delay(5);
      #endif
   }
}

void stbwingraph_SetGLWindow(void *win)
{
   stbwingraph__window *z = (stbwingraph__window *) win;
   if (z)
      SDL_GL_MakeCurrent(z->window, z->my_context);
}

void stbwingraph_MakeFonts(void *window, int font_base)
{
   stbwingraph_ods("WARNING: MakeFonts not supported in SDL2\n");
}

void stbwingraph_SwapBuffers(void *win)
{
   stbwingraph__window *z;
   if (win == NULL) win = stbwingraph_primary_window;
   z = (stbwingraph__window *) win;
   SDL_GL_SwapWindow(z->window);
}
#endif

#undef STB_EXTERN
#ifdef STB_WINGRAPH_DISABLE_DEFINE_AT_END
#undef STB_DEFINE
#endif

#endif // INCLUDE_STB_WINGRAPH_H
