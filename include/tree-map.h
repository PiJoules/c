#ifndef TREE_MAP_H_
#define TREE_MAP_H_

typedef int (*TreeMapCmp)(const void*, const void*);
typedef const void* (*KeyCtor)(const void*);
typedef void (*KeyDtor)(const void*);

// The tree map does not own any of its values or keys. It's only in charge
// of destroying nodes it creates.
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
bool tree_map_get(const TreeMap* map, const void* key, void* val);
void tree_map_clear(TreeMap* map);
void tree_map_clone(TreeMap* dst, const TreeMap* src);

void string_tree_map_construct(TreeMap* map);

void RunTreeMapTests();

#endif  // TREE_MAP_H_
