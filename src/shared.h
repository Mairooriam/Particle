#pragma once
#include <stdbool.h>
#define Assert(Expression)                                                     \
  if (!(Expression)) {                                                         \
    *(int *)0 = 0;                                                             \
  }
#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((KiloBytes(value)) * 1024)
#define GigaBytes(value) ((MegaBytes(value)) * 1024)
#define TeraBytes(value) ((GigaBytes(value)) * 1024)

typedef struct {
  bool isInitialized;
  void *permamentMemory;
  size_t permanentMemorySize;
  void *transientMemory;
  size_t transientMemorySize;
} GameMemory;

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

#endif
typedef struct {
  Vector2 mousePos;
  Camera3D camera;
  bool mouseButtons[3]; // Left, middle, right
  bool keys[256];       // Keyboard state
} Input;

#define GAME_UPDATE(name)                                                      \
  void name(GameMemory *gameMemory, Input *input, float frameTime)
typedef GAME_UPDATE(GameUpdate);
GAME_UPDATE(game_update_stub) {
  (void)gameMemory;
  (void)frameTime;
  (void)input;
}


typedef enum {
  RENDER_RECTANGLE,
  RENDER_CIRCLE,
  RENDER_INSTANCED
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
      Mesh *mesh;
      Matrix *transforms;
      Material *material;
      size_t count;
    } instance;
  };
} RenderCommand;

#define MAX_RENDER_COMMANDS 1000000
typedef struct {
  RenderCommand commands[MAX_RENDER_COMMANDS];
  int count;
} RenderQueue;