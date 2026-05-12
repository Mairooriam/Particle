#pragma once
#include "internal/mirMath.h"
#include <stdbool.h>
#include <stdint.h>

// ── Simple 2D/debug draw commands (unchanged) ───────────────────────────────
typedef enum {
  RENDER_RECTANGLE,
  RENDER_CIRCLE,
  RENDER_INSTANCED,
  RENDER_CUBE_3D,
  RENDER_LINE_3D,
  RENDER_SPHERE_3D
} RenderCommandType;

typedef struct {
  float x, y, z;   // position
  float r, g, b;   // color
  float radius;    // 0 = triangle, >0 = circle
} DrawCommand;

// ── Mesh types the app can request ──────────────────────────────────────────
typedef enum {
  MESH_TRIANGLE,
  MESH_SQUARE,
  MESH_TYPE_COUNT   // keep last — used by engine to size lookup tables
} MeshType;

// ── Per-instance data filled by the app ─────────────────────────────────────
typedef struct {
  float x, y, z;     // world position
  float r, g, b;     // tint color
  float scale;        // uniform scale
} InstanceData;

// ── One render item = one mesh type drawn N times (instanced) ───────────────
#define MAX_INSTANCES 1000
typedef struct {
  MeshType    mesh;
  InstanceData instances[MAX_INSTANCES];
  uint32_t    instanceCount;
} RenderItem;

// ── Queue holding both simple commands and instanced render items ────────────
#define MAX_DRAW_COMMANDS 10000
#define MAX_RENDER_ITEMS  64
typedef struct {
  // simple draw commands (legacy / 2D / debug)
  DrawCommand commands[MAX_DRAW_COMMANDS];
  int         count;

  // instanced 3D render items
  RenderItem  items[MAX_RENDER_ITEMS];
  int         itemCount;
} RenderQueue;

// ── Helper: push an instance onto an existing RenderItem slot ────────────────
// Returns 1 on success, 0 if full.
static inline int RenderItem_push(RenderItem *item, InstanceData data) {
  if (item->instanceCount >= MAX_INSTANCES) return 0;
  item->instances[item->instanceCount++] = data;
  return 1;
}

// ── Helper: find or create a RenderItem slot for a given MeshType ────────────
// Returns pointer to slot, or NULL if MAX_RENDER_ITEMS exceeded.
static inline RenderItem *RenderQueue_getItem(RenderQueue *rq, MeshType mesh) {
  for (int i = 0; i < rq->itemCount; i++) {
    if (rq->items[i].mesh == mesh) return &rq->items[i];
  }
  if (rq->itemCount >= MAX_RENDER_ITEMS) return (RenderItem *)0;
  RenderItem *slot = &rq->items[rq->itemCount++];
  slot->mesh = mesh;
  slot->instanceCount = 0;
  return slot;
}

// ── Helper: clear the queue for the next frame ───────────────────────────────
static inline void RenderQueue_clear(RenderQueue *rq) {
  rq->count = 0;
  rq->itemCount = 0;
}