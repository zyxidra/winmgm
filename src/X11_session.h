
#include "winmgm.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WIN_APP_LIMIT 5

typedef struct window_information
{
  Window window;
  int height;
  int width;
} win_info;

void unmaximize_window (Display *display, Window window);
void process_window (Display *display, Window window, Atom net_wm_desktop,
		     Atom net_wm_name, Atom utf8_string,
		     unsigned long current_desktop, Window active_win);
void get_window_dimensions (Display *display, Window window, int *width,
			    int *height);
void arrange_window (int window_count, Window windows[], Display *display,
		     int screen);
Window *fetch_window_list (Display *display, Window root, unsigned long *nitems,
			   Atom atom, int workspace_id);
unsigned long int get_current_workspace (Display *display, Window root);
void run_x11_layout ();
