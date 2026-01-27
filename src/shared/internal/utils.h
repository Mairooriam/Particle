#pragma once
#include "stdlib.h"
#ifdef ENABLE_TIMING
typedef struct {
  const char *name;
  double total_ms;
  int count;
} TimingEntry;

extern TimingEntry g_timing_entries[32];
extern int g_timing_count;
extern double g_timing_accumulated_time;
extern double g_timing_print_interval; // Print every N seconds

#define TIME_IT(label, code)                                                   \
  do {                                                                         \
    double _start = GetTime();                                                 \
    code;                                                                      \
    double _elapsed = (GetTime() - _start) * 1000.0;                           \
    if (g_timing_count < 32) {                                                 \
      g_timing_entries[g_timing_count].name = label;                           \
      g_timing_entries[g_timing_count].total_ms = _elapsed;                    \
      g_timing_entries[g_timing_count].count = 1;                              \
      g_timing_count++;                                                        \
    }                                                                          \
  } while (0)

#define TIMING_FRAME_BEGIN()                                                   \
  do {                                                                         \
    g_timing_count = 0;                                                        \
  } while (0)

#define TIMING_FRAME_END(frametime)                                            \
  do {                                                                         \
    g_timing_accumulated_time += frametime;                                    \
    if (g_timing_accumulated_time >= g_timing_print_interval) {                \
      for (int i = 0; i < g_timing_count; i++) {                               \
        printf("%s: %.3f ms\n", g_timing_entries[i].name,                      \
               g_timing_entries[i].total_ms);                                  \
      }                                                                        \
      printf("--- (%.2fs elapsed)\n", g_timing_accumulated_time);              \
      g_timing_accumulated_time = 0.0;                                         \
    }                                                                          \
  } while (0)

#define TIMING_SET_INTERVAL(seconds)                                           \
  do {                                                                         \
    g_timing_print_interval = seconds;                                         \
  } while (0)
#else
#define TIME_IT(label, code)                                                   \
  do {                                                                         \
    code;                                                                      \
  } while (0)
#define TIMING_FRAME_BEGIN()
#define TIMING_FRAME_END(frametime)
#define TIMING_SET_INTERVAL(seconds)
#endif

#define FLAG_CLEAR_ALL(flags) ((flags) = FLAG_NONE)
