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
  RenderCommandType type;
  union {
    struct {
      float x, y, width, height;
      Color color;
    } rectangle;
    struct {
      float centerX, centerY, radius;
      Color color;
    } circle;
    struct {
      // Mesh *mesh; // For now all use same mesh
      Matrix *transforms;
      // Material *material;
      size_t count;
    } instance;
    struct {
      Vector3 start;
      Vector3 end;
      Color color;
    } line3D;
    struct {
      bool wireFrame;
      int origin; // 0 = center, 1 = ???, 2 = ??
      float height;
      float width;
      float depth;
      Color color;
    } cube3D;
    struct {
      Vector3 center;
      float radius;
      Color color;
    } sphere3D;
    struct {
      Matrix transform;
    } transform;
  };
} RenderCommand;

#define MAX_RENDER_COMMANDS 1000000
typedef struct {
  bool isMeshReloadRequired;
  // Mesh *instanceMesh;
  RenderCommand commands[MAX_RENDER_COMMANDS];
  int count;
} RenderQueue;
