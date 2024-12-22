#include <X11/X.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct window_information {
  Window window;
  int height;
  int width;
} win_info;

void unmaximize_window(Display *display, Window window) {
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  if (wm_state == None || max_horz == None || max_vert == None) {
    fprintf(stderr, "Unable to retrieve required atoms.\n");
    return;
  }

  XClientMessageEvent event = {0};
  event.type = ClientMessage;
  event.window = window;
  event.message_type = wm_state;
  event.format = 32;
  event.data.l[0] = 0; // Remove maximized state
  event.data.l[1] = max_horz;
  event.data.l[2] = max_vert;

  if (!XSendEvent(display, DefaultRootWindow(display), False,
                  SubstructureRedirectMask | SubstructureNotifyMask,
                  (XEvent *)&event)) {
    fprintf(stderr, "Failed to send the event to unmaximize the window.\n");
  }

  XFlush(display);
}

int error_handler(Display *display, XErrorEvent *error) {
  if (error->error_code == BadWindow) {
    printf("Caught BadWindow error: invalid window ID 0x%lx\n",
           error->resourceid);
  } else {
    printf("Caught other error: %d\n", error->error_code);
  }
  return 0; // Return 0 to prevent the program from terminating
}

void process_window(Display *display, Window window, Atom net_wm_desktop,
                    Atom net_wm_name, Atom utf8_string,
                    unsigned long current_desktop, Window active_win) {
  unsigned long window_desktop = -1;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  if (XGetWindowProperty(display, window, net_wm_desktop, 0, 1, False,
                         XA_CARDINAL, &actual_type, &actual_format, &nitems,
                         &bytes_after, &prop) == Success &&
      prop) {
    window_desktop = *(unsigned long *)prop;
    XFree(prop);
  }

  if (window_desktop == current_desktop || window == active_win) {
    // Unmaximize the window
    unmaximize_window(display, window);
  }
}

void get_window_dimensions(Display *display, Window window, int *width,
                           int *height) {
  if (display == NULL) {
    fprintf(stderr, "Display is NULL.\n");
    return;
  }

  if (window == None) {
    fprintf(stderr, "Invalid window.\n");
    return;
  }

  if (width == NULL || height == NULL) {
    fprintf(stderr, "Width or height pointer is NULL.\n");
    return;
  }

  printf("\nWIN: %ld \n", window);
  // Get window dimensions
  XWindowAttributes attributes;
  int status = XGetWindowAttributes(display, window, &attributes);
  if (status == 0) {
    fprintf(stderr,
            "Error: Failed to get window attributes for window ID: %lu\n",
            (unsigned long)window);
    return;
  }
  printf("Got attribute... \n");

  *width = attributes.width;
  *height = attributes.height;

  printf("width=%d, height=%d\n", *width, *height);
}

void arrange_workspace(Display *display, Window root) {
  Atom net_client_list = XInternAtom(display, "_NET_CLIENT_LIST", False);
  Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", False);
  Atom net_current_desktop =
      XInternAtom(display, "_NET_CURRENT_DESKTOP", False);
  Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
  Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);
  Atom active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

  if (net_client_list == None || net_wm_desktop == None ||
      net_current_desktop == None) {
    fprintf(stderr, "Unable to retrieve required atoms.\n");
    return;
  }

  unsigned long current_desktop = 0;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *prop = NULL;

  Atom net_active_window = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
  if (net_active_window == None) {
    fprintf(stderr, "_NET_ACTIVE_WINDOW atom not found.\n");
    return;
  }

  if (XGetWindowProperty(display, root, net_active_window, 0, (~0L), False,
                         XA_WINDOW, &actual_type, &actual_format, &nitems,
                         &bytes_after, &prop) != Success) {
    fprintf(stderr, "Failed to retrieve _NET_ACTIVE_WINDOW property.\n");
    return;
  }

  if (actual_type != XA_WINDOW || nitems == 0) {
    fprintf(stderr, "_NET_ACTIVE_WINDOW does not return a valid window.\n");
    if (prop)
      XFree(prop);
    return;
  }

  Window active_curr_buf = *(Window *)prop;
  if (prop)
    XFree(prop);

  unmaximize_window(display, active_curr_buf);

  // Get the current desktop
  if (XGetWindowProperty(display, root, net_current_desktop, 0, 1, False,
                         XA_CARDINAL, &actual_type, &actual_format, &nitems,
                         &bytes_after, &prop) == Success &&
      prop) {
    current_desktop = *(unsigned long *)prop;
    XFree(prop);
  }

  // Get the active window
  Window active_win = 0;
  if (XGetWindowProperty(display, root, active_window, 0, 1, False, XA_WINDOW,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &prop) == Success &&
      prop) {
    active_win = *(Window *)prop;
    XFree(prop);
  }

  // Query and process children windows
  Window parent, *children;
  unsigned int nchildren;

  if (XQueryTree(display, root, &root, &parent, &children, &nchildren) != 0 &&
      children) {
    for (unsigned int i = 0; i < nchildren; i++) {
      process_window(display, children[i], net_wm_desktop, net_wm_name,
                     utf8_string, current_desktop, active_win);
    }
    XFree(children);
  }
}

