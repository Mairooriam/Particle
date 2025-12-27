
#include "application.h"
#include "fix_win32_compatibility.h"
#include "log.h"
#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
typedef struct {
  HMODULE gameCodeDLL;
  GameUpdate *update;
  bool isvalid;
} GameCode;

// source: https://guide.handmadehero.org/code/day021/
static GameCode loadGameCode() {
  set_log_prefix("[loadGameCode] ");
  GameCode result = {0};
  CopyFile("libapplication.dll", "gameCodeDLL_Temp.dll", FALSE);
  result.gameCodeDLL = LoadLibraryA("gameCodeDLL_Temp.dll");
  LOG("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)GetProcAddress(result.gameCodeDLL, "game_update");
    if (result.update) {
      result.isvalid = true;
      LOG("Loading .dlls was succesfull");
    }
  }

  if (!result.isvalid) {
    result.update = game_update_stub;
    LOG("Loading .dlls wasn't succesfull. Resetting to stub functions");
  }

  return result;
}

static void unloadGameCode(GameCode *gameCode) {
  set_log_prefix("[unloadGameCode]");
  if (gameCode->gameCodeDLL) {
    FreeLibrary(gameCode->gameCodeDLL);
    LOG("Freed .dlls");
  }
  gameCode->isvalid = false;
  gameCode->update = game_update_stub;
}

typedef struct {
  Position pos;
} GameState;
GameState *init_gameState() {
  GameState *result = malloc(sizeof(GameState));
  if (!result) {
    assert(0 && "LMAO failed");
  }

  result->pos.x = 0;
  result->pos.y = 0;

  return result;
}

int main() {
  GameCode code = loadGameCode();

  GameState *game = init_gameState();

  InitWindow(800, 600, "Hot-reload Example");
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    flush_logs();

    // float dt = GetFrameTime();
    printf("x:%f,y:%f\n", game->pos.x, game->pos.y);
    code.update(&game->pos);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    // api.render(app_ctx);
    EndDrawing();

    if (IsKeyPressed(KEY_F5)) {
      unloadGameCode(&code);
      code = loadGameCode();
    }
  }
  CloseWindow();

  return 0;
}
