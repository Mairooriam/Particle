#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "utils.h"
#include "cglm/cglm.h"

#include "cglm/affine2d.h"
#include "cglm/cam.h"
#include "cglm/mat4.h"

#define Assert(expression)                                                     \
  if (!(expression)) {                                                         \
    __builtin_trap();                                                          \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)
#define TeraBytes(value) ((GigaBytes(value)) * 1024)

// to not clash with raylib when included in main.c
#ifndef RAYLIB_H
typedef struct {
  float x, y;
} Vector2;

typedef struct {
  float x, y, z;
} Vector3;

// Camera, defines position/orientation in 3d space
typedef struct Camera3D {
  Vector3 position; // Camera position
  Vector3 target;   // Camera target it looks-at
  Vector3 up;       // Camera up vector (rotation over its axis)
  float fovy; // Camera field-of-view aperture in Y (degrees) in perspective,
              // used as near plane height in world units in orthographic
  int projection; // Camera projection: CAMERA_PERSPECTIVE or
                  // CAMERA_ORTHOGRAPHIC
} Camera3D;

typedef struct {
  unsigned char r, g, b, a;
} Color;

typedef struct {
  float x, y, z, w;
} Vector4;

typedef struct {
  float m0, m4, m8, m12;
  float m1, m5, m9, m13;
  float m2, m6, m10, m14;
  float m3, m7, m11, m15;
} Matrix;

// Mesh, vertex data and vao/vbo
typedef struct Mesh {
  int vertexCount;   // Number of vertices stored in arrays
  int triangleCount; // Number of triangles stored (indexed or not)

  // Vertex attributes data
  float *vertices;  // Vertex position (XYZ - 3 components per vertex)
                    // (shader-location = 0)
  float *texcoords; // Vertex texture coordinates (UV - 2 components per vertex)
                    // (shader-location = 1)
  float *texcoords2; // Vertex texture second coordinates (UV - 2 components per
                     // vertex) (shader-location = 5)
  float *normals;    // Vertex normals (XYZ - 3 components per vertex)
                     // (shader-location = 2)
  float *tangents;   // Vertex tangents (XYZW - 4 components per vertex)
                     // (shader-location = 4)
  unsigned char *colors;   // Vertex colors (RGBA - 4 components per vertex)
                           // (shader-location = 3)
  unsigned short *indices; // Vertex indices (in case vertex data comes indexed)

  // Animation vertex data
  float
      *animVertices;  // Animated vertex positions (after bones transformations)
  float *animNormals; // Animated normals (after bones transformations)
  unsigned char
      *boneIds; // Vertex bone ids, max 255 bone ids, up to 4 bones influence by
                // vertex (skinning) (shader-location = 6)
  float *boneWeights;   // Vertex bone weight, up to 4 bones influence by vertex
                        // (skinning) (shader-location = 7)
  Matrix *boneMatrices; // Bones animated transformation matrices
  int boneCount;        // Number of bones

  // OpenGL identifiers
  unsigned int vaoId;  // OpenGL Vertex Array Object id
  unsigned int *vboId; // OpenGL Vertex Buffer Objects id (default vertex data)
} Mesh;

// Shader
typedef struct Shader {
  unsigned int id; // Shader program id
  int *locs;       // Shader locations array (RL_MAX_SHADER_LOCATIONS)
} Shader;

// Texture, tex data stored in GPU memory (VRAM)
typedef struct Texture {
  unsigned int id; // OpenGL texture id
  int width;       // Texture base width
  int height;      // Texture base height
  int mipmaps;     // Mipmap levels, 1 by default
  int format;      // Data format (PixelFormat type)
} Texture;

// Texture2D, same as Texture
typedef Texture Texture2D;

// MaterialMap
typedef struct MaterialMap {
  Texture2D texture; // Material map texture
  Color color;       // Material map color
  float value;       // Material map value
} MaterialMap;

// Material, includes shader and maps
typedef struct Material {
  Shader shader;     // Material shader
  MaterialMap *maps; // Material maps array (MAX_MATERIAL_MAPS)
  float params[4];   // Material generic parameters (if required)
} Material;

