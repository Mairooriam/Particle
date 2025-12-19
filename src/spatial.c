#include "spatial.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI  // Exclude GDI (prevents Rectangle conflict)
#define NOUSER // Exclude User32 (prevents CloseWindow, ShowCursor conflicts)
#include <windows.h>
#endif

#define NOB_IMPLEMENTATION
#include "nob.h"

void render(SpatialContext *ctx) {
  for (size_t i = 0; i < ctx->particles->count; i++) {
    Particle p = ctx->particles->items[i];
    // DrawCircleV(p.pos, 5.0f, RED);
    DrawCircleSector(p.pos, 5.0f, 0, 360, 6, RED);
  }
}

void handle_input(SpatialContext *ctx) { NOB_UNUSED(ctx); }

void handle_update(SpatialContext *ctx) {

  for (size_t i = 0; i < ctx->particles->count; i++) {
    Particle *p = &ctx->particles->items[i];
    particle_update(ctx, p);
  }
}
void particle_update(SpatialContext *ctx, Particle *p) {
  float bounds_min = 0.0f;
  float bounds_max_x = ctx->window.width;
  float bounds_max_y = ctx->window.height;
  p->pos.x = p->pos.x + p->v.x * ctx->frameTime;
  p->pos.y = p->pos.y + p->v.y * ctx->frameTime;

  if (p->pos.x < bounds_min) {
    p->pos.x = bounds_min;
    p->v.x = p->v.x * -1;
  } else if (p->pos.x > bounds_max_x) {
    p->pos.x = bounds_max_x;
    p->v.x = p->v.x * -1;
  }

  if (p->pos.y < bounds_min) {
    p->pos.y = bounds_min;
    p->v.y = p->v.y * -1;

  } else if (p->pos.y > bounds_max_y) {
    p->pos.y = bounds_max_y;
    p->v.y = p->v.y * -1;
  }
}
