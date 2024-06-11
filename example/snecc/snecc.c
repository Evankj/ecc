#include "ecc.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>

const int KB = 1024;
const int MB = 1024 * 1024;
const size_t MAIN_ARENA_SIZE = MB * 100;
// const size_t MAX_ENTITIES = 1000;
const int ROWS = 16;
const int COLS = 32;

const size_t FRAME_ARENA_SIZE = MAIN_ARENA_SIZE;

const Color SNAKE_HEAD_COL = GREEN;
const Color SNAKE_BODY_COL = BLUE;
const Color APPLE_COL = RED;

const int GRID_SQUARE_SIZE = 32;

enum Direction { NONE, UP, DOWN, LEFT, RIGHT, QUIT };
typedef struct {
  enum Direction direction;
} Input;
typedef Vector2 Position;
typedef struct {
  Vector2 currentPos;
  Vector2 lastPos;
} GridPosition;
typedef Vector2 Direction;
typedef float Speed;
typedef Vector2 Scale;
typedef struct SnakeNode {
  struct SnakeNode *next;
  Entity *entity;
} SnakeNode;
typedef struct {
  float angle;
} Rotation;
typedef struct {
  Color color;
} Renderer;
typedef short SnakeHead;
typedef short Apple;
typedef short AppleEater;

enum GameMode { RUNNING, OVER };

typedef struct {
  Bucket *bucket;
  Arena *frameArena;
  SnakeNode *tailTip;
  Entity *snakeHead;
  Entity *apple;
  int screenWidth;
  int screenHeight;
  int score;
  enum GameMode gameMode;

} GameState;

// typedef void (*System)(GameState *gameState, Entity *entity);

Vector2 ToGridPos(Vector2 position, int gridSize) {

  float gridX = roundf(position.x / gridSize) * gridSize;
  float gridY = roundf(position.y / gridSize) * gridSize;

  return (Vector2){.x = gridX, .y = gridY};
}

void AddSnakeNode(GameState *gameState) {
  size_t snakeNodeId = BucketCreateEntity(gameState->bucket);
  Entity *snakeEntity = gameState->bucket->entities[snakeNodeId];

  SnakeNode *node =
      ADD_COMPONENT_TO_ENTITY(gameState->bucket, snakeEntity, SnakeNode);
  node->next = gameState->tailTip;
  node->entity = gameState->bucket->entities[snakeNodeId];

  GridPosition *tailPos = GET_COMPONENT_FROM_ENTITY(
      gameState->bucket, gameState->tailTip->entity, GridPosition);
  if (!tailPos) {
    // If we reach here something has gone seriously wrong...
    printf("NO TAIL POSITION\n");
    exit(1);
  }

  gameState->tailTip = node;

  GridPosition *nodeGridPos =
      ADD_COMPONENT_TO_ENTITY(gameState->bucket, snakeEntity, GridPosition);
  nodeGridPos->currentPos.x = tailPos->lastPos.x;
  nodeGridPos->currentPos.y = tailPos->lastPos.y;
  nodeGridPos->lastPos.x = 0;
  nodeGridPos->lastPos.y = 0;

  Scale *nodeScale =
      ADD_COMPONENT_TO_ENTITY(gameState->bucket, snakeEntity, Scale);
  nodeScale->x = GRID_SQUARE_SIZE;
  nodeScale->y = GRID_SQUARE_SIZE;

  Renderer *nodeRenderer =
      ADD_COMPONENT_TO_ENTITY(gameState->bucket, snakeEntity, Renderer);
  nodeRenderer->color = SNAKE_BODY_COL;
}

void AppleEaterSystem(GameState *gameState, Entity *entity) {
  if (!entity) {
    return;
  }

  GridPosition *gridPosition =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, GridPosition);
  SnakeNode *snakeNode =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, SnakeNode);
  SnakeHead *snakeHead =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, SnakeHead);

  // we only want the head node
  if (!snakeHead || !gridPosition || !snakeNode) {
    return;
  }

  GridPosition *applePosition = GET_COMPONENT_FROM_ENTITY(
      gameState->bucket, gameState->apple, GridPosition);

  if (!applePosition) {
    printf("NO APPLE FOUND!!!\n");
    return;
  }

  Vector2 diff =
      Vector2Subtract(gridPosition->currentPos, applePosition->currentPos);

  if (diff.x == 0 && diff.y == 0) {
    // Eat apple
    AddSnakeNode(gameState);
    applePosition->currentPos.x =
        GetRandomValue(0, (gameState->screenWidth / GRID_SQUARE_SIZE) - 1) *
        GRID_SQUARE_SIZE;
    applePosition->currentPos.y =
        GetRandomValue(0, (gameState->screenHeight / GRID_SQUARE_SIZE) - 1) *
        GRID_SQUARE_SIZE;
    gameState->score += 1;
  }
}

