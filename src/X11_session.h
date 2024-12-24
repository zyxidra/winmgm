
#include "winmgm.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct window_information {
  Window window;
  int height;
  int width;
} win_info;

void run_x11_layout();
