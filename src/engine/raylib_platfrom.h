#pragma once
#include "shared/shared.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
typedef struct {
  HMODULE gameCodeDLL;
  FILETIME currentDLLtimestamp;
  bool reloadDLLRequested;
  float reloadDLLDelay;
  float clock;
  GameUpdate *update;
  bool isvalid;
  bool reloadDLLExecuted;

} GameCode;
