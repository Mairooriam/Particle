#include "vulkanLayer.h"
#include "core.h"

#define LOG(fmt, ...) printf(fmt, ##__VA_ARGS__);

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
  // set_log_prefix("[loadGameCode] ");
  GameCode result = {0};
  result.currentDLLtimestamp = getFileLastWriteTime(sourceDLLfilepath);
  DeleteFileA(tempDLLfilepath);
  if (!CopyFile(sourceDLLfilepath, tempDLLfilepath, FALSE)) {
    DWORD error = GetLastError();
    LOG("Failed to copy DLL from %s to %s. Error: %lu", sourceDLLfilepath,
        tempDLLfilepath, error);
    result.isvalid = false;
    result.update = game_update_stub;
    return result;
  }

  result.gameCodeDLL = LoadLibraryA(tempDLLfilepath);
  if (!result.gameCodeDLL) {
    DWORD error = GetLastError();
    LOG("Failed to load DLL %s, error: %lu", tempDLLfilepath, error);
  }

  LOG("Trying to load .dlls");
  if (result.gameCodeDLL) {
    result.update =
        (GameUpdate *)(void *)GetProcAddress(result.gameCodeDLL, "game_update");
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
  // set_log_prefix("[unloadGameCode]");
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
  // input->mousePos = GetMousePosition();
  // for (int i = 0; i < 3; i++) {
  //   input->mouseButtons[i] = IsMouseButtonDown(i);
  // }
  // for (int i = 0; i < 256; i++) {
  //   input->keys[i] = IsKeyDown(i);
  // }
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
//
// void window_size_callback(GLFWwindow *window, int width, int height) {
//   (void)width;
//   (void)height;
//   printf("hello from callback\n");
//   vulkanContext *app = (vulkanContext *)(glfwGetWindowUserPointer(window));
//   app->framebufferResized = true;
// }

int main(int argc, char const *argv[]) {
  std::unique_ptr<vulkanContext> ctx = std::make_unique<vulkanContext>();
  init(ctx);
  while (!ctx->quit) {
    uint64_t lastTime{SDL_GetTicks()};
    drawFrame(ctx, lastTime);
    pollEvents(ctx, lastTime);
  }

  /* code */
  return 0;
}
