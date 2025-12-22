#include "tree-map.h"

#include <assert.h>
#include <stdio.h>
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
  if (map->key)
    cb(map->key, map->value, arg);

  if (map->left)
    tree_map_iterate(map->left, cb, arg);

  if (map->right)
    tree_map_iterate(map->right, cb, arg);
}

void tree_map_set(TreeMap* map, const void* key, void* val) {
  if (map->key == NULL) {
    assert(!map->left && !map->right);
    map->key = map->key_ctor(key);
    map->value = val;
    return;
  }

  int cmp = map->cmp(key, map->key);
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
  if (map->key == NULL) {
    assert(map->left == NULL && map->right == NULL);
    return false;
  }

  int cmp = map->cmp(key, map->key);
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

static void map_appender(const void* key, void* value, void* arg) {
  tree_map_set((TreeMap*)arg, key, value);
}

void tree_map_append(TreeMap* map, const TreeMap* other) {
  tree_map_iterate((TreeMap*)other, map_appender, map);
}

static void tree_map_erase_impl(TreeMap* map, const void* key,
                                TreeMap** parent_left_or_right_ptr) {
  assert(map->key);
  assert(!parent_left_or_right_ptr || *parent_left_or_right_ptr == map);

  int cmp = map->cmp(key, map->key);
  if (cmp < 0) {
    if (map->left)
      tree_map_erase_impl(map->left, key, &map->left);
  } else if (cmp > 0) {
    if (map->right)
      tree_map_erase_impl(map->right, key, &map->right);
  } else {
    map->key_dtor(map->key);
    TreeMap* left = map->left;
    TreeMap* right = map->right;

    if (!map->left && !map->right) {
      // If this has no children, then free this node. It could've only been
      // malloc'd since it's not the root node.
      if (parent_left_or_right_ptr == NULL) {
        map->key = NULL;
      } else {
        *parent_left_or_right_ptr = NULL;
        free(map);
      }
    } else if (map->left && !map->right) {
      // Just redirect the parent node pointer to the left tree.
      *map = *left;
      free(left);
    } else if (!map->left && map->right) {
      // Just redirect the parent node pointer to the right tree.
      *map = *right;
      free(right);
    } else {
      // We have two subtrees. Set the parent's pointer to the left one then
      // append the right.
      tree_map_append(left, right);
      tree_map_destroy(right);
      free(right);

      *map = *left;
      free(left);
    }
  }
}

void tree_map_erase(TreeMap* map, const void* key) {
  if (!map->key) {
    assert(!map->left && !map->right);
    return;
  }
  return tree_map_erase_impl(map, key, NULL);
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

static void TestTreeMapAppend() {
  TreeMap m;
  TreeMap m2;
  string_tree_map_construct(&m);
  string_tree_map_construct(&m2);

  int vals[] = {1, 2, 3, 4, 5, 6};
  tree_map_set(&m, "key1", &vals[0]);
  tree_map_set(&m, "key2", &vals[1]);
  tree_map_set(&m, "key3", &vals[2]);
  tree_map_set(&m2, "key3", &vals[3]);
  tree_map_set(&m2, "key4", &vals[4]);
  tree_map_set(&m2, "key5", &vals[5]);

  tree_map_append(&m, &m2);
  int val;
  assert(tree_map_get(&m, "key1", &val));
  assert(val == 1);
  assert(tree_map_get(&m, "key2", &val));
  assert(val == 2);
  assert(tree_map_get(&m, "key3", &val));
  assert(val == 4);
  assert(tree_map_get(&m, "key4", &val));
  assert(val == 5);
  assert(tree_map_get(&m, "key5", &val));
  assert(val == 6);

  tree_map_destroy(&m);
  tree_map_destroy(&m2);
}

static void AssertEmptyMap(const TreeMap* m) {
  assert(m->left == NULL);
  assert(m->right == NULL);
  assert(m->key == NULL);
}

static void AssertMapHasOnlyOneElem(const TreeMap* m) {
  assert(m->left == NULL);
  assert(m->right == NULL);
  assert(m->key != NULL);
}

static void TestTreeMapErase() {
  TreeMap m;

  int x[] = {1, 2, 3, 4};

  {
    string_tree_map_construct(&m);

    tree_map_set(&m, "key1", &x[0]);
    assert(tree_map_has(&m, "key1"));

    tree_map_erase(&m, "key1");
    assert(!tree_map_has(&m, "key1"));
    AssertEmptyMap(&m);

    tree_map_destroy(&m);
  }

  {
    string_tree_map_construct(&m);

    tree_map_set(&m, "key1", &x[0]);
    tree_map_set(&m, "key2", &x[1]);
    assert(tree_map_has(&m, "key1"));
    assert(tree_map_has(&m, "key2"));

    tree_map_erase(&m, "key1");
    assert(!tree_map_has(&m, "key1"));
    assert(tree_map_has(&m, "key2"));
    AssertMapHasOnlyOneElem(&m);
    assert(strcmp(m.key, "key2") == 0);

    tree_map_erase(&m, "key2");
    AssertEmptyMap(&m);

    tree_map_destroy(&m);
  }

  {
    string_tree_map_construct(&m);

    tree_map_set(&m, "key1", &x[0]);
    tree_map_set(&m, "key2", &x[1]);
    assert(tree_map_has(&m, "key1"));
    assert(tree_map_has(&m, "key2"));

    tree_map_erase(&m, "key2");
    assert(tree_map_has(&m, "key1"));
    AssertMapHasOnlyOneElem(&m);
    assert(!tree_map_has(&m, "key2"));
    assert(strcmp(m.key, "key1") == 0);

    tree_map_erase(&m, "key1");
    AssertEmptyMap(&m);

    tree_map_destroy(&m);
  }

  {
    string_tree_map_construct(&m);

    tree_map_set(&m, "key1", &x[0]);
    assert(m.left == NULL);
    tree_map_set(&m, "key2", &x[1]);
    assert(m.left == NULL);
    tree_map_set(&m, "key4", &x[3]);
    assert(m.left == NULL);

    assert(tree_map_has(&m, "key1"));
    assert(tree_map_has(&m, "key2"));
    assert(tree_map_has(&m, "key4"));

    assert(m.left == NULL);
    assert(strcmp(m.key, "key1") == 0);
    assert(m.right->left == NULL);
    assert(strcmp(m.right->key, "key2") == 0);
    AssertMapHasOnlyOneElem(m.right->right);
    assert(strcmp(m.right->right->key, "key4") == 0);

    tree_map_erase(&m, "key2");
    assert(m.left == NULL);
    assert(strcmp(m.key, "key1") == 0);
    AssertMapHasOnlyOneElem(m.right);
    assert(strcmp(m.right->key, "key4") == 0);

    assert(tree_map_has(&m, "key1"));
    assert(!tree_map_has(&m, "key2"));
    assert(tree_map_has(&m, "key4"));
    assert(m.left == NULL);
    assert(m.right != NULL);
    assert(strcmp(m.key, "key1") == 0);

    AssertMapHasOnlyOneElem(m.right);
    assert(strcmp(m.right->key, "key4") == 0);

    tree_map_erase(&m, "key1");
    assert(!tree_map_has(&m, "key1"));
    assert(!tree_map_has(&m, "key2"));
    assert(tree_map_has(&m, "key4"));
    AssertMapHasOnlyOneElem(&m);
    assert(strcmp(m.key, "key4") == 0);

    tree_map_erase(&m, "key4");
    AssertEmptyMap(&m);

    tree_map_destroy(&m);
  }

  string_tree_map_construct(&m);

  tree_map_set(&m, "key1", &x[0]);
  tree_map_set(&m, "key2", &x[1]);
  tree_map_set(&m, "key4", &x[3]);
  tree_map_set(&m, "key3", &x[2]);

  assert(strcmp(m.key, "key1") == 0);
  assert(strcmp(m.right->key, "key2") == 0);
  assert(strcmp(m.right->right->key, "key4") == 0);
  assert(strcmp(m.right->right->left->key, "key3") == 0);

  assert(tree_map_has(&m, "key1"));
  assert(tree_map_has(&m, "key2"));
  assert(tree_map_has(&m, "key3"));
  assert(tree_map_has(&m, "key4"));

  tree_map_erase(&m, "key2");
  assert(strcmp(m.key, "key1") == 0);
  assert(strcmp(m.right->key, "key4") == 0);
  assert(strcmp(m.right->left->key, "key3") == 0);

  assert(tree_map_has(&m, "key1"));
  assert(!tree_map_has(&m, "key2"));
  assert(tree_map_has(&m, "key3"));
  assert(tree_map_has(&m, "key4"));

  tree_map_erase(&m, "key1");
  assert(!tree_map_has(&m, "key1"));
  assert(!tree_map_has(&m, "key2"));
  assert(tree_map_has(&m, "key3"));
  assert(tree_map_has(&m, "key4"));

  tree_map_erase(&m, "key3");
  assert(!tree_map_has(&m, "key1"));
  assert(!tree_map_has(&m, "key2"));
  assert(!tree_map_has(&m, "key3"));
  assert(tree_map_has(&m, "key4"));

  tree_map_erase(&m, "key4");
  assert(!tree_map_has(&m, "key1"));
  assert(!tree_map_has(&m, "key2"));
  assert(!tree_map_has(&m, "key3"));
  assert(!tree_map_has(&m, "key4"));

  tree_map_destroy(&m);
}

void RunTreeMapTests() {
  TestTreeMapConstruction();
  TestTreeMapInsertion();
  TestTreeMapOverrideKeyValue();
  TestTreeMapClone();
  TestTreeMapCloneEmpty();
  TestTreeMapErase();
}

///
/// End Tree Map Tests
///
