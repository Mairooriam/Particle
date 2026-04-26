// TODO: abstract away if more platfroms?
#include "platform.h"
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#include "windows/wCore.h"
#define PLATFORM_IMPL(NAME) wCore_##NAME
#elif defined(__linux__)
#include "linux/lCore.h"
#define PLATFORM_IMPL(NAME) lCore_##NAME
#else
#error "Unsupported platform"
#endif

// File API
MirFileResult MirFileOpen(const char *path, MirFile *out,
                          MirFileAccess accessType) {
  return PLATFORM_IMPL(file_open)(path, out, accessType);
}
MirFileResult MirFileWrite(MirFile *handle, const void *data, size_t size,
                           size_t *bytesWritten) {
  return PLATFORM_IMPL(file_write)(handle, data, size, bytesWritten);
}
void MirFileClose(MirFile *handle) { PLATFORM_IMPL(file_close)(handle); }
MirFileResult MirFileSize(MirFile *file, size_t *size) {
  return PLATFORM_IMPL(file_size)(file, size);
}
uint64_t MirFileLastWritetime(const char *filename) {
  return PLATFORM_IMPL(file_lastWritetime)(filename);
}

// DLL API
bool MirDLLLoad(const char *sourcePath, const char *tempPath,
                DLLHandle *outDLL) {
  return PLATFORM_IMPL(dll_load)(sourcePath, tempPath, outDLL);
}
void MirDLLUnload(DLLHandle *dll) { PLATFORM_IMPL(dll_unload)(dll); }
void *MirDLLGetFunction(DLLHandle *dll, const char *functionName) {
  return PLATFORM_IMPL(dll_getFunction)(dll, functionName);
}
bool MirDLLHasChanged(const char *sourcePath, const DLLHandle *dll) {
  return PLATFORM_IMPL(dll_hasChanged)(sourcePath, dll);
}
