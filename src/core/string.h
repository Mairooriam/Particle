#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef struct sv {
  char *data;
  size_t lenght;
} sv;

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string;

sv sv_create(const char *string);


//TODO: expand on the string functionality. for now just copy paste from old project
static void ConcatStrings(size_t sourceACount, char *sourceAstr,
                          size_t sourceBCount, char *sourceBstr,
                          size_t destCount, char *destStr) {
  (void)destCount;
  // TOOO: overflowing stuff
  for (size_t i = 0; i < sourceACount; i++) {
    *destStr++ = *sourceAstr++;
  }
  for (size_t i = 0; i < sourceBCount; i++) {
    *destStr++ = *sourceBstr++;
  }
  *destStr++ = 0;
}
#ifdef __cplusplus
}
#endif
