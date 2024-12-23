#include "X11_session.h"

int
main ()
{
#ifdef __linux
  char *session_type = getenv ("XDG_SESSION_TYPE");
  if (session_type != NULL)
  {
    if (strcmp (session_type, "x11") == 0)
    {
      run_x11_layout ();
    }
    else
    {
      printf ("The session %s type is not supported.\n", session_type);
    }
  }
  else
  {
    printf ("The session is not set.\n");
  }
#endif
  return 0;
}
