#pragma once
#include "internal/mirMath.h"
#include <stdbool.h>

typedef enum {
  RENDER_RECTANGLE,
  RENDER_CIRCLE,
  RENDER_INSTANCED,
  RENDER_CUBE_3D,
  RENDER_LINE_3D,
  RENDER_SPHERE_3D
} RenderCommandType;

typedef struct {
  float x, y, z;       // position
  float r, g, b;       // color
  float radius;        // 0 = not a circle (triangle), >0 = circle
} DrawCommand;

#define MAX_DRAW_COMMANDS 10000
typedef struct {
  DrawCommand commands[MAX_DRAW_COMMANDS];
  int count;
} RenderQueue;