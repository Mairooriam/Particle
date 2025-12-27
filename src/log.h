#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static char log_buffer[4096] = {0};
static char log_prefix[256] = {0};
static bool log_overflowed = false;

#define LOG(fmt, ...)                                                          \
  do {                                                                         \
    if (!log_overflowed) {                                                     \
      size_t before_len = strlen(log_buffer);                                  \
      int needed =                                                             \
          snprintf(NULL, 0, "%s" fmt "\n", log_prefix, ##__VA_ARGS__);         \
      if (before_len + needed >= sizeof(log_buffer)) {                         \
        strcpy_s(log_buffer, sizeof(log_buffer), "OVERFLOW");                 \
        log_overflowed = true;                                                 \
      } else {                                                                 \
        snprintf(log_buffer + before_len, sizeof(log_buffer) - before_len,     \
                 "%s" fmt "\n", log_prefix, ##__VA_ARGS__);                    \
      }                                                                        \
    }                                                                          \
  } while (0)

static void set_log_prefix(const char *prefix) {
  strncpy_s(log_prefix, sizeof(log_prefix), prefix, sizeof(log_prefix) - 1);
  log_prefix[sizeof(log_prefix) - 1] = '\0';
}

static void flush_logs() {
  if (log_buffer[0]) {
    printf("%s", log_buffer);
    log_buffer[0] = '\0';
    log_overflowed = false;
  }
}