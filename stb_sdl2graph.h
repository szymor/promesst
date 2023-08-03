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
//    2d rendering interface (that can be done easily in software)
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
STB_EXTERN void stbwingraph_Priority(int n);

STB_EXTERN void stbwingraph_MakeFonts(void *window, int font_base);
STB_EXTERN void stbwingraph_ShowWindow(void *window);
STB_EXTERN void *stbwingraph_CreateWindow(int primary, stbwingraph_window_proc func, void *data, char *text, int width, int height, int fullscreen, int resizeable, int dest_alpha, int stencil);
STB_EXTERN void *stbwingraph_CreateWindowSimple(stbwingraph_window_proc func, int width, int height);
STB_EXTERN void *stbwingraph_CreateWindowSimpleFull(stbwingraph_window_proc func, int fullscreen, int ww, int wh, int fw, int fh);
STB_EXTERN void stbwingraph_DestroyWindow(void *window);
STB_EXTERN void stbwingraph_ShowCursor(void *window, int visible);
STB_EXTERN float stbwingraph_GetTimestep(float minimum_time);
STB_EXTERN void stbwingraph_SetGLWindow(void *win);
typedef int (*stbwingraph_update)(float timestep, int real, int in_client);
STB_EXTERN int stbwingraph_MainLoop(stbwingraph_update func, float mintime);

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