void SnakeTailMovementSystem(GameState *gameState) {

  SnakeNode *tailTip = gameState->tailTip;
  if (!tailTip) {
    return;
  }

  GridPosition *gridPosition = GET_COMPONENT_FROM_ENTITY(
      gameState->bucket, tailTip->entity, GridPosition);

  if (tailTip->entity == gameState->snakeHead) {
    GridPosition *moveToPosition = GET_COMPONENT_FROM_ENTITY(
        gameState->bucket, gameState->snakeHead, GridPosition);
    gridPosition->currentPos = moveToPosition->currentPos;
    return;
  }

  while (tailTip->next) {

    GridPosition *gridPosition = GET_COMPONENT_FROM_ENTITY(
        gameState->bucket, tailTip->entity, GridPosition);

    if (!gridPosition) {
      return;
    }

    GridPosition *moveToPosition = GET_COMPONENT_FROM_ENTITY(
        gameState->bucket, tailTip->next->entity, GridPosition);

    if (!moveToPosition) {
      return;
    }
    Vector2 previousPosition = gridPosition->currentPos;

    gridPosition->currentPos = moveToPosition->lastPos;

    if (Vector2Distance(gridPosition->currentPos, previousPosition) != 0) {
      gridPosition->lastPos = previousPosition;
    }
    tailTip = tailTip->next;
  }
}

void HeadCollisionSystem(GameState *gameState, Entity *entity) {
  if (!entity) {
    return;
  }

  if (entity == gameState->snakeHead) {
    return;
  }

  GridPosition *gridPosition =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, GridPosition);

  SnakeNode *node =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, SnakeNode);
  if (!gridPosition || !node) {
    return;
  }

  GridPosition *headPosition = GET_COMPONENT_FROM_ENTITY(
      gameState->bucket, gameState->snakeHead, GridPosition);

  if (!headPosition) {
    // Shouldn't happen, this is a bug
    printf(
        "CRITICAL ERROR: Tail node does not have a GridPosition component\n");
    exit(1);
  }

  int eatingTail =
      Vector2Distance(gridPosition->currentPos, headPosition->currentPos) == 0;

  int outOfBounds = (headPosition->currentPos.x > (gameState->screenWidth - GRID_SQUARE_SIZE)) ||
                    (headPosition->currentPos.x < 0) ||
                    (headPosition->currentPos.y > (gameState->screenHeight - GRID_SQUARE_SIZE)) ||
                    (headPosition->currentPos.y < 0);

  if (eatingTail || outOfBounds) {
    printf("ATE OUR OWN TAIL\n");
    gameState->gameMode = OVER;
  }
}

void HeadMovementSystem(GameState *gameState, Entity *entity, float dt) {
  if (!entity) {
    return;
  }

  Position *position =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Position);
  GridPosition *gridPosition =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, GridPosition);
  Direction *direction =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Direction);
  Speed *speed = GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Speed);

  if (!direction || !position || !gridPosition || !speed) {
    return;
  }

  *position = Vector2Add(*position, Vector2Scale(*direction, *speed * dt));

  Vector2 previousPosition = gridPosition->currentPos;
  gridPosition->currentPos = ToGridPos(*position, GRID_SQUARE_SIZE);
  if (Vector2Distance(gridPosition->currentPos, previousPosition) != 0) {
    gridPosition->lastPos = previousPosition;
  }
}

void SetDirectionSystem(GameState *gameState, Entity *entity) {
  if (!entity) {
    return;
  }
  Input *input = GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Input);
  Direction *direction =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Direction);

  if (!direction || !input) {
    return;
  }

  switch (input->direction) {
  case UP:
    direction->x = 0;
    direction->y = -1;
    break;
  case DOWN:
    direction->x = 0;
    direction->y = 1;
    break;
  case LEFT:
    direction->x = -1;
    direction->y = 0;
    break;
  case RIGHT:
    direction->x = 1;
    direction->y = 0;
    break;
  default:
    break;
  }
}

void InputSystem(GameState *gameState, Entity *entity) {

  if (!entity) {
    return;
  }

  Input *input = GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Input);

  if (!input) {
    // printf("NO INPUT COMPONENT FOR THIS ENTITY\n");
    return;
  }

  // printf("DIR: %d\n", input->direction);

  switch (GetCharPressed()) {
  case 'w':
    if (input->direction != DOWN) {
      input->direction = UP;
    }
    break;
  case 'a':
    if (input->direction != RIGHT) {
      input->direction = LEFT;
    }
    break;
  case 's':
    if (input->direction != UP) {
      input->direction = DOWN;
    }
    break;
  case 'd':
    if (input->direction != LEFT) {
      input->direction = RIGHT;
    }
    break;
  default:
    // input->direction = NONE;
    break;
  }
}

