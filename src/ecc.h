#include <cstddef>
#include <cstdlib>

// Memory Arena utilities
// ---------------------------------------------------------------------------------------------------
typedef struct {
  char *data; // char* to allow pointer arithmetic
  size_t top;
  size_t capacity; // Could this be smaller?
} Arena;

// Need to be able to create, free, allocate to and pop from arenas

// Create
inline Arena *ArenaCreate(size_t size) {
  Arena *arena = (Arena *)malloc(sizeof(Arena));
  if (!arena) {
    return NULL;
  }

  arena->data = (char *)malloc(size);

  if (!arena->data) {
    free(arena);
    return NULL;
  }

  arena->capacity = size;
  arena->top = 0;

  return arena;
}

// Free
inline void ArenaDestroy(Arena *arena) {
  free(arena->data);
  free(arena);
}

// Basically just store pointer to top, bump top up by size and assign data into
// pointer <-> top
inline void *ArenaAllocate(Arena *arena, size_t size) {
  if (arena->top + size > arena->capacity) {
    return NULL;
  }

  void *ptr = arena->data + arena->top;
  arena->top += size;

  return ptr;
}
// ---------------------------------------------------------------------------------------------------

// Linked List utilities
// ---------------------------------------------------------------------------------------------------

typedef struct LinkedListNode {
  void *value;
  LinkedListNode *next;
} LinkedListNode;

typedef struct LinkedList {
  LinkedListNode *head;
  size_t length;
  Arena *arena;
} LinkedList;

inline LinkedList *LinkedListCreate(Arena *arena) {
  LinkedList *linkedList =
      (LinkedList *)ArenaAllocate(arena, sizeof(LinkedList));
  if (!linkedList) {
    return NULL;
  }

  *linkedList = (LinkedList){
      .head = NULL,
      .length = 0,
      .arena = arena,
  };

  return linkedList;
}

inline LinkedListNode *LinkedListPush(LinkedList *list, void *value) {
  // allocate new node in list's arena
  LinkedListNode *node =
      (LinkedListNode *)ArenaAllocate(list->arena, sizeof(LinkedListNode));
  if (!node) {
    return NULL;
  }

  *node = {
      .value = value,
      .next = NULL,
  };

  LinkedListNode *head = list->head;
  if (head) {
    head->next = node;
  }

  list->head = node;

  list->length++;

  return node;
}

inline LinkedListNode *LinkedListPop(LinkedList *list) {

  LinkedListNode *head = list->head;
  if (!head) {
    return NULL;
  }
  list->head = head->next;

  list->length--;

  return head;
}


// ---------------------------------------------------------------------------------------------------

// Entities are just indexes into arrays and a bitmask signifying which
// components they hold
//
// Components are just structs with pointers stored in an array for each
// component type (these are all of the same length and entities point to
// the same index in each of them)
//
// Queries are just bitmasks representing which entities to include based on
// the components associated with them
//
// Buckets hold all of the component arrays and entity indexes and the
// information needed to create bit masks for queries to be run on the bucket

// So I can change it later to support > 63 component types (0 is no components)
typedef size_t BitMask;
const size_t MAX_COMPONENT_TYPES = sizeof(BitMask);

typedef struct {
  BitMask mask; // The bitmask of this component type

  size_t componentId; // The id of this component type in the bucket to which it
                      // is assigned - this is probably unnecessary

  size_t componentSize; // The size of an individual component of this type

  void *entries[]; // array of pointers to actual structs containing the
                   // information for this component on a given entity

} ComponentType;

typedef struct {
  size_t index;
  BitMask mask;
} Entity;

typedef struct {
  size_t componentIdTop;
  Arena *arena;
  ComponentType *components[MAX_COMPONENT_TYPES];
  size_t entityCount;
  LinkedList *freeIndexes;
  size_t entityListEnd;
  Entity *entities[]; // The array of entities to hold all added entities (There
                      // can be NULL elements if entities are deleted and their
                      // freed index is not reused)
} Bucket;

typedef struct {
  Bucket *bucket;
  BitMask includeMask;
  BitMask excludeMask;
} Query;

inline Query *QueryCreate(Bucket *bucket) {
  Query *query = (Query *)ArenaAllocate(bucket->arena, sizeof(Query));
  *query = {
      .bucket = bucket,
      .includeMask = 0,
      .excludeMask = 0,
  };

  return query;
}

inline void QueryWithComponentType(Query *query, ComponentType *componentType) {
  query->includeMask &= componentType->mask;
}

inline void QueryWithoutComponentType(Query *query,
                                      ComponentType *componentType) {
  query->excludeMask &= componentType->mask;
}

inline Bucket *BucketCreate(Arena *arena, size_t maxEntities) {
  Bucket *bucket = (Bucket *)ArenaAllocate(arena, sizeof(Bucket));
  if (!bucket) {
    return NULL;
  }

  bucket->componentIdTop = 0;
  bucket->arena = arena;
  bucket->entityCount = 0;
  bucket->entityListEnd = 0;
  bucket->freeIndexes = LinkedListCreate(arena);

  // allocate component arrays
  size_t pos = arena->top;

  for (size_t i = 0; i < MAX_COMPONENT_TYPES; ++i) {
    bucket->components[i] =
        (ComponentType *)ArenaAllocate(arena, sizeof(ComponentType));
    if (!bucket->components[i]) {
      // something went wrong, free allocated component types from the arena and
      // exit
      arena->top = pos;
      return NULL;
    }
  }
  for (size_t i = 0; i < maxEntities; ++i) {
    bucket->entities[i] = (Entity *)ArenaAllocate(arena, sizeof(Entity));
    if (!bucket->entities[i]) {
      // something went wrong, free allocated component types and entitise from
      // the arena and exit
      arena->top = pos;
      return NULL;
    }
  }

  return bucket;
}

inline size_t BucketCreateEntity(Bucket *bucket) {
  LinkedListNode *node = LinkedListPop(bucket->freeIndexes);
  size_t *val = (size_t *)node->value;
  size_t index = -1;
  if (!val) {
    // no free index to use, go from bucket end pointer
    index = bucket->entityListEnd++;
  } else {
    // free index found, use that instead
    index = *val;
  }
  bucket->entityCount++;

  return index;
}

inline void BucketDeleteEntity(Bucket *bucket, size_t index) {
  if (index >= bucket->entityListEnd) {
    return;
  }

  Entity *entity = bucket->entities[index];
  if (entity) {
    entity->mask = 0;
  }

  LinkedListPush(bucket->freeIndexes, &index);
}

inline ComponentType *BucketRegisterComponentType(Bucket *bucket, size_t size) {
  size_t index = bucket->componentIdTop++;

  ComponentType *component = bucket->components[index];

  component->mask = 1 << index;
  component->componentSize = size;
  component->componentId = index; // this is probably unnecessary??

  return component;
}

inline void AddComponentToEntityById(Bucket *bucket, Entity *entity,
                                     ComponentType *componentType,
                                     void *component) {
  entity->mask &= componentType->mask;
  componentType->entries[entity->index] = component;
}

inline void RemoveComponentFromEntityById(Bucket *bucket, Entity *entity,
                                          ComponentType *componentType) {
  entity->mask &= ~componentType->mask;

  componentType->entries[entity->index] =
      NULL; // probably don't actually have to do this as it's no longer in the
            // mask and this isn't saving any space
            // TODO: Come back to this
}
