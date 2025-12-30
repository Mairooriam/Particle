
#include <raylib.h>
#include "rlgl.h"
#include "raymath.h"
#include "raylib_platfrom.h"

#include "log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static FILETIME getFileLastWriteTime(const char *filename) {
  FILETIME result;
  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  if (GetFileAttributesExA(filename, GetFileExInfoStandard, &fileInfo)) {
    result = fileInfo.ftLastWriteTime;
  } else {
    result = (FILETIME){0};
  }
  return result;
}

// source: https://guide.handmadehero.org/code/day021/
static GameCode loadGameCode(char *sourceDLLfilepath, char *tempDLLfilepath) {
  set_log_prefix("[loadGameCode] ");
  GameCode result = {0};
  result.currentDLLtimestamp = getFileLastWriteTime(sourceDLLfilepath);
  CopyFile(sourceDLLfilepath, tempDLLfilepath, FALSE);

  result.gameCodeDLL = LoadLibraryA(tempDLLfilepath);
  if (!result.gameCodeDLL) {
    DWORD error = GetLastError();
    LOG("Failed to load DLL %s, error: %lu", tempDLLfilepath, error);
  }

  LOG("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)GetProcAddress(result.gameCodeDLL, "game_update");
    if (result.update) {
      result.isvalid = true;
      LOG("Loading .dlls was succesfull");
    } else {
      LOG("Failed to get function address for game_update");
    }
  }

  if (!result.isvalid) {
    result.update = game_update_stub;
    LOG("Loading .dlls wasn't succesfull. Resetting to stub functions. "
        "DLL_SOURCE: %s , DLL_TEMP: %s",
        sourceDLLfilepath, tempDLLfilepath);
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

static void ConcatStrings(size_t sourceACount, char *sourceAstr,
                          size_t sourceBCount, char *sourceBstr,
                          size_t destCount, char *destStr) {
  (void)destCount;
  // TOOO: overflowing stuff
  for (size_t i = 0; i < sourceACount; i++) {
    *destStr++ = *sourceAstr++;
  }
  for (size_t i = 0; i < sourceBCount; i++) {
    *destStr++ = *sourceBstr++;
  }
  *destStr++ = 0;
}
// TODO: use something other than raylib?
static void collect_input(Input *input) {
  input->mousePos = GetMousePosition();
  for (int i = 0; i < 3; i++) {
    input->mouseButtons[i] = IsMouseButtonDown(i);
  }
  for (int i = 0; i < 256; i++) {
    input->keys[i] = IsKeyDown(i);
  }
}
int main() {
  char EXEDirPath[MAX_PATH];
  DWORD SizeOfFilename = GetModuleFileNameA(0, EXEDirPath, sizeof(EXEDirPath));
  (void)SizeOfFilename;

  char sourceDLLfilename[] = "application.dll";
  char sourceDLLfilepath[MAX_PATH];
  char tempDLLfilepath[MAX_PATH];
  char tempDLLfilename[] = "libapplication_temp.dll";

  char *onePastLastSlash = EXEDirPath;
  for (char *scan = EXEDirPath; *scan; ++scan) {
    if (*scan == '\\') {
      onePastLastSlash = scan + 1;
    }
  }

  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath,
                sizeof(sourceDLLfilename) - 1, sourceDLLfilename,
                sizeof(sourceDLLfilepath), sourceDLLfilepath);

  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath,
                sizeof(tempDLLfilename) - 1, tempDLLfilename,
                sizeof(tempDLLfilepath), tempDLLfilepath);

  GameCode code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
  code.reloadDLLRequested = false;
  code.reloadDLLDelay = 0.0f;

  InitWindow(800, 600, "Hot-reload Example");

#if INTERNAL_BUILD
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);
#else
  LPVOID baseAddress = 0;
