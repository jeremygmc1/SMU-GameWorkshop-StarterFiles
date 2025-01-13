#include "include/raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raymath.h"
#include "utils.h"

// ::CONSTANTS
#define COLOR_PLAYER \
  CLITERAL(Color) { 0, 158, 47, 255 } // RAYLIB LIME
#define COLOR_BULLET \
  CLITERAL(Color) { 255, 0, 255, 0 } // Magenta
#define COLOR_ENEMY \
  CLITERAL(Color) { 190, 33, 55, 255 } // Maroon

#define PLAYER_RADIUS 16
#define PLAYER_SPEED 150
#define PLAYER_INIT_HEALTH 100

#define ENEMY_RADIUS 30
#define ENEMY_SPEED 50
#define ENEMY_INIT_HEALTH 100
#define NUM_ENEMIES 3

#define NUM_BULLETS 30
#define BULLET_LIFETIME 3.0
#define BULLET_SPEED 500
#define BULLET_RADIUS 4
#define BULLET_DAMAGE 10

// ::STRUCT DECLARATIONS

// Declares bullet parameters
typedef struct Bullet {
  Vector2 position;
  Vector2 unitDirection;
  float   timeLeft;
  bool    isValid;
} Bullet;

// Declares a frame for mouse input and its parameters
typedef struct InputFrame {
  bool isMouseDown;
} InputFrame;

// Declares the Enemy parameters
typedef struct Enemy {
  Vector2 position;
  bool    isValid;
  int     health;
} Enemy;

// Declares the Player parameters
typedef struct Player {
  Vector2 position;
  bool    isValid;
} Player;

typedef struct World {
  bool isPlayerDead; // Added new state
  bool isGameWon; // Added new state

  Vector2 cameraTarget;
  Player  player;
  Bullet  bullets[NUM_BULLETS]; 
  int     nextBulletIndex; 
  Enemy   enemies[NUM_ENEMIES];
} World;

// ::GLOBALS
World* world = 0;

// ::HELPER FUNCTIONS

void spawnNextBullet(Vector2 position, Vector2 unitDirection) {
  Bullet* newBullet = &world->bullets[world->nextBulletIndex];
  if (newBullet->isValid) printf("WARNING: REUSUING ALREADY EXISTING BULLET. Consider expanding bullet pool size\n");
  newBullet->isValid = true;
  newBullet->position = position;
  newBullet->unitDirection = unitDirection;
  newBullet->timeLeft = BULLET_LIFETIME;

  world->nextBulletIndex++;
  if (world->nextBulletIndex >= NUM_BULLETS) world->nextBulletIndex = 0;
}

void InitGame() {
  world = (World*)calloc(1, sizeof(World));

  // Spawns the Player
  world->player = (Player){
      .position = (Vector2){0, 0},
      .isValid = true
  };

  // Spawns the Enemies
  const float ENEMY_SPAWN_RADIUS = 300.0f;
  const float ANGLE_STEP = (2 * PI) / NUM_ENEMIES;
  for (int iEnemy = 0; iEnemy < NUM_ENEMIES; iEnemy++) {
    float   angle = iEnemy * ANGLE_STEP;
    Vector2 spawnPoint;
    spawnPoint.x = world->player.position.x + ENEMY_SPAWN_RADIUS * cos(angle);
    spawnPoint.y = world->player.position.y + ENEMY_SPAWN_RADIUS * sin(angle);

    world->enemies[iEnemy] = (Enemy){
        .position = spawnPoint,
        .isValid = true,
        .health = ENEMY_INIT_HEALTH};
  }
}

