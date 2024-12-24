/*
 * This file is part of winmgm.
 *
 * winmgm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * winmgm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with winmgm.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2024 Ardinugraha
 */

#include "X11_session.h"
#include "winmgm.h"
#include <X11/Xlib.h>

const int WIN_APP_LIMIT = 5;

static Display *initialize_display() {
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    LOG(WINMGM_ERR, "Cannot open display.\n");
  }
  return display;
}

static Window get_root_window(Display *display) {
  return RootWindow(display, DefaultScreen(display));
}

static int error_handler(Display *display, XErrorEvent *error) {
  if (error->error_code == BadWindow) {
    LOG(WINMGM_ERR,
        "Caught BadWindow error: invalid window ID "
        "0x%lx\n",
        error->resourceid);
  } else {
    LOG(WINMGM_ERR, " Caught other error: %d\n", error->error_code);
  }
  return 0; // Return 0 to prevent the program from terminating
}

static void unmaximize_window(Display *display, Window window) {
  Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
  Atom max_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom max_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);

  if (wm_state == None || max_horz == None || max_vert == None) {
    LOG(WINMGM_ERR, "Unable to retrieve required atoms.\n");
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
    LOG(WINMGM_ERR, "Failed to send the event to unmaximize the window.\n");
  }
  XFlush(display);
}

static void get_window_dimensions(Display *display, Window window, int *width,
                                  int *height) {
  if (display == NULL) {
    LOG(WINMGM_ERR, "Display is NULL.\n");
    return;
  }

  if (window == None) {
    LOG(WINMGM_ERR, "Invalid window.\n");
    return;
  }

  if (width == NULL || height == NULL) {
    LOG(WINMGM_ERR, "Width or height pointer is NULL.\n");
    return;
  }

  // Get window dimensions
  XWindowAttributes attributes;
  int status = XGetWindowAttributes(display, window, &attributes);
  if (status == 0) {
    LOG(WINMGM_ERR,
        "Error: Failed to get window attributes for window ID: "
        "%lu\n",
        (unsigned long)window);
    return;
  }

  *width = attributes.width;
  *height = attributes.height;
}

static void arrange_window(int window_count, Window windows[], Display *display,
                           int screen) {
  Screen *scr = ScreenOfDisplay(display, screen);
  int screen_width = scr->width;
  int screen_height = scr->height;

  int half_width = screen_width / 2;
  int half_height = screen_height / 2;

  for (int i = 0; i < window_count; i++) {
    int x = 0, y = 0, width = 0, height = 0;

    switch (window_count) {
    case 1:
      // Single window takes full screen
      x = 0;
      y = 0;
      width = screen_width;
      height = screen_height - 10;
      break;

    case 2:
      // Two windows split left and right
      width = half_width;
      height = screen_height;
      x = i * half_width;
      y = 0;
      break;

    case 3:
      if (i == 0) {
        // First window occupies left half
        x = 0;
        y = 0;
        width = half_width;
        height = screen_height;
      } else {
        // Remaining windows split the right half
        x = half_width;
        y = (i - 1) * half_height;
        width = half_width;
        height = half_height;
      }
      break;

    case 4:
      // Four windows split into quadrants
      width = half_width;
      height = half_height;
      x = (i % 2) * half_width;
      y = (i / 2) * half_height;
      break;

    default:
      // For more than 4 windows, stack them vertically on the left side
      x = 0;
      y = 0;
      width = screen_width;
      height = screen_height - 10;
      break;
    }

    XMoveResizeWindow(display, windows[i], x, y, width, height);
  }
}

