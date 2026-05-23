#pragma once // also add this
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#define MONKI_COUNT 5
struct ShaderData {
  glm::mat4 projection;
  glm::mat4 view;
  glm::mat4 model[MONKI_COUNT];
  glm::vec4 lightPos{0.0f, -10.0f, 10.0f, 0.0f};
  uint32_t selected{1};
};