// ::MAIN
int main(void) {
  const int screenWidth = 800;
  const int screenHeight = 450;
  InitWindow(screenWidth, screenHeight, "DotHack Game Workshop");
  SetTargetFPS(0);
  SetExitKey(KEY_NULL);
  bool exitWindowRequested = false;
  bool exitWindow = false;

  // ::INITIALISATION
  Texture2D logo = LoadTexture("./resources/Raylib_logo.png");

  InitGame();

  Camera2D camera = {0};
  camera.target = world->cameraTarget;
  camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  camera.rotation = 0;
  camera.zoom = 1.0;

  // ::GAME LOOP
  while (!exitWindow) {
    float deltaTime = GetFrameTime();
    InputFrame inputFrame = (InputFrame){0};
    const Vector2 SCREEN_MOUSE_POS = GetMousePosition();
    const bool    IS_MOUSE_VALID = SCREEN_MOUSE_POS.x != 0 && SCREEN_MOUSE_POS.y != 0;
    const Vector2 WORLD_MOUSE_POS = GetScreenToWorld2D(SCREEN_MOUSE_POS, camera);

    { // ::LOOP INPUT

      if (exitWindowRequested) {
        if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER)) exitWindow = true;
        else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) exitWindowRequested = false;
      } else if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) exitWindowRequested = true;

      if (IsMouseButtonPressed(0)) {
        inputFrame.isMouseDown = true;
      }

      if (world->isPlayerDead || world->isGameWon) {
        if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_ENTER)) {
          memset(world, 0, sizeof(World));
          InitGame();
        }
      } else {
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
          world->player.position.x += PLAYER_SPEED * deltaTime;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
          world->player.position.x -= PLAYER_SPEED * deltaTime;
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
          world->player.position.y -= PLAYER_SPEED * deltaTime;
        if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
          world->player.position.y += PLAYER_SPEED * deltaTime;
      }
    }

    { // ::LOOP UPDATE LOGIC
      { // Camera Update
        Vector2 target_pos = world->player.position;
        animate_v2_to_target(&(camera.target), target_pos, deltaTime, 3.0f);
      }

      { // BULLET UPDATE
        if (inputFrame.isMouseDown) {
          inputFrame.isMouseDown = false; // NOTE: Consuming the button click
        
          Vector2 shotDirection = Vector2Subtract(WORLD_MOUSE_POS, world->player.position);
          shotDirection = Vector2Normalize(shotDirection);
          spawnNextBullet(world->player.position, shotDirection);
        }

        for (int iBullet = 0; iBullet < NUM_BULLETS; iBullet++) {
          Bullet* curBullet = &world->bullets[iBullet];
          if (!curBullet->isValid) continue;

          // PART 1: Update bullet lifetime
          curBullet->timeLeft -= deltaTime;
          if (curBullet->timeLeft <= 0.0f) {
            curBullet->isValid = false;
            continue;
          }

          // PART 2: Update bullet position
          curBullet->position = Vector2Add(curBullet->position, Vector2Scale(curBullet->unitDirection, deltaTime * BULLET_SPEED));

          // PART 3: Check collision with enemy			
          for (int iEnemy = 0; iEnemy < NUM_ENEMIES; iEnemy++) {
            Enemy* curEnemy = &world->enemies[iEnemy];
            if (!curEnemy->isValid) continue;

            if (CheckCollisionCircles(curEnemy->position, ENEMY_RADIUS, curBullet->position, BULLET_RADIUS)) {
              curBullet->isValid = false;
              curEnemy->health -= BULLET_DAMAGE;
              if (curEnemy->health <= 0) {
                curEnemy->isValid = false;
              }
              break;
            }
          }
        }
      }

      { // ENEMY UPDATE
        int numEnemyAlive = 0; 
        bool    touchedPlayer = false;
        Vector2 target_pos = world->player.position;
        for (int iEnemy = 0; iEnemy < NUM_ENEMIES; iEnemy++) {
          Enemy* curEnemy = &world->enemies[iEnemy];
          if (!curEnemy->isValid) continue;

          numEnemyAlive++; // This will effectively count how many alive enemies

          // NOTE: METHOD 1: "Rubber band" effect
          // animate_v2_to_target(&(curEnemy->position), target_pos, deltaTime, 0.25f);

          // NOTE: METHOD 2: Uniform speed
          Vector2 displacement = Vector2Subtract(target_pos, curEnemy->position);
          displacement = Vector2Scale(displacement, (ENEMY_SPEED * deltaTime) / Vector2Length(displacement));
          curEnemy->position = Vector2Add(curEnemy->position, displacement);
        
        if (CheckCollisionCircles(
					curEnemy->position,
					ENEMY_RADIUS,
					world->player.position,
					PLAYER_RADIUS)) {
			      touchedPlayer = true;
		      }
	      }

        if (touchedPlayer) {
          world->player.isValid = false;
        }
      }
    }

    { // ::LOOP RENDER
      BeginDrawing();
      {
        ClearBackground(RAYWHITE);

        { // ::RENDER CAMERA SPACE
          BeginMode2D(camera);
          render_tile_background(camera.target);

          DrawTexture(logo, -logo.width / 2.0f, -logo.height / 2.0f, (Color){255, 255, 255, 32});

          // ::RENDER PLAYER
          if (world->player.isValid) DrawCircleV(world->player.position, PLAYER_RADIUS, COLOR_PLAYER);

          // ::RENDER ENEMY
          for (int iEnemy = 0; iEnemy < NUM_ENEMIES; iEnemy++) {
            if (!world->enemies[iEnemy].isValid) continue;

            DrawCircleV(world->enemies[iEnemy].position, ENEMY_RADIUS, COLOR_ENEMY);
          }

          EndMode2D();
        }

        { // ::RENDER SCREEN SPACE
        if (IS_MOUSE_VALID) { // Render Mouse Crosshair
          const int INNER_RADIUS = 12;
          const int OUTER_RADIUS = 16;

          DrawCircleLinesV(SCREEN_MOUSE_POS, INNER_RADIUS + 1, RED);
          DrawCircleLinesV(SCREEN_MOUSE_POS, INNER_RADIUS, RED);
          DrawCircleLinesV(SCREEN_MOUSE_POS, INNER_RADIUS - 1, RED);
          DrawLineEx((Vector2){SCREEN_MOUSE_POS.x - OUTER_RADIUS, SCREEN_MOUSE_POS.y},
            (Vector2){SCREEN_MOUSE_POS.x + OUTER_RADIUS, SCREEN_MOUSE_POS.y},
            2.0f,
            RED);
          DrawLineEx((Vector2){SCREEN_MOUSE_POS.x, SCREEN_MOUSE_POS.y - OUTER_RADIUS},
            (Vector2){SCREEN_MOUSE_POS.x, SCREEN_MOUSE_POS.y + OUTER_RADIUS},
            2.0f,
            RED);
        }

        // ::RENDER BULLET
        for (int iBullet = 0; iBullet < NUM_BULLETS; iBullet++) {
          Bullet curBullet = world->bullets[iBullet];
          if (!curBullet.isValid) continue;

          DrawCircleV(curBullet.position, BULLET_RADIUS, COLOR_BULLET);
        }

        if (!world->isPlayerDead && touchedPlayer) {
          world->isPlayerDead = true;
          world->player.isValid = false;
        }
        // Add the below if statement
        if (!world->isGameWon && numEnemyAlive == 0) {
          world->isGameWon = true;
        }

        #if DEBUG
                  DrawText("This is a debug build", 10, 10, 20, LIGHTGRAY);
        #elif !DEBUG
                  DrawText("This is a release build", 10, 10, 20, LIGHTGRAY);
        #endif
        }
      }

      // EXIT WINDOW
      if (exitWindowRequested) {
        DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0, 0, 0, 128});
        DrawRectangle(0, 100, screenWidth, 200, RAYWHITE);
        DrawText("Are you sure you want to exit program? [Y/N]", 40, 180, 30, BLACK);
      }
      EndDrawing();
    }
  }

  free(world);
  UnloadTexture(logo);
  CloseWindow();
  return 0;
}
