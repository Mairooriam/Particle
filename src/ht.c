#include "ht.h"
#include <stdint.h>
#include <stdlib.h>

ht *ht_create(void) {
  ht *t = malloc(sizeof(ht));
  if (!t) {
    return NULL;
  }

  t->capacity = INITIAL_CAPACITY;
  t->count = 0;
  t->entries = calloc(t->capacity, sizeof(ht_entry));

  if (t->entries == NULL) {
    free(t);
    return NULL;
  }
  return t;
}

void ht_destroy(ht *t) {
  if (!t)
    return;

  free(t->entries);
  free(t);
}

void *ht_get(ht *t, uint16_t key) {
  if (!t)
    return NULL;

  size_t index = key % t->capacity;

  for (size_t i = 0; i < t->capacity; i++) {
    (void)i;
    ht_entry *entry = &t->entries[index];

    if (entry->key == 0 && entry->value == NULL) {
      return NULL;
    }

    if (entry->key == key) {
      return entry->value;
    }

    index = (index + 1) % t->capacity;
  }

  return NULL;
}
static bool ht_set_entry(ht_entry *entries, size_t capacity, uint16_t key,
                         void *value, size_t *plength) {
  size_t index = key % capacity;

  for (size_t i = 0; i < capacity; i++) {
    ht_entry *entry = &entries[index];

    if (entry->key == key) {
      entry->value = value;
      return true;
    }

    if (entry->key == 0) {
      entry->key = key;
      entry->value = value;
      if (plength)
        (*plength)++;
      return true;
    }

    index = (index + 1) % capacity;
  }

  return false;
}

static bool ht_expand(ht *t) {
  size_t new_capacity = t->capacity * 2;
  if (new_capacity < t->capacity) {
    return false; // Overflow
  }

  ht_entry *new_entries = calloc(new_capacity, sizeof(ht_entry));
  if (!new_entries) {
    return false; // Failed allocating
  }

  for (size_t i = 0; i < t->capacity; i++) {
    ht_entry entry = t->entries[i];
    if (entry.key != 0) {
      ht_set_entry(new_entries, new_capacity, entry.key, entry.value, NULL);
    }
  }

  free(t->entries);
  t->entries = new_entries;
  t->capacity = new_capacity;
  return true;
}

bool ht_set(ht *t, uint16_t key, void *value) {
  if (!t)
    return false;

  if (ht_set_entry(t->entries, t->capacity, key, value, &t->count)) {
    return true;
  }

  if (!ht_expand(t))
    return false;
  return ht_set_entry(t->entries, t->capacity, key, value, &t->count);
}

void ht_make_key(uint8_t val1, uint8_t val2, uint16_t *key) {
  *key = ((uint16_t)val1 << 8) | val2;
}
