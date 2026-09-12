#pragma once
#include "transform.h"
#include "light.h"
#include "opengl_objects.h"
#include <GLFW/glfw3.h>

struct VertexAttributeCreateInfo {
  unsigned int index;
  int size;
  unsigned int type;
  bool normalized;
  int stride;
  void* pointer;
};

struct VertexArrayCreateInfo {
  std::vector<VertexAttributeCreateInfo> attributes;
  std::vector<std::pair<unsigned int, unsigned int>> buffers;
};

template <typename T>
struct BufferCreateInfo {
  unsigned int type; // GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER, etc.
  unsigned int usage; // GL_STATIC_DRAW, GL_DYNAMIC_DRAW, etc.
  size_t size; // Size in bytes
  const T* data; // Data
};

struct TextureCreateInfo {
  int width;
  int height;
  int channels;
  unsigned char* data;
};

extern std::vector<std::shared_ptr<Framebuffer>> all_framebuffers;
extern std::vector<std::pair<Transform, TexturePack>> sprite_objects;
extern std::vector<std::pair<Transform, Light>> lights;
extern std::vector<unsigned int> loaded_textures;
extern std::vector<unsigned int> loaded_vertex_arrays;
extern std::vector<unsigned int> loaded_buffers;

unsigned int CreateVertexArrayObject(VertexArrayCreateInfo info);
template <typename T> unsigned int CreateBufferObject(BufferCreateInfo<T> info) {
  unsigned int buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(info.type, buffer);
  glBufferData(info.type, info.size, info.data, info.usage);
  loaded_buffers.push_back(buffer);
  return buffer;
}
unsigned int CreateTextureObject(TextureCreateInfo info);
unsigned int LoadShaderProgram(std::vector<std::pair<unsigned int, std::string>> shader_paths);
std::shared_ptr<Framebuffer> CreateFramebuffer(GLFWwindow* window);
