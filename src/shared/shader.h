#pragma once
#include <cglm/cglm.h>
#include <stdint.h>

#define MONKI_COUNT 5

typedef struct ShaderData {
  mat4 projection;
  mat4 view;
  mat4 model[MONKI_COUNT];
  vec4 lightPos;
  uint32_t selected;
} ShaderData;