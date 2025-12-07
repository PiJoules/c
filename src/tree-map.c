#include "tree-map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void key_dtor_default(const void*) {}

static const void* key_ctor_default(const void* key) { return key; }

void tree_map_construct(TreeMap* map, TreeMapCmp cmp, KeyCtor ctor,
                        KeyDtor dtor) {
  map->left = NULL;
  map->right = NULL;
  map->key = NULL;
  map->value = NULL;
  map->cmp = cmp;
  map->key_ctor = ctor ? ctor : &key_ctor_default;
  map->key_dtor = dtor ? dtor : &key_dtor_default;
}

void tree_map_destroy(TreeMap* map) {
  if (map->left) {
    tree_map_destroy(map->left);
    free(map->left);
    map->left = NULL;
  }

  if (map->right) {
    tree_map_destroy(map->right);
    free(map->right);
    map->right = NULL;
  }

  map->key_dtor(map->key);
  map->key = NULL;
}

void tree_map_iterate(TreeMap* map, TreeMapCallback cb, void* arg) {
  if (map->left)
    tree_map_iterate(map->left, cb, arg);

  if (map->right)
    tree_map_iterate(map->right, cb, arg);

  if (map->key)
    cb(map->key, map->value, arg);
}

void tree_map_set(TreeMap* map, const void* key, void* val) {
  if (map->key == NULL) {
    map->key = map->key_ctor(key);
    map->value = val;
    return;
  }

  int cmp = map->cmp(map->key, key);
  if (cmp < 0) {
    if (map->left == NULL) {
      map->left = malloc(sizeof(TreeMap));
      assert(map->left);
      tree_map_construct(map->left, map->cmp, map->key_ctor, map->key_dtor);
    }
    tree_map_set(map->left, key, val);
  } else if (cmp > 0) {
    if (map->right == NULL) {
      map->right = malloc(sizeof(TreeMap));
      assert(map->right);
      tree_map_construct(map->right, map->cmp, map->key_ctor, map->key_dtor);
    }
    tree_map_set(map->right, key, val);
  } else {
    map->value = val;
  }
}

// Get a value from the map corresponding to a key. If the value was found,
// return true and set the value in `val`. Otherwise, return false.
bool tree_map_get(const TreeMap* map, const void* key, void* val) {
  if (map->key == NULL)
    return false;

  int cmp = map->cmp(map->key, key);
  if (cmp < 0) {
    if (map->left == NULL)
      return false;
    return tree_map_get(map->left, key, val);
  } else if (cmp > 0) {
    if (map->right == NULL)
      return false;
    return tree_map_get(map->right, key, val);
  } else {
    if (val)
      *(void**)val = map->value;
    return true;
  }
}

void tree_map_clear(TreeMap* map) {
  tree_map_destroy(map);
  tree_map_construct(map, map->cmp, map->key_ctor, map->key_dtor);
}

void tree_map_clone(TreeMap* dst, const TreeMap* src) {
  tree_map_clear(dst);

  if (!src->key)
    return;

  dst->key = src->key_ctor(src->key);
  dst->value = src->value;

  if (src->left) {
    dst->left = malloc(sizeof(TreeMap));
    tree_map_construct(dst->left, src->cmp, src->key_ctor, src->key_dtor);
    tree_map_clone(dst->left, src->left);
  } else {
    dst->left = NULL;
  }

  if (src->right) {
    dst->right = malloc(sizeof(TreeMap));
    tree_map_construct(dst->right, src->cmp, src->key_ctor, src->key_dtor);
    tree_map_clone(dst->right, src->right);
  } else {
    dst->right = NULL;
  }
}

static int string_tree_map_cmp(const void* lhs, const void* rhs) {
  return strcmp(lhs, rhs);
}

static void string_tree_map_key_dtor(const void* key) {
  if (key)
    free((void*)key);
}

static const void* string_tree_map_key_ctor(const void* key) {
  size_t len = strlen((const char*)key);
  char* cpy = malloc(sizeof(char) * (len + 1));
  memcpy(cpy, key, len);
  cpy[len] = 0;
  return cpy;
}

void string_tree_map_construct(TreeMap* map) {
  tree_map_construct(map, string_tree_map_cmp, string_tree_map_key_ctor,
                     string_tree_map_key_dtor);
}

///
/// Start Tree Map Tests
///

static void TestTreeMapConstruction() {
  TreeMap m;
  tree_map_construct(&m, NULL, NULL, NULL);

  assert(m.left == NULL);
  assert(m.right == NULL);
  assert(m.value == NULL);
  assert(m.key == NULL);

  tree_map_destroy(&m);
}

static void TestTreeMapInsertion() {
  TreeMap m;
  string_tree_map_construct(&m);

  void* res = NULL;
  assert(!tree_map_get(&m, "key", &res));

  const char* val = "val";
  tree_map_set(&m, "key", (char*)val);

  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  const char* val2 = "val2";
  tree_map_set(&m, "key2", (char*)val2);

  assert(tree_map_get(&m, "key2", &res));
  assert(res == val2);
  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  tree_map_destroy(&m);
}

static void TestTreeMapOverrideKeyValue() {
  TreeMap m;
  string_tree_map_construct(&m);

  const char* val = "val";
  tree_map_set(&m, "key", (char*)val);

  void* res = NULL;
  assert(tree_map_get(&m, "key", &res));
  assert(res == val);

  const char* newval = "newval";
  tree_map_set(&m, "key", (char*)newval);

  assert(tree_map_get(&m, "key", &res));
  assert(res == newval);

  tree_map_destroy(&m);
}

static void TestTreeMapClone() {
  TreeMap m;
  string_tree_map_construct(&m);

  const char* val = "val";
  tree_map_set(&m, "key", (char*)val);

  tree_map_clear(&m);
  assert(m.key == NULL);
  assert(m.left == NULL);
  assert(m.right == NULL);

  TreeMap m2;
  string_tree_map_construct(&m2);
  tree_map_set(&m2, "key", (char*)val);
  const char* val2 = "val2";
  tree_map_set(&m2, "key2", (char*)val2);

  tree_map_clone(&m, &m2);
  assert(strcmp(m.key, "key") == 0);
  assert(strcmp(m2.key, "key") == 0);

  void* res = NULL;
  assert(tree_map_get(&m, "key", &res));
  assert(strcmp(res, val) == 0);
  assert(tree_map_get(&m2, "key", &res));
  assert(strcmp(res, val) == 0);
  assert(tree_map_get(&m, "key2", &res));
  assert(strcmp(res, val2) == 0);
  assert(tree_map_get(&m2, "key2", &res));
  assert(strcmp(res, val2) == 0);

  tree_map_destroy(&m);
  tree_map_destroy(&m2);
}

static void TestTreeMapCloneEmpty() {
  TreeMap m;
  TreeMap m2;
  string_tree_map_construct(&m);
  string_tree_map_construct(&m2);

  tree_map_clone(&m, &m2);
  assert(m.key == NULL);
  assert(m2.key == NULL);

  tree_map_destroy(&m);
  tree_map_destroy(&m2);
}

void RunTreeMapTests() {
  TestTreeMapConstruction();
  TestTreeMapInsertion();
  TestTreeMapOverrideKeyValue();
  TestTreeMapClone();
  TestTreeMapCloneEmpty();
}

///
/// End Tree Map Tests
///
