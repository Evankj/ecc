#include "ecc.h"
#include <stdio.h>

#define ASSERT(expr)                                                           \
  if (!(expr)) {                                                               \
    printf("Assertion failed: %s, file %s, line %d\n", #expr, __FILE__,        \
           __LINE__);                                                          \
    exit(1);                                                                   \
  }

const int KB = 1024;
const int MB = KB * 1024;

const int TEST_ARENA_SIZE = MB * 10; // 10mb

void testCreateBucket() {

  int expectedEntityCount = 3;
  int entityCount = -1;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);

  for (int i = 0; i < expectedEntityCount; i++) {
    BucketCreateEntity(bucket);
  }

  entityCount = bucket->entityCount;

  ArenaDestroy(testArena);

  ASSERT(entityCount == expectedEntityCount);

  printf("testCreateBucket        PASSED\n");
}

void testDeleteEntityFromBucket() {
  int expectedEntityCount = 0;
  int entityCount = -1;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);

  BucketCreateEntity(bucket);

  BucketDeleteEntity(bucket, 0);

  entityCount = bucket->entityCount;

  ArenaDestroy(testArena);

  ASSERT(entityCount == expectedEntityCount);

  printf("testDeleteEntityFromBucket        PASSED\n");
}

void testAssignComponentTypeToEntity() {
  int expectedComponentTypeCount = 1;
  int componentTypeCount = -1;

  float expectedX = 1.0;
  float expectedY = 2.0;

  float x, y;

  typedef struct {
    float x;
    float y;
  } Position;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);

  ComponentType *posComponentType =
      BucketRegisterComponentType(bucket, sizeof(Position));

  componentTypeCount = bucket->componentIdTop;

  size_t entity = BucketCreateEntity(bucket);

  Position *pos = ArenaAllocate(bucket->arena, sizeof(Position));
  pos->x = expectedX;
  pos->y = expectedY;

  AddComponentToEntityById(bucket, entity, posComponentType, pos);
  Position *entityPos =
      GetComponentForEntityById(bucket, entity, posComponentType);
  if (entityPos != NULL) {
    x = entityPos->x;
    y = entityPos->y;
  }

  BucketCreateEntity(bucket);

  ArenaDestroy(testArena);

  ASSERT(componentTypeCount == expectedComponentTypeCount);
  ASSERT(x == expectedX && y == expectedY);

  printf("testAssignComponentTypeToEntity        PASSED\n");
}

void testRemoveComponentTypeFromEntity() {
  int expectedComponentTypeCount = 1;
  int componentTypeCount = -1;

  float expectedX = 1.0;
  float expectedY = 2.0;


  int hasPosComponent = -1;
  size_t entityMask = 0;


  typedef struct {
    float x;
    float y;
  } Position;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);

  ComponentType *posComponentType =
      BucketRegisterComponentType(bucket, sizeof(Position));

  componentTypeCount = bucket->componentIdTop;

  size_t entity = BucketCreateEntity(bucket);

  Position *pos = ArenaAllocate(bucket->arena, sizeof(Position));
  pos->x = expectedX;
  pos->y = expectedY;

  AddComponentToEntityById(bucket, entity, posComponentType, pos);
  Position *entityPos =
      GetComponentForEntityById(bucket, entity, posComponentType);
  if (entityPos != NULL) {
    hasPosComponent = 1;
    entityMask = bucket->entities[entity]->mask;
  }

  RemoveComponentFromEntityById(bucket, entity, posComponentType);
  entityPos = GetComponentForEntityById(bucket, entity, posComponentType);
  if (entityPos == NULL) {
    hasPosComponent = 0;
    entityMask = bucket->entities[entity]->mask;
  }

  BucketCreateEntity(bucket);

  ArenaDestroy(testArena);

  ASSERT(hasPosComponent == 0);
  ASSERT(entityMask == 0);

  printf("testRemoveComponentTypeFromEntity        PASSED\n");
}

int main(void) {
  printf("Running tests for ecc.h\n");
  testCreateBucket();
  testDeleteEntityFromBucket();
  testAssignComponentTypeToEntity();
  testRemoveComponentTypeFromEntity();
  return 0;
}