#endif

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(64);
  gameMemory.transientMemorySize = GigaBytes((uint64_t)4);

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    LOG("FAILED TO ALLOC PERMANENT MEMORY");
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);

  if (!gameMemory.transientMemory) {
    LOG("FAILED TO ALLOC TRANSIENT MEMORY");
  }
  Input input = {0};
  SetTargetFPS(60);
  float frameTime = 1.0f;

  // TODO: 3D CODE move out of here in future
  Camera3D camera = {.position = (Vector3){100, 100, 100},
                     .target = (Vector3){50, 50, 0},
                     .up = (Vector3){0, 1, 0},
                     .fovy = 45,
                     .projection = CAMERA_PERSPECTIVE};
  input.camera = camera;
  Shader shader = LoadShader("lighting_instancing.vs", "lighting.fs");
  Material matinstances = LoadMaterialDefault();
  shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
  shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
  int ambientLoc = GetShaderLocation(shader, "ambient");
  SetShaderValue(shader, ambientLoc, (float[4]){0.2f, 0.2f, 0.2f, 1.0f},
                 SHADER_UNIFORM_VEC4);
  matinstances.shader = shader;
  matinstances.maps[MATERIAL_MAP_DIFFUSE].color = RED;
  // END OF 3D SHIT
  bool isCursorDisabled = true;
  DisableCursor();
  while (!WindowShouldClose()) {
    Vector3 oldPosition = input.camera.position;
    if (isCursorDisabled) {
      UpdateCamera(&input.camera, CAMERA_FREE);
      Vector3 positionDelta =
          Vector3Subtract(input.camera.position, oldPosition);
      float speedMultiplier = 5.0f;
      input.camera.position =
          Vector3Add(oldPosition, Vector3Scale(positionDelta, speedMultiplier));
    }

    frameTime = GetFrameTime();
    collect_input(&input);

    if (code.reloadDLLRequested) {
      code.clock += frameTime;
    }

    if (IsKeyPressed(KEY_F1)) {
      if (isCursorDisabled) {
        EnableCursor();
      } else {
        DisableCursor();
      }
      isCursorDisabled = !isCursorDisabled;
    }

    if (code.reloadDLLRequested && (code.clock >= code.reloadDLLDelay)) {
      LOG("Reloading DLLs.");
      unloadGameCode(&code);
      code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
      code.reloadDLLRequested = false;
      code.clock = 0;
    }

    FILETIME time = getFileLastWriteTime(sourceDLLfilepath);
    if (CompareFileTime(&time, &code.currentDLLtimestamp) != 0) {
      code.reloadDLLRequested = true;
    }

    code.update(&gameMemory, &input, frameTime);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(input.camera);
    RenderQueue *renderQueue = (RenderQueue *)gameMemory.transientMemory;
    if (renderQueue->isMeshReloadRequired) {
      UploadMesh(&renderQueue->instanceMesh, false);
      renderQueue->isMeshReloadRequired = false;
    }
    for (int i = 0; i < renderQueue->count; i++) {
      RenderCommand cmd = renderQueue->commands[i];
      switch (cmd.type) {
      case RENDER_RECTANGLE: {

        Color color = (Color){cmd.rectangle.color.r, cmd.rectangle.color.g,
                              cmd.rectangle.color.b, cmd.rectangle.color.a};
        DrawRectangle(cmd.rectangle.x, cmd.rectangle.y, cmd.rectangle.width,
                      cmd.rectangle.height, color);
      } break;
      case RENDER_CIRCLE: {
        Color color = (Color){cmd.circle.color.r, cmd.circle.color.g,
                              cmd.circle.color.b, cmd.circle.color.a};
        DrawSphere((Vector3){cmd.circle.centerX, cmd.circle.centerY, 0},
                   cmd.circle.radius, color);
      } break;
      case RENDER_INSTANCED: {
        // TODO: FOR FUTURE
        //  DrawMeshInstanced(*cmd.instance.mesh, *cmd.instance.material,
        //                    cmd.instance.transforms, cmd.instance.count);
        DrawMeshInstanced(*cmd.instance.mesh, matinstances,
                          cmd.instance.transforms, cmd.instance.count);

      } break;
      case RENDER_LINE_3D: {
        DrawLine3D(cmd.line3D.start, cmd.line3D.end, cmd.line3D.color);
      } break;
      case RENDER_CUBE_3D: {
        Vector3 corner = {0, 0, 0};
        float width = cmd.cube3D.width, height = cmd.cube3D.height,
              depth = cmd.cube3D.depth;
        Vector3 center = {corner.x, corner.y, corner.z};

        if (cmd.cube3D.origin == 1) {
          center = (Vector3){corner.x + width / 2, corner.y + height / 2,
                             corner.z + depth / 2};
        }

        if (cmd.cube3D.wireFrame) {
          DrawCubeWires(center, width, height, depth, cmd.cube3D.color);
        } else {
          DrawCube(center, width, height, depth, cmd.cube3D.color);
        }
      } break;
      }
    }
    EndMode3D();
    DrawFPS(10, 10);
    EndDrawing();
    flush_logs();
  }
  CloseWindow();

  return 0;
}
