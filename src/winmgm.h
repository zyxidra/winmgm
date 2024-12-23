#include <stdio.h>

#define WINMGM_ERR 1
#define WINMGM_WARN 2
#define WINMGM_INFO 3
#define WINMGM_DBG 4

// Current log level (change this to set the level)
#define CURRENT_LOG_LEVEL WINMGM_DBG

#define LOG(level, fmt, ...)                                                   \
  do                                                                           \
  {                                                                            \
    if (level <= CURRENT_LOG_LEVEL)                                            \
    {                                                                          \
      printf ("[%s] " fmt, #level, ##__VA_ARGS__);                             \
    }                                                                          \
  } while (0)
