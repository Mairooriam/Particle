#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void strrev_impl(char *head) {
  if (!head)
    return;
  char *tail = head;
  while (*tail)
    ++tail; // find the 0 terminator, like head+strlen
  --tail;   // tail points to the last real char
            // head still points to the first
  for (; head < tail; ++head, --tail) {
    // walk pointers inwards until they meet or cross in the middle
    char h = *head, t = *tail;
    *head = t; // swapping as we go
    *tail = h;
  }
}

// straight from AI
#define NUM_TO_BINARY_STR(buf, size, num)                                      \
  do {                                                                         \
    size_t pos = 0;                                                            \
    size_t total_bits = sizeof(num) * 8;                                       \
    for (size_t i = 0; i < total_bits; i++) {                                  \
      size_t shift_amount = total_bits - 1 - i;                                \
      uint64_t mask = (shift_amount < 64) ? (1ULL << shift_amount) : 0;        \
      uint64_t masked = ((uint64_t)(num) & mask);                              \
      char c = masked ? '1' : '0';                                             \
      snprintf((buf) + pos, (size) - pos, "%c", c);                            \
      pos++;                                                                   \
      if ((i + 1) % 8 == 0 && (i + 1) < total_bits) {                          \
        snprintf((buf) + pos, (size) - pos, " ");                              \
        pos++;                                                                 \
      }                                                                        \
    }                                                                          \
    (buf)[pos] = '\0';                                                         \
  } while (0)

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  // int v = -10;
  // int sign = -(v < 0);
  // printf("Value: %i ,sign: %i\n", v, sign);
  // v = 10;
  // sign = -(v < 0);
  // printf("Value: %i ,sign: %i\n", v, sign);
  //
  // v = -10;
  // sign = v >> (sizeof(int) * 8 - 1);
  // printf("Value: %i ,sign: %i\n", v, sign);

  // 100000001
  // ^ is it - or +
  // 10000001
  // int val = 1;
  // size_t counter = 1;
  // for (int mask = 1; mask < (1 << 30); mask = mask << 1) {
  //   printf("c:%lli %i shifted left %i  --  ", counter, val, (val <<
  //   counter)); counter++; printf("Mask: %i --- ", mask); printf("BITWISE AND
  //   on v:%i\n", (v & mask));
  // }

  // v = -10;
  // char buf[64];
  // memset(buf, 0, 32);
  // NUM_TO_BINARY_STR(buf, 64, v);
  // printf("%s\n", buf);

  // ENTITY: 1 -> entity 2:
  // delete entiy 2
  // entity 2: <-

  //   255|8                           65k                 16 million
  // |00000000|00000000 00000000|00000000 00000000|00000000 00000000 00000000|
  // |  flags |    spare        |   generation    |         ID               |
  char buf[128];
  size_t mask_id = 0xFFFFFF;       // 24 bits for raw id -> 16 million entities;
  size_t mask_generation = 0xFFFF; // 16 bits for generation
  size_t mask_generation_offset = 24;
  size_t mask_flags = 0xFF;
  size_t mask_flags_offset = 56;

  uint64_t entity = 0xFFFFFFFFFFFFFFFF;

  // Debug: print the actual hex values
  printf("mask_id hex: 0x%zx\n", mask_id);
  printf("id hex:      0x%llx\n", entity);
  printf("sizeof(mask_id): %zu bits\n", sizeof(mask_id) * 8);
  printf("sizeof(id):      %zu bits\n", sizeof(entity) * 8);

  NUM_TO_BINARY_STR(buf, 128, mask_id);
  printf("id_mask: %s\n", buf);
  NUM_TO_BINARY_STR(buf, 128, mask_generation);
  printf("ge_mask: %s\n", buf);
  size_t mask_gen_with_offset = mask_generation << mask_generation_offset;
  NUM_TO_BINARY_STR(buf, 128, mask_gen_with_offset);
  printf("ge_moff: %s\n", buf);
  NUM_TO_BINARY_STR(buf, 128, mask_flags);
  printf("fl_mask: %s\n", buf);
  size_t mask_flags_with_offset = mask_flags << mask_flags_offset;
  NUM_TO_BINARY_STR(buf, 128, mask_flags_with_offset);
  printf("fla_off: %s\n", buf);

  NUM_TO_BINARY_STR(buf, 128, entity);
  printf("id:      %s\n", buf);

  // size_t flag = 0xFFFFFFFFFFFFFFFF;
  size_t flag = 1ULL;
  NUM_TO_BINARY_STR(buf, 128, flag);
  printf("\nflag:      %s\n", buf);
  flag = (1ULL << 1);
  NUM_TO_BINARY_STR(buf, 128, flag);
  printf("\nflag:      %s\n", buf);

  // printf("sizeof(int):%zu  *  8  -  1\n", sizeof(int));

  return 0;
}