typedef Vector4 Quaternion;
static Matrix MatrixScale(float x, float y, float z) {
  Matrix result = {x,    0.0f, 0.0f, 0.0f, 0.0f, y,    0.0f, 0.0f,
                   0.0f, 0.0f, z,    0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

  return result;
}
static Matrix MatrixMultiply(Matrix left, Matrix right) {
  Matrix result = {0};

  result.m0 = left.m0 * right.m0 + left.m1 * right.m4 + left.m2 * right.m8 +
              left.m3 * right.m12;
  result.m1 = left.m0 * right.m1 + left.m1 * right.m5 + left.m2 * right.m9 +
              left.m3 * right.m13;
  result.m2 = left.m0 * right.m2 + left.m1 * right.m6 + left.m2 * right.m10 +
              left.m3 * right.m14;
  result.m3 = left.m0 * right.m3 + left.m1 * right.m7 + left.m2 * right.m11 +
              left.m3 * right.m15;
  result.m4 = left.m4 * right.m0 + left.m5 * right.m4 + left.m6 * right.m8 +
              left.m7 * right.m12;
  result.m5 = left.m4 * right.m1 + left.m5 * right.m5 + left.m6 * right.m9 +
              left.m7 * right.m13;
  result.m6 = left.m4 * right.m2 + left.m5 * right.m6 + left.m6 * right.m10 +
              left.m7 * right.m14;
  result.m7 = left.m4 * right.m3 + left.m5 * right.m7 + left.m6 * right.m11 +
              left.m7 * right.m15;
  result.m8 = left.m8 * right.m0 + left.m9 * right.m4 + left.m10 * right.m8 +
              left.m11 * right.m12;
  result.m9 = left.m8 * right.m1 + left.m9 * right.m5 + left.m10 * right.m9 +
              left.m11 * right.m13;
  result.m10 = left.m8 * right.m2 + left.m9 * right.m6 + left.m10 * right.m10 +
               left.m11 * right.m14;
  result.m11 = left.m8 * right.m3 + left.m9 * right.m7 + left.m10 * right.m11 +
               left.m11 * right.m15;
  result.m12 = left.m12 * right.m0 + left.m13 * right.m4 + left.m14 * right.m8 +
               left.m15 * right.m12;
  result.m13 = left.m12 * right.m1 + left.m13 * right.m5 + left.m14 * right.m9 +
               left.m15 * right.m13;
  result.m14 = left.m12 * right.m2 + left.m13 * right.m6 +
               left.m14 * right.m10 + left.m15 * right.m14;
  result.m15 = left.m12 * right.m3 + left.m13 * right.m7 +
               left.m14 * right.m11 + left.m15 * right.m15;

  return result;
}

#endif
typedef struct {
  mat4 *items;
  size_t capacity;
  size_t count;
} arr_mat4;
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
      Mesh *mesh; // For now all use same mesh
      Matrix *transforms;
      Material *material;
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
      mat4 transform;
    } transform;
  };
} RenderCommand;

#define MAX_RENDER_COMMANDS 1000000
typedef struct {
  bool isMeshReloadRequired;
  Mesh *instanceMesh;
  RenderCommand commands[MAX_RENDER_COMMANDS];
  int count;
} RenderQueue;
typedef struct {
  vec2 pos;
  vec3 color;
} Vertex;
typedef struct {
  bool isInitialized;
  Vertex *vertices;   // REMOVE IN  FUTURE
  size_t vertexCount; // REMOVE IN FUTURE
  size_t permanentMemorySize;
  size_t transientMemorySize;
  RenderQueue *renderQueue;
  arr_mat4 *transforms;
  vec4 *instanceColors;
  void *permamentMemory;
  void *transientMemory;
  bool reloadDLLHappened;
} GameMemory;

typedef struct {
  Vector2 mousePos;
  Camera3D camera;
  bool mouseButtons[3]; // Left, middle, right
  bool keys[256];       // Keyboard state
} Input;

#define GAME_UPDATE(name)                                                      \
  void name(GameMemory *gameMemory, Input *input, float frameTime)
typedef GAME_UPDATE(GameUpdate);
static GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
  (void)input;
}

typedef struct {
  uint32_t *items;
  size_t count;
  size_t capacity;
} arr_uint32_t;
DA_CREATE(arr_uint32_t)
DA_FREE(arr_uint32_t)
DA_INIT(arr_uint32_t)
// TODO: make proper utils.h not spread around utls and other files
#define ARR_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))
#define CLAMP(val, min, max)                                                   \
  ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))
