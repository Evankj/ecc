#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// AddComponentToEntity(Bucket *bucket, Entity *entity, size_t componentSize,
// char *componentName)
// assert(entity != NULL);
#define ADD_COMPONENT_TO_ENTITY(bucket, entity, ComponentType)                 \
  ({                                                                           \
    ComponentType *comp = AddComponentToEntity(                                \
        bucket, entity, sizeof(ComponentType), #ComponentType);                \
    comp; /* Return the pointer to the component */                            \
  })

#define GET_COMPONENT_FROM_ENTITY(bucket, entity, ComponentType)               \
  ({                                                                           \
    ComponentType *comp =                                                      \
        GetComponentForEntity(bucket, entity, #ComponentType);                 \
    comp;                                                                      \
  })

// Memory Arena utilities
// ---------------------------------------------------------------------------------------------------
typedef struct {
  char *data; // char* to allow pointer arithmetic
  size_t top;
  size_t capacity; // Could this be smaller?
} Arena;

// Need to be able to create, free, allocate to and pop from arenas

// Create
Arena *ArenaCreate(size_t size) {
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

void ArenaClear(Arena *arena) { arena->top = 0; }

// Free
void ArenaDestroy(Arena *arena) {
  free(arena->data);
  free(arena);
}

// Basically just store pointer to top, bump top up by size and assign data into
// pointer <-> top
void *ArenaAllocate(Arena *arena, size_t size) {

  if (arena->top + size > arena->capacity) {
    return NULL;
  }

  void *ptr = arena->data + arena->top;
  memset(ptr, 0, size);
  arena->top += size;

  return ptr;
}
// ---------------------------------------------------------------------------------------------------

// Linked List utilities
// ---------------------------------------------------------------------------------------------------

typedef struct LinkedListNode {
  void *value;
  struct LinkedListNode *next;
} LinkedListNode;

typedef struct LinkedList {
  LinkedListNode *head;
  size_t length;
  Arena *arena;
} LinkedList;

LinkedList *LinkedListCreate(Arena *arena) {
  LinkedList *linkedList =
      (LinkedList *)ArenaAllocate(arena, sizeof(LinkedList));
  if (!linkedList) {
    return NULL;
  }

  linkedList->head = NULL;
  linkedList->length = 0;
  linkedList->arena = arena;

  return linkedList;
}

LinkedListNode *LinkedListPush(LinkedList *list, void *value) {
  // allocate new node in list's arena
  LinkedListNode *node =
      (LinkedListNode *)ArenaAllocate(list->arena, sizeof(LinkedListNode));
  if (!node) {
    return NULL;
  }

  node->value = value;
  node->next = NULL;

  LinkedListNode *head = list->head;
  if (head) {
    head->next = node;
  }

  list->head = node;

  list->length++;

  return node;
}

void *LinkedListPop(LinkedList *list) {

  if (!list->head) {
    return NULL;
  }

  LinkedListNode *head = list->head;
  void *val = head->value;

  list->head = list->head->next;

  if (list->length > 0) {
    list->length--;
  }

  return val;
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
const size_t MAX_COMPONENT_TYPES = 63;
const size_t MAX_QUERIES = 1000;

const size_t MAX_ENTITIES = 10000;

typedef struct {
  BitMask mask; // The bitmask of this component type
  char *name; // The name of the component (generally provided via the macro) -
              // it needs to be allocated on the heap and cannot be changed

  size_t componentId; // The id of this component type in the bucket to which it
                      // is assigned - this is probably unnecessary

  size_t componentSize; // The size of an individual component of this type

  void *entries[MAX_ENTITIES]; // pointer to array of pointers to actual structs
                               // containing the information for this component
                               // on a given entity

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
  size_t maxEntities;
  size_t entityListEnd;
  LinkedList *queries;

  Entity *entities[MAX_ENTITIES]; // The array of entities to hold all added
                                  // entities (There can be NULL elements if
                                  // entities are deleted and their freed index
                                  // is not reused)
} Bucket;

Bucket *BucketCreate(Arena *arena, size_t maxEntities) {
  Bucket *bucket = (Bucket *)ArenaAllocate(arena, sizeof(Bucket));
  if (!bucket) {
    return NULL;
  }

  bucket->componentIdTop = 0;
  bucket->arena = arena;
  bucket->entityCount = 0;
  bucket->entityListEnd = 0;
  bucket->freeIndexes = LinkedListCreate(arena);
  bucket->maxEntities = maxEntities;
  bucket->queries = LinkedListCreate(arena);

  // allocate component arrays
  size_t pos = arena->top;

  for (size_t i = 0; i < MAX_COMPONENT_TYPES; i++) {
    ComponentType *component =
        (ComponentType *)ArenaAllocate(arena, sizeof(ComponentType));
    if (!component) {
      // something went wrong, free allocated component types from the arena and
      // exit
      arena->top = pos;
      return NULL;
    }
    component->mask = 0;
    component->componentId = 0;
    component->componentSize = 0;
    bucket->components[i] = component;
  }

  for (size_t i = 0; i < MAX_ENTITIES; i++) {
    Entity *entity = ArenaAllocate(arena, sizeof(Entity));
    if (!entity) {
      // something went wrong, free allocated component types and entitise from
      // the arena and exit
      arena->top = pos;
      return NULL;
    }
    entity->mask = 0;
    entity->index = i;
    bucket->entities[i] = entity;
  }

  return bucket;
}

size_t BucketCreateEntity(Bucket *bucket) {

  // size_t index;
  // size_t val = (size_t)LinkedListPop(bucket->freeIndexes);
  // if (!val) {
  //   index = bucket->entityListEnd++;
  // } else {
  //   index = val;
  // }

  size_t index = bucket->entityListEnd++;
  Entity *entity = bucket->entities[index];

  entity->mask = 0;
  entity->index = index;

  bucket->entityCount++;

  return index;
}

void BucketDeleteEntity(Bucket *bucket, size_t index) {
  if (index < 0 || index >= bucket->entityListEnd) {
    return;
  }

  Entity *entity = bucket->entities[index];
  if (entity) {
    entity->mask = 0;
  }

  bucket->entityCount--;

  // LinkedListPush(bucket->freeIndexes, &index);
  //
}

ComponentType *BucketRegisterComponentType(Bucket *bucket, size_t size,
                                           char *name) {

  size_t index = bucket->componentIdTop++;

  ComponentType *component = bucket->components[index];

  component->mask = 1 << index;
  component->componentSize = size;
  component->componentId = index; // this is probably unnecessary??
  component->name = name;

  for (int i = 0; i < MAX_ENTITIES; i++) {
    // Stores pointers to memory in an arena, not actual component values
    component->entries[i] = ArenaAllocate(bucket->arena, sizeof(void *));
  }

  return component;
}

// Get a component for an entity. This function will search for the component by
// its name . It is an O(n) operation to check for existence and return the
// component
void *GetComponentForEntity(Bucket *bucket, Entity *entity,
                            char *componentName) {
  if (!entity || entity->index < 0 || entity->index > MAX_ENTITIES) {
    return NULL;
  }

  for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
    ComponentType *checkComponentType = bucket->components[i];
    // we've reached the end of the list, return early as there's no component
    // with that name registered
    if (!checkComponentType) {
      return NULL;
    }

    if (strcmp(checkComponentType->name, componentName) == 0) {
      return checkComponentType->entries[entity->index];
    }
  }
  return NULL;
}

void *AddComponentToEntityById(Bucket *bucket, size_t entityId,
                               ComponentType *componentType) {

  if (entityId < 0 || entityId > bucket->entityListEnd) {
    return NULL;
  }

  Entity *entity = bucket->entities[entityId];
  if (entity == NULL) {
    return NULL;
  }

  entity->mask = entity->mask | componentType->mask;

  void *component = ArenaAllocate(bucket->arena, componentType->componentSize);
  componentType->entries[entity->index] = component;

  return component;
}

// Add a component to an entity. This function will also register the
// component to the bucket if it isn't already registered. It is an O(n)
// operation to check for existence and register the component
void *AddComponentToEntity(Bucket *bucket, Entity *entity, size_t componentSize,
                           char *componentName) {

  if (!entity || entity->index < 0 || entity->index > MAX_ENTITIES) {
    return NULL;
  }

  // iterate through component types. If we reach a NULL without finding a
  // componentType with a matching name, it means we need to register a new
  // one
  for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
    ComponentType *checkComponentType = bucket->components[i];
    // reached an empty component type, create a new component and break out of
    // the loop.
    if (checkComponentType->mask == 0) {

      ComponentType *componentType =
          BucketRegisterComponentType(bucket, componentSize, componentName);
      void *componentData =
          AddComponentToEntityById(bucket, entity->index, componentType);

      return componentData;
    }

    // reached a component whose name matches
    if (strcmp(checkComponentType->name, componentName) == 0) {
      void *componentData =
          AddComponentToEntityById(bucket, entity->index, checkComponentType);
      return componentData;
    }
  }

  // reached the end of the components list and didn't find a match or a space
  // to create a new component
  return NULL;
}

void RemoveComponentFromEntityById(Bucket *bucket, size_t entityId,
                                   ComponentType *componentType) {

  if (entityId < 0 || entityId > bucket->entityListEnd) {
    return;
  }

  Entity *entity = bucket->entities[entityId];
  entity->mask &= ~componentType->mask;

  componentType->entries[entity->index] =
      NULL; // probably don't actually have to do this as it's no longer in
            // the mask and this isn't saving any space
            // TODO: Come back to this
}

void *GetComponentForEntityById(Bucket *bucket, size_t entityId,
                                ComponentType *componentType) {
  if (entityId < 0 || entityId > bucket->entityListEnd) {
    return NULL;
  }

  Entity *entity = bucket->entities[entityId];

  if (!((componentType->mask & entity->mask) == componentType->mask)) {
    return NULL;
  }

  return componentType->entries[entityId];
}
