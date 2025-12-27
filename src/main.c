
#include "application.h"
#include "fix_win32_compatibility.h"
#include "log.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdio.h>
#include <winbase.h>
typedef struct {
  HMODULE gameCodeDLL;
  GameUpdate *update;
  bool isvalid;
} GameCode;

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

int main() {
  GameCode code = loadGameCode();

  InitWindow(800, 600, "Hot-reload Example");
  SetTargetFPS(60);
  Position pos = {0, 0};
  while (!WindowShouldClose()) {
    flush_logs();

    // float dt = GetFrameTime();
    printf("x:%f,y:%f\n", pos.x, pos.y);
    code.update(&pos);

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
