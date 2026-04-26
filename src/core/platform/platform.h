#ifndef CORE_PLATFORM_H
#define CORE_PLATFORM_H

#include "platform_types.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

// Public API declarations only
MirFileResult MirFileSize(MirFile *file, size_t *size);
MirFileResult MirFileOpen(const char *path, MirFile *out,
                          MirFileAccess accesType);
MirFileResult MirFileWrite(MirFile *handle, const void *data, size_t size,
                           size_t *bytesWritten);
uint64_t MirFileLastWritetime(const char *filename);
void MirFileClose(MirFile *handle);

// DLL API
typedef struct {
  void *handle;
  uint64_t lastWriteTime;
} DLLHandle;

bool MirDLLLoad(const char *sourcePath, const char *tempPath,
                DLLHandle *outDLL);
void MirDLLUnload(DLLHandle *dll);
void *MirDLLGetFunction(DLLHandle *dll, const char *functionName);
bool MirDLLHasChanged(const char *sourcePath, const DLLHandle *dll);
#ifdef __cplusplus
}
#endif
#endif // CORE_PLATFORM_H
