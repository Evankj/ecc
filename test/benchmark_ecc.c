#include "ecc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_ITERATIONS 100000
#define ARENA_SIZE 1024 * 1024 * 1024
#define COMPONENT_SIZE 16

const size_t BENCHMARK_ENTITIES = 1;

typedef struct {
  int data[4]; // Example component data structure
} ExampleComponent;

void RunBenchmark(int iterations) {

  // fprintf(stdout, "Creating arena...\n");
  Arena *arena = ArenaCreate(ARENA_SIZE);
  if (!arena) {
    fprintf(stderr, "Failed to create arena\n");
    return;
  }

  // fprintf(stdout, "Creating bucket...\n");
  // fprintf(stdout, "Remaining arena size: %d\n", arena->capacity -
  // arena->top);
  Bucket *bucket = BucketCreate(arena, BENCHMARK_ENTITIES);
  if (!bucket) {
    fprintf(stderr, "Failed to create bucket\n");
    return;
  }

  // fprintf(stdout, "Registering component...\n");
  // ComponentType *componentType =
  //     BucketRegisterComponentType(bucket, sizeof(ExampleComponent));
  // if (!componentType) {
  //   fprintf(stderr, "Failed to register component type\n");
  // }

  clock_t start = clock();

  // fprintf(stdout, "Beginning benchmark...\n");
  for (int i = 0; i < iterations; i++) {

    // fprintf(stdout, "Iteration %d\n", i);
    size_t entityId = BucketCreateEntity(bucket);

    Entity *entity = bucket->entities[entityId];

    // fprintf(stdout, "Creating component...\n");
    ExampleComponent *component = ADD_COMPONENT_TO_ENTITY(
        bucket, entity, ExampleComponent);

    component->data[0] = i;

    // fprintf(stdout, "Retrieving component from entity %d...\n", entityId);
    ExampleComponent *retrievedComponent = GET_COMPONENT_FROM_ENTITY(bucket, entity, ExampleComponent);

    if (!retrievedComponent ||
        retrievedComponent->data[0] != component->data[0]) {
      fprintf(stderr, "Component data mismatch\n");
    }

    // fprintf(stdout, "Removing entity...\n");
    BucketDeleteEntity(bucket, entityId);
    bucket->entityListEnd = 0;
  }

  clock_t end = clock();
  double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

  printf("Benchmark completed in %.4f seconds\n", elapsed);

  ArenaDestroy(arena);
}

int main(int argc, char **argv) {
  int iterations = DEFAULT_ITERATIONS;

  if (argc > 1) {
    iterations = atoi(argv[1]);
  }

  RunBenchmark(iterations);
  return 0;
}
