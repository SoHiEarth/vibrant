#pragma once
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
