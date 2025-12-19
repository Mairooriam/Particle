#pragma once
#include <stdint.h>

// TODO: expand on the iterator stuff
//  https://github.com/benhoyt/ht
typedef struct ht_entry {
  uint16_t key;
  void *value;
} ht_entry;

typedef struct ht {
  ht_entry *entries;
  size_t capacity;
  size_t count;
} ht;
typedef struct ht ht;
ht *ht_create(void);
void ht_destroy(ht *t);
void *ht_get(ht *t, uint16_t key);
bool ht_set(ht *t, uint16_t key, void *value);

void ht_make_key(uint8_t val1, uint8_t val2, uint16_t *key);

#define INITIAL_CAPACITY 16
typedef struct {
  uint16_t key;
  void *value;

  ht *_table;
  size_t index;

} hti;
