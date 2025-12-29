#pragma once
#include "fix_win32_compatibility.h"
#include "shared.h"
typedef struct {
  HMODULE gameCodeDLL;
  FILETIME currentDLLtimestamp;
  bool reloadDLLRequested;
  float reloadDLLDelay;
  float clock;
  GameUpdate *update;
  bool isvalid;
} GameCode;