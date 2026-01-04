
#include <raylib.h>
#include "fix_win32_compatibility.h"
#include "rlgl.h"
#include "raymath.h"
#include "raylib_platfrom.h"

#include "log.h"
#include "shared.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <winnt.h>

// TODO: fix timestep https://gafferongames.com/post/fix_your_timestep/
// TODO: Clean up hotloop code
// TODO: clean up this pile of code

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

void saveGameMemory(const char *filePath, GameMemory *gameMemory) {
  HANDLE file = CreateFileA(filePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  Assert(file != INVALID_HANDLE_VALUE);

  DWORD bytesWritten;
  BOOL result = WriteFile(file, gameMemory->permamentMemory,
                          gameMemory->permanentMemorySize, &bytesWritten, NULL);
  // Optionally check if bytesWritten == gameMemory->permanentMemorySize
  Assert(
      result !=
      0) // Will break stuff down the line. add handling if u come accros this.
      CloseHandle(file);
}

void loadGameMemory(const char *filePath, GameMemory *gameMemory) {
  HANDLE file = CreateFileA(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);
  Assert(file != INVALID_HANDLE_VALUE);

  DWORD bytesRead;
  BOOL result = ReadFile(file, gameMemory->permamentMemory,
                         gameMemory->permanentMemorySize, &bytesRead, NULL);
  Assert(
      result !=
      0); // Will break stuff down the line. add handling if u come accros this.
  Assert(bytesRead == gameMemory->permanentMemorySize);
  CloseHandle(file);
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

// static void appendToFile(FILE *h, void *base, size_t size) {
//   BOOL result = WriteFile(h, base, size, NULL, NULL);
// }

static void save_input_to_file(HANDLE h, Input *input) {
  DWORD written;
  BOOL result = WriteFile(h, input, sizeof(Input), &written, NULL);
  Assert(written == sizeof(Input));
  // Will break stuff down the line. add handling if u come accros this.
  Assert(result != 0);
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

  // TODO: 3D CODE move out of here in future
  Camera3D camera = {.position = (Vector3){100, 100, 100},
                     .target = (Vector3){50, 50, 0},
                     .up = (Vector3){0, 1, 0},
                     .fovy = 45,
                     .projection = CAMERA_PERSPECTIVE};
  // Camera3D camera = {0};
  // camera.position = (Vector3){50, 50, 100}; // Above the center, looking down
  // camera.target = (Vector3){50, 50, 0};     // Looking at the center
  // camera.up = (Vector3){0, 1, 0};
  // camera.fovy = 100; // This value is ignored for orthographic
  // camera.projection = CAMERA_ORTHOGRAPHIC;
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
  bool recordingInput = false;
  bool playBackOn = false;
  HANDLE inputFileHandle = NULL;
  Input loadedInput = {0};
  DisableCursor();

  double t = 0.0;
  const double dt = 1.0 / 40.0;
  double currentTime = GetTime();
  double accumulator = 0.0;
  Camera3D previousCamera = input.camera;
  Camera3D currentCamera = input.camera;

  while (!WindowShouldClose()) {
    double newTime = GetTime();
    double frameTime = newTime - currentTime;
    if (frameTime > 0.25)
      frameTime = 0.25;
    currentTime = newTime;
    accumulator += frameTime;

    previousCamera = currentCamera;
    Vector3 oldPosition = input.camera.position;

    // --- 2D-style orthographic camera controls ---
    // float panSpeed = 2.0f;
    // float zoomSpeed = 4.0f;
    //
    // if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
    //   input.camera.position.x += panSpeed;
    // if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
    //   input.camera.position.x -= panSpeed;
    // if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
    //   input.camera.position.y += panSpeed;
    // if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
    //   input.camera.position.y -= panSpeed;
    //
    // input.camera.target.x = input.camera.position.x;
    // input.camera.target.y = input.camera.position.y;
    //
    // float wheel = GetMouseWheelMove();
    // if (wheel != 0) {
    //   float zoomFactor = 1.15f;
    //   if (wheel > 0) {
    //     input.camera.fovy /= powf(zoomFactor, wheel);
    //   } else {
    //     input.camera.fovy *= powf(zoomFactor, -wheel);
    //   }
    //   if (input.camera.fovy < 5)
    //     input.camera.fovy = 5;
    //   if (input.camera.fovy > 1000)
    //     input.camera.fovy = 1000;
    // }
    // --- end 2D-style camera controls ---
    if (isCursorDisabled) {
      UpdateCamera(&input.camera, CAMERA_FREE);
      Vector3 positionDelta =
          Vector3Subtract(input.camera.position, oldPosition);
      float speedMultiplier = 5.0f;
      input.camera.position =
          Vector3Add(oldPosition, Vector3Scale(positionDelta, speedMultiplier));
    }
    currentCamera = input.camera;

    if (playBackOn) {
      printf("playing back input\n");
      DWORD bytesRead;
      BOOL result = ReadFile(inputFileHandle, &loadedInput, sizeof(Input),
                             &bytesRead, NULL);
      if (!result || bytesRead != sizeof(Input)) {
        loadGameMemory("test.tmp", &gameMemory);
        SetFilePointer(inputFileHandle, 0, NULL, FILE_BEGIN);
      } else {
        input = loadedInput;
      }
    } else {
      collect_input(&input);
    }
    if (playBackOn) {
      input = loadedInput;
    }

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
    if (IsKeyPressed(KEY_F2)) {
      recordingInput = recordingInput ? false : true;
      if (recordingInput == true) {
        CloseHandle(inputFileHandle);
        saveGameMemory("test.tmp", &gameMemory);
        inputFileHandle =
            CreateFileA("input.tmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

      } else {
        if (inputFileHandle) {
          CloseHandle(inputFileHandle);
          inputFileHandle =
              CreateFileA("input.tmp", GENERIC_READ, 0, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);
        }
      }
    }
    if (IsKeyPressed(KEY_F7)) {
      saveGameMemory("test.tmp", &gameMemory);
    }

    if (IsKeyPressed(KEY_F8)) {
      loadGameMemory("test.tmp", &gameMemory);
    }
    if (IsKeyPressed(KEY_F4)) {
      playBackOn = playBackOn ? false : true;
      if (playBackOn) {
        // Reset file position to beginning for playback
        if (inputFileHandle) {
          SetFilePointer(inputFileHandle, 0, NULL, FILE_BEGIN);
        }
      }
    }
    if (recordingInput) {
      printf("saving input each frame\n");
      save_input_to_file(inputFileHandle, &input);
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

    // https://gafferongames.com/post/fix_your_timestep/
    // NOt sure how to add interpolation to my "state" because of the
    // hotreload stuff....
    while (accumulator >= dt) {
      code.update(&gameMemory, &input, dt);
      accumulator -= dt;
      t += dt;
    }

    const double alpha = accumulator / dt;
    Camera3D renderCamera = currentCamera;
    renderCamera.position =
        Vector3Lerp(previousCamera.position, currentCamera.position, alpha);
    renderCamera.target =
        Vector3Lerp(previousCamera.target, currentCamera.target, alpha);

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(renderCamera);
    RenderQueue *renderQueue = gameMemory.renderQueue;
    // No render queue available yet, skip rendering
    if (!renderQueue) {
      EndMode3D();
      DrawFPS(10, 10);
      EndDrawing();
      flush_logs();
      continue;
    }
    if (renderQueue->isMeshReloadRequired) {
      UploadMesh(renderQueue->instanceMesh, false);
      renderQueue->isMeshReloadRequired = false;
    }

    // FIRST PASS
    // TODO: also consume the commands so second pass doenst have to iterate
    // trough all of them again
    for (int i = 0; i < renderQueue->count; i++) {
      RenderCommand cmd = renderQueue->commands[i];

      if (cmd.type == RENDER_INSTANCED) {
        // printf("Mesh OpenGL IDs - VAO: %u, VBO: %u vertexCount:%i\n",
        // cmd.instance.mesh->vaoId,
        // cmd.instance.mesh->vboId ? cmd.instance.mesh->vboId[0] : 0,
        // cmd.instance.mesh->vertexCount);
        DrawMeshInstanced(*cmd.instance.mesh, matinstances,
                          cmd.instance.transforms, cmd.instance.count);
      }
    }

    // Not sure what does but works
    rlDrawRenderBatchActive();

    // SECOND PASS
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
        // Already handled in first pass - skip
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
      case RENDER_SPHERE_3D: {
        DrawSphere(cmd.sphere3D.center, cmd.sphere3D.radius,
                   cmd.sphere3D.color);
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
