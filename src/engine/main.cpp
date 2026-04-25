#include "vulkanLayer.h"
#include <core/log.h>

int main(int argc, char const *argv[]) {
  std::unique_ptr<vulkanContext> ctx = std::make_unique<vulkanContext>();
  init(ctx);
  while (!ctx->quit) {
    uint64_t lastTime{SDL_GetTicks()};
    drawFrame(ctx, lastTime);
    log_error("Testing");
    pollEvents(ctx, lastTime);
  }

  /* code */
  return 0;
}
