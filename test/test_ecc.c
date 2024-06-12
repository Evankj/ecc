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

void TestCreateBucket() {

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

  printf("TestCreateBucket        PASSED\n");
}

void TestDeleteEntityFromBucket() {
  int expectedEntityCount = 0;
  int entityCount = -1;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);

  BucketCreateEntity(bucket);

  BucketDeleteEntity(bucket, 0);

  entityCount = bucket->entityCount;

  ArenaDestroy(testArena);

  ASSERT(entityCount == expectedEntityCount);

  printf("TestDeleteEntityFromBucket        PASSED\n");
}

void TestAssignComponentTypeToEntity() {
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
      BucketRegisterComponentType(bucket, sizeof(Position), "Position");

  componentTypeCount = bucket->componentIdTop;

  Entity *entity = BucketCreateEntity(bucket);


  Position *pos = AddComponentToEntityById(bucket, entity->index, posComponentType);
  pos->x = expectedX;
  pos->y = expectedY;
  Position *entityPos =
      GetComponentForEntityById(bucket, entity->index, posComponentType);
  if (entityPos != NULL) {
    x = entityPos->x;
    y = entityPos->y;
  }

  BucketCreateEntity(bucket);

  ArenaDestroy(testArena);

  ASSERT(componentTypeCount == expectedComponentTypeCount);
  ASSERT(x == expectedX && y == expectedY);

  printf("TestAssignComponentTypeToEntity        PASSED\n");
}

void TestAssignComponentTypeToEntityWithMacro() {
  int expectedComponentTypeCount = 1;
  int componentTypeCount = 0;

  float expectedX = 1.0;
  float expectedY = 2.0;

  float x, y;

  typedef struct {
    float x;
    float y;
  } Position;

  Arena *testArena = ArenaCreate(TEST_ARENA_SIZE);

  Bucket *bucket = BucketCreate(testArena, 10);


  Entity *entity = BucketCreateEntity(bucket);

  Position *pos = ADD_COMPONENT_TO_ENTITY(bucket, entity, Position);
  pos->x = expectedX;
  pos->y = expectedY;

  componentTypeCount = bucket->componentIdTop;

  Position *entityPos = GET_COMPONENT_FROM_ENTITY(bucket, entity, Position);
  if (entityPos != NULL) {
    x = entityPos->x;
    y = entityPos->y;
  }

  ArenaDestroy(testArena);

  ASSERT(componentTypeCount == expectedComponentTypeCount);
  ASSERT(x == expectedX && y == expectedY);

  printf("TestAssignComponentTypeToEntityWithMacro        PASSED\n");
}

void TestRemoveComponentTypeFromEntity() {
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
      BucketRegisterComponentType(bucket, sizeof(Position), "Position");

  componentTypeCount = bucket->componentIdTop;

  Entity *entity = BucketCreateEntity(bucket);


  Position *pos = AddComponentToEntityById(bucket, entity->index, posComponentType);
  pos->x = expectedX;
  pos->y = expectedY;
  Position *entityPos =
      GetComponentForEntityById(bucket, entity->index, posComponentType);
  if (entityPos != NULL) {
    hasPosComponent = 1;
    entityMask = entity->mask;
  }

  RemoveComponentFromEntityById(bucket, entity->index, posComponentType);
  entityPos = GetComponentForEntityById(bucket, entity->index, posComponentType);
  if (entityPos == NULL) {
    hasPosComponent = 0;
    entityMask = entity->mask;
  }


  ArenaDestroy(testArena);

  ASSERT(hasPosComponent == 0);
  ASSERT(entityMask == 0);

  printf("TestRemoveComponentTypeFromEntity        PASSED\n");
}


void TestRemoveComponentTypeFromEntityWithMacro() {
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

  Entity *entity = BucketCreateEntity(bucket);

  Position *pos = ADD_COMPONENT_TO_ENTITY(bucket, entity, Position);
  pos->x = expectedX;
  pos->y = expectedY;

  componentTypeCount = bucket->componentIdTop;

  Position *entityPos = GET_COMPONENT_FROM_ENTITY(bucket, entity, Position);
      
  if (entityPos != NULL) {
    hasPosComponent = 1;
    entityMask = entity->mask;
  }
  
  REMOVE_COMPONENT_FROM_ENTITY(bucket, entity, Position);

  entityPos = GET_COMPONENT_FROM_ENTITY(bucket, entity, Position);

  if (entityPos == NULL) {
    hasPosComponent = 0;
    entityMask = entity->mask;
  }

  BucketCreateEntity(bucket);

  ArenaDestroy(testArena);

  ASSERT(hasPosComponent == 0);
  ASSERT(entityMask == 0);

  printf("TestRemoveComponentTypeFromEntityWithMacro        PASSED\n");
}

int main(void) {
  printf("Running tests for ecc.h\n");
  TestCreateBucket();
  TestDeleteEntityFromBucket();
  TestAssignComponentTypeToEntity();
  TestAssignComponentTypeToEntityWithMacro();
  TestRemoveComponentTypeFromEntity();
  TestRemoveComponentTypeFromEntityWithMacro();
  return 0;
}