void RenderSystem(GameState *gameState, Entity *entity) {
  if (!entity) {
    return;
  }
  Renderer *renderer =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Renderer);
  GridPosition *gridPosition =
      GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, GridPosition);
  Scale *scale = GET_COMPONENT_FROM_ENTITY(gameState->bucket, entity, Scale);

  if (!renderer || !gridPosition || !scale) {
    return;
  }

  DrawRectangleRec(
      (Rectangle){
          .x = (float)gridPosition->currentPos.x,
          .y = (float)gridPosition->currentPos.y,
          .width = scale->x,
          .height = scale->y,
      },
      renderer->color);
}

// void ScoreDisplaySystem(GameState *gameState) {
//   DrawText(gameState->score, gameState->screenWidth / 2, 0, 22, GREEN);
// }

GameState *InitialiseGame() {
  Arena *arena = ArenaCreate(MAIN_ARENA_SIZE);
  Bucket *gameWorld = BucketCreate(arena, MAX_ENTITIES);

  GameState *gameState = ArenaAllocate(arena, sizeof(GameState));
  gameState->bucket = gameWorld;
  gameState->gameMode = RUNNING;

  // gameState->frameArena = ArenaCreate(FRAME_ARENA_SIZE);

  gameState->screenWidth = 800;
  gameState->screenHeight = 800;

  size_t snakeIndex = BucketCreateEntity(gameWorld);
  gameState->snakeHead = gameWorld->entities[snakeIndex];

  SnakeHead *snakeHead =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, SnakeHead);

  Position *snakePos =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Position);
  snakePos->x = (float)gameState->screenWidth / 2;
  snakePos->y = (float)gameState->screenHeight / 2;

  GridPosition *snakeGridPos =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, GridPosition);
  *snakeGridPos = (GridPosition){0};

  Direction *snakeDir =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Direction);
  snakeDir->x = 0;
  snakeDir->y = 0;

  Scale *snakeScale =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Scale);
  snakeScale->x = GRID_SQUARE_SIZE;
  snakeScale->y = GRID_SQUARE_SIZE;

  Renderer *snakeRenderer =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Renderer);
  snakeRenderer->color = SNAKE_HEAD_COL;

  AppleEater *appleEater =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, AppleEater);

  Speed *snakeSpeed =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Speed);
  *snakeSpeed = 250;

  Rotation *snakeRot =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Rotation);
  snakeRot->angle = 0;

  Input *input =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, Input);

  SnakeNode *snakeHeadNode =
      ADD_COMPONENT_TO_ENTITY(gameWorld, gameState->snakeHead, SnakeNode);
  snakeHeadNode->next = NULL;
  snakeHeadNode->entity = gameState->snakeHead;

  gameState->tailTip = snakeHeadNode;

  // Setup apple
  size_t appleIndex = BucketCreateEntity(gameWorld);
  Entity *apple = gameWorld->entities[appleIndex];

  Apple *gameApple = ADD_COMPONENT_TO_ENTITY(gameWorld, apple, Apple);

  GridPosition *appleGridPos =
      ADD_COMPONENT_TO_ENTITY(gameWorld, apple, GridPosition);
  appleGridPos->currentPos.x =
      GetRandomValue(0, gameState->screenWidth / GRID_SQUARE_SIZE) *
      GRID_SQUARE_SIZE;
  appleGridPos->currentPos.y =
      GetRandomValue(0, gameState->screenHeight / GRID_SQUARE_SIZE) *
      GRID_SQUARE_SIZE;
  appleGridPos->lastPos.x = 0;
  appleGridPos->lastPos.y = 0;

  Scale *appleScale = ADD_COMPONENT_TO_ENTITY(gameWorld, apple, Scale);
  appleScale->x = GRID_SQUARE_SIZE;
  appleScale->y = GRID_SQUARE_SIZE;

  Renderer *appleRenderer = ADD_COMPONENT_TO_ENTITY(gameWorld, apple, Renderer);
  appleRenderer->color = APPLE_COL;

  gameState->apple = apple;
  gameState->score = 0;

  return gameState;
}


void EndGame(Bucket *gameWorld) { ArenaDestroy(gameWorld->arena); }

int main(void) {

  int score = 0;

  GameState *gameState = InitialiseGame();

  printf("GAME STATE INITIALISED\n");

  InitWindow(gameState->screenWidth, gameState->screenHeight, "snecc example");

  SetTargetFPS(60);

  while (!WindowShouldClose() && gameState->gameMode == RUNNING) {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawFPS(0, 0);

    for (int i = 0; i < gameState->bucket->entityListEnd; i++) {
      Entity *entity = gameState->bucket->entities[i];
      if (!entity) {
        continue;
      }

      InputSystem(gameState, entity);

      SetDirectionSystem(gameState, entity);

      AppleEaterSystem(gameState, entity);

      SnakeTailMovementSystem(gameState);

      HeadMovementSystem(gameState, entity, GetFrameTime());

      HeadCollisionSystem(gameState, entity);

      RenderSystem(gameState, entity);
    }
    EndDrawing();
  }

  printf("Game over! Final Score: %d\n", gameState->score);
  EndGame(gameState->bucket);

  CloseWindow();
}