static Window *fetch_window_list(Display *display, Window root,
                                 unsigned long *nitems, Atom atom,
                                 int workspace_id) {
  Atom actual_type;
  int actual_format;
  unsigned long bytes_after;
  Window *windows = NULL;

  // Retrieve all windows
  if (XGetWindowProperty(display, root, atom, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, nitems, &bytes_after,
                         (unsigned char **)&windows) != Success) {
    LOG(WINMGM_ERR, "Failed to fetch window list.\n");
    return NULL;
  }

  if (*nitems == 0) {
    LOG(WINMGM_INFO, "No windows found.\n");
    if (windows != NULL) {
      XFree(windows);
    }
    return NULL;
  }

  Window *filtered_windows = malloc(sizeof(Window) * (*nitems));
  if (filtered_windows == NULL) {
    LOG(WINMGM_ERR, " fail to allocate filtered_windows. \n");
    XFree(windows);
    return NULL;
  }

  unsigned long filtered_count = 0;

  for (unsigned long i = 0; i < *nitems; i++) {
    Window window = windows[i];
    Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", True);
    if (net_wm_desktop == None) {
      LOG(WINMGM_WARN, "_NET_WM_DESKTOP atom is not supported by the "
                       "X server.\n");
      XFree(filtered_windows);
      XFree(windows); // Free the original windows list before
      // returning
      return NULL;
    }

    Atom actual_type;
    int actual_format;
    unsigned long nitems_returned, bytes_after;
    unsigned char *data = NULL;
    if (XGetWindowProperty(display, window, net_wm_desktop, 0, 1, False,
                           XA_CARDINAL, &actual_type, &actual_format,
                           &nitems_returned, &bytes_after, &data) == Success &&
        data) {
      unsigned long window_workspace_id = *(unsigned long *)data;
      XFree(data);

      if (window_workspace_id == workspace_id) {
        filtered_windows[filtered_count++] = window;
      }
    }
  }

  *nitems = filtered_count;

  if (windows != NULL) {
    XFree(windows); // Only use XFree to free the windows
  }

  return filtered_windows;
}

static unsigned long int get_current_workspace(Display *display, Window root) {
  Atom currentDesktopAtom;
  unsigned long *desktop = NULL;
  Atom actualType;
  int actualFormat;
  unsigned long nItems, bytesAfter;

  // Get the _NET_CURRENT_DESKTOP atom
  currentDesktopAtom = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);
  if (currentDesktopAtom == None) {
    LOG(WINMGM_ERR, "_NET_CURRENT_DESKTOP not supported by the window "
                    "manager\n");
    XCloseDisplay(display);
    return -1;
  }

  // Query the _NET_CURRENT_DESKTOP property
  if (XGetWindowProperty(display, root, currentDesktopAtom, 0, 1, False,
                         XA_CARDINAL, &actualType, &actualFormat, &nItems,
                         &bytesAfter, (unsigned char **)&desktop) == Success &&
      desktop) {
    int workspaceNumber = (int)*desktop;
    XFree(desktop);
    return workspaceNumber;
  } else {
    LOG(WINMGM_ERR, "Failed to get the current desktop\n");
    XCloseDisplay(display);
    return -1;
  }
}

int move_window_to_workspace(Display *display, Window window, int workspace) {
  Atom net_wm_desktop = XInternAtom(display, "_NET_WM_DESKTOP", True);
  if (net_wm_desktop == None) {
    LOG(WINMGM_ERR, "_NET_WM_DESKTOP atom is not supported by the X server.\n");
    return -1;
  }

  // Set the _NET_WM_DESKTOP property
  XClientMessageEvent event = {0};
  event.type = ClientMessage;
  event.window = window;
  event.message_type = net_wm_desktop;
  event.format = 32;
  event.data.l[0] = workspace;   // Target workspace
  event.data.l[1] = CurrentTime; // Timestamp (use CurrentTime for "now")

  if (XSendEvent(display, DefaultRootWindow(display), False,
                 SubstructureNotifyMask | SubstructureRedirectMask,
                 (XEvent *)&event)) {
    XFlush(display);
    LOG(WINMGM_INFO, "Moved window 0x%lx to workspace %d\n", window, workspace);
    return 0;
  } else {
    LOG(WINMGM_ERR,
        "Failed to send event to move window 0x%lx to "
        "workspace %d.\n",
        window, workspace);
    return -1;
  }
}

Window get_last_opened_window(Display *display) {
  Atom net_client_list_stacking =
      XInternAtom(display, "_NET_CLIENT_LIST_STACKING", True);
  if (net_client_list_stacking == None) {
    LOG(WINMGM_ERR, "_NET_CLIENT_LIST_STACKING atom is not supported by "
                    "the X server.\n");
    return 0;
  }

  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;

  // Retrieve the stacking order of client windows
  if (XGetWindowProperty(display, DefaultRootWindow(display),
                         net_client_list_stacking, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &data) == Success &&
      data) {
    Window *stacking_list = (Window *)data;
    Window last_window =
        stacking_list[nitems - 1]; // The last window in the list
    XFree(data);
    return last_window;
  }

  return 0;
}

static unsigned long get_total_workspace(Display *display, Window root) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = NULL;
  unsigned long total_workspaces = 0;
  Atom desktops_atom = XInternAtom(display, "_NET_NUMBER_OF_DESKTOPS", True);

  if (XGetWindowProperty(display, root, desktops_atom, 0, 1, False, XA_CARDINAL,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &data) == Success) {
    if (data) {
      total_workspaces = *(unsigned long *)data;
      XFree(data);
    } else {
      fprintf(stderr, "Failed to retrieve _NET_NUMBER_OF_DESKTOPS data.\n");
    }
  } else {
    fprintf(stderr, "XGetWindowProperty failed for _NET_NUMBER_OF_DESKTOPS.\n");
  }

  return total_workspaces;
}

