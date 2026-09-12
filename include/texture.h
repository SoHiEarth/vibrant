#ifndef TEXTURE_H
#define TEXTURE_H
#include <string>

struct Texture {
  unsigned int id;
  std::string path;
};

Texture LoadTexture(const std::string& path);
#endif  // TEXTURE_H