void stbwingraph_Priority(int n)
{
/* STUB: Implement setting thread priority */
#if 0
   int p;
   switch (n) {
      case -1: p = THREAD_PRIORITY_BELOW_NORMAL; break;
      case 0: p = THREAD_PRIORITY_NORMAL; break;
      case 1: p = THREAD_PRIORITY_ABOVE_NORMAL; break;
      default:
         if (n < 0) p = THREAD_PRIORITY_LOWEST;
         else p = THREAD_PRIORITY_HIGHEST;
   }
   SetThreadPriority(GetCurrentThread(), p);
#endif
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

void stbwingraph_ShowWindow(void *window)
{
   stbwingraph_event e = { STBWGE_create_postshow };
   stbwingraph__window *z = (stbwingraph__window *) window;
   SDL_ShowWindow(z->window);
   e.handle = window;
   z->func(z->data, &e);
}

void *stbwingraph_CreateWindow(int primary, stbwingraph_window_proc func, void *data, char *text,
           int width, int height, int fullscreen, int resizeable, int dest_alpha, int stencil)
{
   stbwingraph__window *z = (stbwingraph__window *) malloc(sizeof(*z));
   unsigned int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

   if (z == NULL) return NULL;
   memset(z, 0, sizeof(*z));
   z->color = 24;
   z->depth = 24;
   z->alpha = dest_alpha;
   z->stencil = stencil;
   z->func = func;
   z->data = data;
   z->mx = -(1 << 30);
   z->my = 0;

   if (primary) {
      if (stbwingraph_request_windowed)
         fullscreen = FALSE;
      else if (stbwingraph_request_fullscreen)
         fullscreen = TRUE;
   }


   z->window = SDL_CreateWindow(text ? text : "sample", 
                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
                      (fullscreen?SDL_WINDOW_FULLSCREEN:0) |
        	      (resizeable?SDL_WINDOW_RESIZABLE:0) |
        	      SDL_WINDOW_OPENGL);

   SDL_SetWindowData(z->window, "stbptr", z);

   z->my_context = SDL_GL_CreateContext(z->window);

   if (primary) {
      if (stbwingraph_primary_window)
         stbwingraph_DestroyWindow(stbwingraph_primary_window);
      stbwingraph_primary_window = z;
   }

   {
      stbwingraph_event e = { STBWGE_create };
      e.did_share_lists = z->did_share_lists;
      e.handle = z;
      if (z->func(z->data, &e) != STBWINGRAPH_do_not_show)
         stbwingraph_ShowWindow(z);
   }

	/*
	 * SDL doesn't sent a SIZE event when the window is created, so send one.
	 */
	{
		stbwingraph_event e = { STBWGE_size };
		e.width = width;
		e.height = height;
		e.handle = z;
		if (z && z->func)
			z->func(z->data, &e);
	}

   return z;
}

void *stbwingraph_CreateWindowSimple(stbwingraph_window_proc func, int width, int height)
{
   int fullscreen = 0;
#if 0
   #ifndef _DEBUG
   if (width ==  640 && height ==  480) fullscreen = 1;
   if (width ==  800 && height ==  600) fullscreen = 1;
   if (width == 1024 && height ==  768) fullscreen = 1;
   if (width == 1280 && height == 1024) fullscreen = 1;
   if (width == 1600 && height == 1200) fullscreen = 1;
   //@TODO: widescreen widths
   #endif
#endif
   return stbwingraph_CreateWindow(1, func, NULL, NULL, width, height, fullscreen, 1, 0, 0);
}

void *stbwingraph_CreateWindowSimpleFull(stbwingraph_window_proc func, int fullscreen, int ww, int wh, int fw, int fh)
{
   if (fullscreen == -1) {
#if 0
   #ifdef _DEBUG
      fullscreen = 0;
   #else
      fullscreen = 1;
   #endif
#endif
   }

   if (fullscreen) {
      if (fw) ww = fw;
      if (fh) wh = fh;
   }
   return stbwingraph_CreateWindow(1, func, NULL, NULL, ww, wh, fullscreen, 1, 0, 0);
}

void stbwingraph_DestroyWindow(void *window)
{
   stbwingraph__window *z = (stbwingraph__window *) window;
   SDL_DestroyWindow(z->window);
   free(z);
   if (stbwingraph_primary_window == window)
      stbwingraph_primary_window = NULL;
}

void stbwingraph_ShowCursor(void *window, int visible)
{
   int hide;
   stbwingraph__window *win;
   if (!window)
      window = stbwingraph_primary_window;
   win = (stbwingraph__window *) window;
   hide = !visible;
   if (hide != win->hide_mouse) {
      win->hide_mouse = hide;
      if (!hide)
         SDL_ShowCursor(SDL_TRUE);
      else if (win->in_client)
         SDL_ShowCursor(SDL_FALSE);
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

// returns 1 if WM_QUIT, 0 if 'func' returned 0
int stbwingraph_MainLoop(stbwingraph_update func, float mintime)
{
   int needs_drawing = FALSE;
   int stbwingraph_force_update = FALSE;
   SDL_Event msg;

   int is_animating = TRUE;
   if (mintime <= 0) mintime = 0.01f;

   for(;;) {
      int n;

      // wait for a message if: (a) we're animating and there's already a message
      // or (b) we're not animating

      SDL_PumpEvents();

      if (!is_animating || SDL_PeepEvents(&msg, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
         stbwingraph_force_update = FALSE;

         while (SDL_PollEvent(&msg))
         {
            stbwingraph_event e = { STBWGE__none };
            stbwingraph__window *z;

            switch (msg.type)
            {
               case SDL_QUIT: 
                  return 0;
               case SDL_WINDOWEVENT:
                  switch (msg.window.event)
                  {
                     case SDL_WINDOWEVENT_CLOSE:
                        e.type = STBWGE_destroy;
                        e.handle = SDL_GetWindowData(SDL_GetWindowFromID(msg.window.windowID),"stbptr");
								z = (stbwingraph__window*)e.handle;
                        if (z && z->func)
                           z->func(z->data, &e);
                        if (e.handle == stbwingraph_primary_window)
                           return 0;
                        break;
                     case SDL_WINDOWEVENT_RESIZED:
							case SDL_WINDOWEVENT_SIZE_CHANGED:
                        e.type = STBWGE_size;
                        e.handle = SDL_GetWindowData(SDL_GetWindowFromID(msg.window.windowID),"stbptr");
                        e.width = msg.window.data1;
                        e.height = msg.window.data2;
								z = (stbwingraph__window*)e.handle;
                        if (z && z->func)
                           z->func(e.handle, &e);
                        break;
                   }
					case SDL_MOUSEMOTION:
						z = (stbwingraph__window*) SDL_GetWindowData(SDL_GetWindowFromID(msg.motion.windowID), "stbptr");
						stbwingraph__mouse(&e, STBWGE_mousemove, msg.motion.x, msg.motion.y, 0, z, z);
						e.handle = z;
						if (z && z->func)
							n = z->func(z->data, &e);
						if (n == STBWINGRAPH_winproc_update)
							stbwingraph_force_update = TRUE;
						break;
					case SDL_MOUSEBUTTONDOWN:
					case SDL_MOUSEBUTTONUP:
						z = (stbwingraph__window*) SDL_GetWindowData(SDL_GetWindowFromID(msg.motion.windowID), "stbptr");
						if (msg.type == SDL_MOUSEBUTTONDOWN)
						{
							if (msg.button.button == SDL_BUTTON_LEFT) e.type = STBWGE_leftdown;
							else if (msg.button.button == SDL_BUTTON_RIGHT) e.type = STBWGE_rightdown;
							else if (msg.button.button == SDL_BUTTON_MIDDLE) e.type = STBWGE_middledown;
						}
						else
						{
							if (msg.button.button == SDL_BUTTON_LEFT) e.type = STBWGE_leftup;
							else if (msg.button.button == SDL_BUTTON_RIGHT) e.type = STBWGE_rightup;
							else if (msg.button.button == SDL_BUTTON_MIDDLE) e.type = STBWGE_middleup;
						}

						stbwingraph__mouse(&e, e.type, msg.button.x, msg.button.y, 0, z, z);
						e.handle = z;
						if (z && z->func)
							n = z->func(z->data, &e);
						if (n == STBWINGRAPH_winproc_update)
							stbwingraph_force_update = TRUE;
						break;
					case SDL_KEYDOWN:
					case SDL_KEYUP:
						z = (stbwingraph__window*) SDL_GetWindowData(SDL_GetWindowFromID(msg.key.windowID), "stbptr");
						stbwingraph__key(&e, (msg.type == SDL_KEYDOWN)?STBWGE_keydown:STBWGE_keyup, msg.key.keysym.sym, z);
						e.handle = z;
						if (z && z->func)
							n = z->func(z->data, &e);
						if (n == STBWINGRAPH_winproc_update)
							stbwingraph_force_update = TRUE;
						/* We emulate WM_CHAR events, as SDL2's text input API is clearly overdoing it */
						if (msg.type == SDL_KEYDOWN && msg.key.keysym.sym < 0x40000000)
						{
							stbwingraph_event ce = { STBWGE__none };
							stbwingraph__key(&ce, STBWGE_char, msg.key.keysym.sym, z);
							ce.handle = z;
							if (z && z->func)
								n = z->func(z->data, &ce);
							if (n == STBWINGRAPH_winproc_update)
								stbwingraph_force_update = TRUE;
						}
						break;

			 }
		 // only force a draw for certain messages...
		 // if I don't do this, we peg at 50% for some reason... must
		 // be a bug somewhere, because we peg at 100% when rendering...
		 // very weird... looks like NVIDIA is pumping some messages
		 // through our pipeline? well, ok, I guess if we can get
		 // non-user-generated messages we have to do this
		 if (!stbwingraph_force_update) {
		    switch (msg.type) {
		       case SDL_MOUSEMOTION:
			  break;
		       case SDL_TEXTEDITING:
		       case SDL_TEXTINPUT:
		       case SDL_KEYDOWN:      
		       case SDL_KEYUP:        
		       case SDL_MOUSEBUTTONDOWN:  
		       case SDL_MOUSEBUTTONUP:  
		       case SDL_WINDOWEVENT:  
			  needs_drawing = TRUE;
			  break;
		    }
		 } else
		    needs_drawing = TRUE;
	      }
      }

      // if another message, process that first
      // @TODO: i don't think this is working, because I can't key ahead
      // in the SVT demo app
      if (SDL_PeepEvents(&msg, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))
         continue;

      // and now call update
      if (needs_drawing || is_animating) {
         int real=1, in_client=1;
         if (stbwingraph_primary_window) {
            stbwingraph__window *z = (stbwingraph__window *) stbwingraph_primary_window;
            if (z && !z->active) {
               real = 0;
            }
            if (z)
               in_client = z->in_client;
         }

         if (stbwingraph_primary_window)
            stbwingraph_SetGLWindow(stbwingraph_primary_window);
         n = func(stbwingraph_GetTimestep(mintime), real, in_client);
         if (n == STBWINGRAPH_update_exit)
            return 0; // update_quit

         is_animating = (n != STBWINGRAPH_update_pause);

         needs_drawing = FALSE;
      }
   }
}

void stbwingraph_SwapBuffers(void *win)
{
   stbwingraph__window *z;
   if (win == NULL) win = stbwingraph_primary_window;
   z = (stbwingraph__window *) win;
   SDL_GL_SwapWindow(z->window);
}
#endif

#ifdef STB_WINMAIN    
void stbwingraph_main(void);

int main(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; ++i)
	{
		if ((strcmp(argv[i],"-window") == 0) || (strcmp(argv[i],"-windowed")))
		{
			stbwingraph_request_windowed = TRUE;
		}
		else if ((strcmp(argv[i],"-full") == 0) || (strcmp(argv[i],"-fullscreen")))
		{
			stbwingraph_request_fullscreen = TRUE;
		}
	}

	stbwingraph_main();
	return 0;
}
#endif

#undef STB_EXTERN
#ifdef STB_WINGRAPH_DISABLE_DEFINE_AT_END
#undef STB_DEFINE
#endif

#endif // INCLUDE_STB_WINGRAPH_H
