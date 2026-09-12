#pragma once

struct TexturePack {
  unsigned int color, normal;
};

struct Framebuffer {
  unsigned int id;
  unsigned int colorbuffer;
  unsigned int depthbuffer;
};
