#include "entity/entityPool.h"
#include <stdlib.h>
// https://ajmmertens.medium.com/doing-a-lot-with-a-little-ecs-identifiers-25a72bd2647
// https://doc.lagout.org/security/Hackers%20Delight.pdf
// https://stackoverflow.com/questions/69023477/what-are-some-good-resources-for-learning-bit-shifting
// https://graphics.stanford.edu/%7Eseander/bithacks.html

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  memory_arena arena = {0};
  size_t arenaSize = 1024 * 100;
  uint8_t *base = (uint8_t *)malloc(arenaSize);
  arena_init(&arena, base, arenaSize);

  EntityPool *ePool = entityPool_InitInArena(&arena, 1000);

  entityPool_clear(ePool);

  for (size_t i = 0; i < 50; i++) {
    entityPool_push(ePool,
                    (Entity){.identifier = entity_id_create(0, 0, 0, 0)});
  }

  entityPool_remove(ePool, 25);

  // size_t newid = _entityPool_GetNextId(ePool);
  en_identifier test = entity_id_create(0xFFFF, 1, 2, 1);
  char buf[128];
  NUM_TO_BINARY_STR(buf, 128, test);
  printf("id  :      %s\n", buf);
  printf("hello world\n");
  // _increment_number_at_offset(test, 0, en_mask_id);

  en_set_identifier_id(&test, 1);
  NUM_TO_BINARY_STR(buf, 128, test);
  printf("id  :      %s\n", buf);

  return 0;
}