static void manage_workspace_limit(Display *display, Window root, int screen,
                                   int *base_workspace_num) {
  unsigned long total_workspace = get_total_workspace(display, root);

  for (int i = 0; i <= total_workspace; i++) {
    Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", True);
    unsigned long total_win_items = 0;

    Window *curr_win_open =
        fetch_window_list(display, root, &total_win_items, atom, i);

    if (total_win_items >= WIN_APP_LIMIT) {
      LOG(WINMGM_INFO, "Reach win limit\n");
      int window_left = total_win_items - WIN_APP_LIMIT;

      while (1) {
        (*base_workspace_num)++;
        curr_win_open = fetch_window_list(display, root, &total_win_items, atom,
                                          *base_workspace_num);
        if (total_win_items < WIN_APP_LIMIT)
          break;
      }

      for (int i = 0; i < window_left; i++) {
        Window last_win_open = get_last_opened_window(display);
        unmaximize_window(display, last_win_open);
        move_window_to_workspace(display, last_win_open, *base_workspace_num);
        arrange_window(total_win_items, curr_win_open, display, screen);
      }
    }
  }
}

static void arrange_dimension(Display *display, Window root,
                              unsigned long base_win_items,
                              Window *curr_win_open, win_info *base_win_info,
                              int screen) {
  for (unsigned long int i = 0; i < base_win_items; i++) {
    win_info *curr_win_info =
        (win_info *)malloc(base_win_items * sizeof(win_info));
    get_window_dimensions(display, curr_win_open[i], &curr_win_info[i].width,
                          &curr_win_info[i].height);

    if (curr_win_info[i].width != base_win_info[i].width ||
        curr_win_info[i].height != base_win_info[i].height) {
      unmaximize_window(display, curr_win_open[i]);
      arrange_window(base_win_items, curr_win_open, display, screen);

      base_win_info[i].width = curr_win_info[i].width;
      base_win_info[i].height = curr_win_info[i].height;
    }

    free(curr_win_info);
  }
}

static void manage_window(Display *display, Window root,
                          unsigned long *base_win_items,
                          win_info *base_win_info, int screen) {
  unsigned long curr_win_items = 0;
  Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", True);
  Window *curr_win_open =
      fetch_window_list(display, root, &curr_win_items, atom,
                        get_current_workspace(display, root));

  if (curr_win_items != *base_win_items) {
    LOG(WINMGM_INFO, "Re-arrange current window items...\n");

    *base_win_items = curr_win_items;
    for (unsigned long int i = 0; i < *base_win_items; i++) {
      if (curr_win_open[i] != 0)
        unmaximize_window(display, curr_win_open[i]);
    }

    if (curr_win_open)
      arrange_window(*base_win_items, curr_win_open, display, screen);
  }

  arrange_dimension(display, root, *base_win_items, curr_win_open,
                    base_win_info, screen);

  if (curr_win_open)
    free(curr_win_open);
}

void run_x11_layout() {
  Display *display = initialize_display();
  if (!display)
    return;

  int screen = DefaultScreen(display);
  Window root = get_root_window(display);

  unsigned long base_win_items = 0;
  win_info *base_win_info = NULL;

  // Arrange the first window init
  Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", True);
  int base_workspace_num = get_current_workspace(display, root);
  Window *windows = fetch_window_list(display, root, &base_win_items, atom,
                                      base_workspace_num);
  if (windows) {
    base_win_info = (win_info *)malloc(base_win_items * sizeof(win_info));
    if (base_win_info == NULL) {
      LOG(WINMGM_WARN, " Memory allocation failed.\n");
      free(windows);
      XCloseDisplay(display);
      return;
    }

    for (unsigned long int i = 0; i < base_win_items; i++) {
      unmaximize_window(display, windows[i]);
      arrange_window(base_win_items, windows, display, screen);
    }

    XFree(windows); // Free the windows list after use
  }

  unsigned long curr_win_items = 0;
  XSetErrorHandler(error_handler);

  while (1) {
    unsigned long int total_workspace = get_total_workspace(display, root);

    manage_workspace_limit(display, root, screen, &base_workspace_num);
    manage_window(display, root, &base_win_items, base_win_info, screen);

    usleep(20000);
  }

  XCloseDisplay(display);
}
