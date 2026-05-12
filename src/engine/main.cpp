#include "vulkanLayer.h"
#include <core/log.h>
#include <core/string.h>
#include <assert.h>
#include <core/platform/platform.h>
#include <stdint.h>

#include "shared.h"
typedef struct {
  HMODULE gameCodeDLL;
  uint64_t currentDLLtimestamp;
  bool reloadDLLRequested;
  float reloadDLLDelay;
  float clock;
  GameUpdate *update;
  bool isvalid;
  bool reloadDLLExecuted;
} GameCode;

// source: https://guide.handmadehero.org/code/day021/
static GameCode loadGameCode(char *sourceDLLfilepath, char *tempDLLfilepath) {
  GameCode result = {0};
  result.currentDLLtimestamp = MirFileLastWritetime(sourceDLLfilepath);
  CopyFile(sourceDLLfilepath, tempDLLfilepath, FALSE);

  result.gameCodeDLL = LoadLibraryA(tempDLLfilepath);
  if (!result.gameCodeDLL) {
    DWORD error = GetLastError();
    log_error("Failed to load DLL %s, error: %lu", tempDLLfilepath, error);
  }

  log_debug("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)GetProcAddress(result.gameCodeDLL, "game_update");
    if (result.update) {
      result.isvalid = true;
      log_debug("Loading .dlls was succesfull");
    } else {
      log_debug("Failed to get function address for game_update");
    }
  }

  if (!result.isvalid) {
    result.update = game_update_stub;
    log_error("Loading .dlls wasn't succesfull. Resetting to stub functions. "
              "DLL_SOURCE: %s , DLL_TEMP: %s",
              sourceDLLfilepath, tempDLLfilepath);
  }

  return result;
}
static void unloadGameCode(GameCode *gameCode) {
  if (gameCode->gameCodeDLL) {
    FreeLibrary(gameCode->gameCodeDLL);
    log_info("Freed .dlls");
  }
  gameCode->isvalid = false;
  gameCode->update = game_update_stub;
}
int main(int argc, char const *argv[]) {
  char EXEDirPath[MAX_PATH];
  DWORD SizeOfFilename = GetModuleFileNameA(0, EXEDirPath, sizeof(EXEDirPath));
  (void)SizeOfFilename;

  char sourceDLLfilename[] = "application.dll";
  char sourceDLLfilepath[MAX_PATH];
  char tempDLLfilepath[MAX_PATH];
  char shaderDir[] = "/shader";
  char shaderPath[MAX_PATH];
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
  ConcatStrings(onePastLastSlash - EXEDirPath, EXEDirPath, sizeof(shaderDir),
                shaderDir, sizeof(shaderPath), shaderPath);

  GameCode code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
  code.reloadDLLRequested = false;
  code.reloadDLLDelay = 0.1f;

#if INTERNAL_BUILD
  LPVOID baseAddress = (LPVOID)TeraBytes((uint64_t)2);
#else
  LPVOID baseAddress = 0;
#endif

  GameMemory gameMemory = {0};
  gameMemory.permanentMemorySize = MegaBytes(512);
  gameMemory.transientMemorySize = MegaBytes(512);
  gameMemory.reloadDLLHappened = false;

  uint64_t totalSize =
      gameMemory.permanentMemorySize + gameMemory.transientMemorySize;
  gameMemory.permamentMemory = VirtualAlloc(
      baseAddress, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!gameMemory.permamentMemory) {
    log_error("FAILED TO ALLOC PERMANENT MEMORY");
    assert(0 && "lol alloc failed");
  }
  gameMemory.transientMemory =
      ((uint8_t *)gameMemory.permamentMemory + gameMemory.permanentMemorySize);

  if (!gameMemory.permamentMemory) {
    log_info("FAILED TO ALLOC PERMANENT MEMORY");
    assert(0 && "lol alloc failed");
  }
  log_info("Allocated memory:");
  log_info("  Permanent Memory: %p (Size: %llu)", gameMemory.permamentMemory,
           gameMemory.permanentMemorySize);
  log_info("  Transient Memory: %p (Size: %llu)", gameMemory.transientMemory,
           gameMemory.transientMemorySize);

  std::unique_ptr<vulkanContext> ctx = std::make_unique<vulkanContext>();
  init(ctx);
  RenderQueue renderQueue = {.commands[0]{0, 0, 0, 0, 0, 0, 2}};
  gameMemory.renderQueue = &renderQueue;
  while (!ctx->quit) {
    uint64_t lastTime{SDL_GetTicks()};
    code.clock += lastTime;
    // ==================== FILE WATCHER ====================
    if (code.reloadDLLRequested && (code.clock >= code.reloadDLLDelay)) {
      log_info("Reloading DLLs.");
      unloadGameCode(&code);
      code = loadGameCode(sourceDLLfilepath, tempDLLfilepath);
      code.reloadDLLRequested = false;
      code.clock = 0;
      gameMemory.reloadDLLHappened = true;
    }

    uint64_t time = MirFileLastWritetime(sourceDLLfilepath);
    if (time != code.currentDLLtimestamp) {
      log_info("reloadDLLRequested!");
      code.reloadDLLRequested = true;
    }
    Input input = {};

    code.update(&gameMemory, &input, lastTime);
    drawFrame(ctx, lastTime, &renderQueue);
    pollEvents(ctx, lastTime);
  }

  /* code */
  return 0;
}
