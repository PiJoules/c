#ifndef TREE_MAP_H_
#define TREE_MAP_H_

#include <stdlib.h>

typedef int (*TreeMapCmp)(const void*, const void*);
typedef const void* (*KeyCtor)(const void*);
typedef void (*KeyDtor)(const void*);

// The tree map does not own any of its values. The map does own its keys and
// is in charge of destroying them via the `key_dtor` function.
struct TreeMapNode {
  const void* key;
  void* value;
  struct TreeMapNode* left;
  struct TreeMapNode* right;
  TreeMapCmp cmp;
  KeyCtor key_ctor;
  KeyDtor key_dtor;
};

typedef struct TreeMapNode TreeMap;

void tree_map_construct(TreeMap* map, TreeMapCmp cmp, KeyCtor ctor,
                        KeyDtor dtor);
void tree_map_destroy(TreeMap* map);

typedef void (*TreeMapCallback)(const void* key, void* value, void* arg);
void tree_map_iterate(TreeMap* map, TreeMapCallback cb, void* arg);

void tree_map_set(TreeMap* map, const void* key, void* val);
void tree_map_append(TreeMap* this, const TreeMap* other);

// Erase a node from this map with a given key if it exists. Does nothing if
// the key does not exist.
void tree_map_erase(TreeMap* map, const void* key);

// Get a value from the map corresponding to a key. If the value was found,
// return true and set the value in `val`. Otherwise, return false.
bool tree_map_get(const TreeMap* map, const void* key, void* val);

static inline bool tree_map_has(const TreeMap* map, const void* key) {
  return tree_map_get(map, key, NULL);
}

void tree_map_clear(TreeMap* map);
void tree_map_clone(TreeMap* dst, const TreeMap* src);

void string_tree_map_construct(TreeMap* map);

void RunTreeMapTests();

#endif  // TREE_MAP_H_