void arrange_window(int window_count, Window windows[], Display *display,
                    int screen) {
  Screen *scr = ScreenOfDisplay(display, screen);
  int screen_width = scr->width;
  int screen_height = scr->height - 50; // Leaving some space at the bottom

  int width = screen_width / 2;
  int height = screen_height / 2;

  // Handling 1 window
  if (window_count == 1) {
    XMoveResizeWindow(display, windows[0], 0, 0, screen_width,
                      screen_height - 10);
  }
  // Handling 2 windows (1 left, 1 right)
  else if (window_count == 2) {
    int width = screen_width / 2;
    XMoveResizeWindow(display, windows[0], 0, 0, width, screen_height - 10);
    XMoveResizeWindow(display, windows[1], width, 0, width, screen_height);
  }
  // Handling 3 windows (1 left, 2 right)
  else if (window_count == 3) {
    XMoveResizeWindow(display, windows[0], 0, 0, width, screen_height - 10);
    XMoveResizeWindow(display, windows[1], width, 0, width, height);
    XMoveResizeWindow(display, windows[2], width, height, width, height);
  }
  // Handling 4 windows (2 left, 2 right)
  else if (window_count == 4) {
    XMoveResizeWindow(display, windows[0], 0, 0, width, height);
    XMoveResizeWindow(display, windows[1], 0, height, width, height);
    XMoveResizeWindow(display, windows[2], width, 0, width, height);
    XMoveResizeWindow(display, windows[3], width, height, width, height);
  }
  // Handling 5+ windows (2 left, 2 right, additional windows)
  else if (window_count >= 5) {
    // First 4 windows are arranged as above
    XMoveResizeWindow(display, windows[0], 0, 0, width, height);
    XMoveResizeWindow(display, windows[1], 0, height, width, height);
    XMoveResizeWindow(display, windows[2], width, 0, width, height);
    XMoveResizeWindow(display, windows[3], width, height, width, height);

    // Remaining windows start filling the right side
    for (int i = 4; i < window_count; i++) {
      if (i % 2 == 0) {
        // First row of the right side (windows 4, 6, 8, ...)
        XMoveResizeWindow(display, windows[i], width, 0, width, height);
      } else {
        // Second row of the right side (windows 5, 7, 9, ...)
        XMoveResizeWindow(display, windows[i], width, height, width, height);
      }
    }
  }
}

Window *fetch_window_list(Display *display, Window root, unsigned long *nitems,
                          Atom atom) {
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;
  Window *windows = NULL;

  if (XGetWindowProperty(display, root, atom, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, nitems, &bytes_after,
                         (unsigned char **)&windows) != Success) {
    fprintf(stderr, "Failed to fetch window list.\n");
    return NULL;
  }

  if (*nitems == 0) {
    fprintf(stderr, "No windows found.\n");
    if (windows) {
      XFree(windows);
    }
    return NULL;
  }

  return windows;
}

int main() {
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Cannot open display.\n");
  }

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  unsigned long nitems = 0;
  win_info *create_wi, *cur_wi;
  int is_arranged = 0;

  Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", True);
  Window *windows = fetch_window_list(display, root, &nitems, atom);
  if (windows) {
    create_wi = (win_info *)malloc(nitems);
    if (nitems > 0) {
      arrange_workspace(display, root);
      arrange_window(nitems, windows, display, screen);
      is_arranged = 1;
    }
    XFree(windows);
  }

  cur_wi = (win_info *)malloc(nitems);
  unsigned long curnitems = 0;

  XSetErrorHandler(error_handler);

  while (1) {
    Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", True);
    Window *cur_win = fetch_window_list(display, root, &curnitems, atom);

    printf("Previous Windows: %ld, Current windows: %ld \n", nitems, curnitems);

    if (curnitems != nitems) {
      printf("Re-arrange... \n");
      nitems = curnitems;
      for (unsigned long int i = 0; i < nitems; i++) {
        unmaximize_window(display, cur_win[i]);
      }
      arrange_window(nitems, cur_win, display, screen);
      printf("After rearrange \n");
    }

    if (curnitems > 1) {
      for (unsigned long int i = 0; i < curnitems; i++) {
        printf("\nTry to get dimensions... \n");
        get_window_dimensions(display, cur_win[i], &cur_wi[i].width,
                              &cur_wi[i].height);

        if (cur_wi[i].width != create_wi[i].width ||
            cur_wi[i].height != create_wi[i].height) {

          printf("Unmaximized windows... \n");
          unmaximize_window(display, windows[i]);

          printf("Arranged windows... \n");
          arrange_window(nitems, cur_win, display, screen);

          create_wi[i].width = cur_wi[i].width;
          create_wi[i].height = cur_wi[i].height;
        }
      }
      printf("\n");
    }
    XFree(cur_win);
    usleep(30000);
  }
  free(create_wi);
  XCloseDisplay(display);
  return 0;
}
