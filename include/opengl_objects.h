#ifndef OPENGL_OBJECTS_H
#define OPENGL_OBJECTS_H

#include <glm/glm.hpp>

struct TexturePack {
  unsigned int color, normal;
};

struct Framebuffer {
  unsigned int id;
  float scale;
  glm::ivec2 size;
  unsigned int colorbuffer;
  unsigned int depthbuffer;
};

#endif  // OPENGL_OBJECTS_H
