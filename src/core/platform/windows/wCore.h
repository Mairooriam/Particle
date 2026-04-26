#pragma once
#include <stdbool.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../platform_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== UTILS ====================
uint64_t combine_u32_u32(DWORD low, DWORD high);

// ==================== FILE I/O ====================

void wCore_file_FiletimeToString(char *str, size_t bufsize, FILETIME ft);

// File I/O API (Windows-specific, for platform.c dispatch)
MirFileResult wCore_file_open(const char *path, MirFile *out,
                              MirFileAccess accessType);
MirFileResult wCore_file_write(MirFile *handle, const void *data, size_t size,
                               size_t *bytesWritten);
void wCore_file_close(MirFile *handle);
MirFileResult wCore_file_size(MirFile *file, size_t *size);
uint64_t wCore_file_lastWritetime(const char *filename);

// ==================== CLIPBOARD ====================

// ==================== DLL ====================
typedef struct {
  HMODULE handle;
  FILETIME lastWriteTime;
} wCore_DLLHandle;
bool wCore_dll_load(const char *sourcePath, const char *tempPath,
                    wCore_DLLHandle *outDLL);
void wCore_dll_unload(wCore_DLLHandle *dll);
bool wCore_dll_hasChanged(const char *sourcePath, const wCore_DLLHandle *dll);
void *wCore_dll_getFunction(wCore_DLLHandle *dll, const char *functionName);
#define wCore_dll_LOAD_DLL_FUNCTION(dll, funcPtr, funcName, funcType,          \
                                    successFlag)                               \
  do {                                                                         \
    funcPtr = (funcType)_wCore_dll_getFunction(dll, funcName);                 \
    if (!funcPtr) {                                                            \
      successFlag = false;                                                     \
    }                                                                          \
  } while (0)

#ifdef __cplusplus
}
#endif
