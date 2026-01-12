#include <glad/glad.h>
// CODE BLOCK: To stop clang from messing with my include
#include "helpers.h"
#include <utility>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <array>
#include <stdexcept>

constexpr int log_size = 512;
std::vector<std::shared_ptr<Framebuffer>> all_framebuffers;
std::vector<unsigned int> loaded_textures;
std::vector<unsigned int> loaded_vertex_arrays;
std::vector<unsigned int> loaded_buffers;

// Shader-related functions

std::string ReadFile(std::filesystem::path path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + path.string());
  }
  std::stringstream stream;
  stream << file.rdbuf();
  file.close();
  return stream.str();
}

unsigned int CompileShader(int shader_type, std::string source) {
  auto shader = glCreateShader(shader_type);
  const char* source_data = source.data();
  glShaderSource(shader, 1, &source_data, nullptr);
  glCompileShader(shader);
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    std::array<char, log_size> log{};
    glGetShaderInfoLog(shader, log_size, nullptr, log.data());
    throw std::runtime_error("Shader compilation failed: " + std::string(log.data()));
  }
  return shader;
}

unsigned int LoadShaderProgram(std::vector<std::pair<unsigned int, std::string>> shader_paths) {
  auto shader = glCreateProgram();
  for (auto& [shader_type, path] : shader_paths) {
    auto compiled_shader = CompileShader(shader_type, ReadFile(path));
    glAttachShader(shader, compiled_shader);
    glDeleteShader(compiled_shader);
  }
  glLinkProgram(shader);
  int success;
  std::array<char, log_size> log{};
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(shader, log_size, nullptr, log.data());
    throw std::runtime_error("Shader program link failed: " + std::string(log.data()));
  }
  return shader;
}

// Framebuffer-related functions
std::shared_ptr<Framebuffer> CreateFramebuffer(GLFWwindow* window, float scale) {
  std::shared_ptr<Framebuffer> framebuffer = std::make_shared<Framebuffer>();
  framebuffer->scale = scale;
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  framebuffer->size = glm::ivec2(std::max(1, (int)(width * scale)),
                                 std::max(1, (int)(height * scale)));
  glGenFramebuffers(1, &framebuffer->id);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
  
  auto create_info = TextureCreateInfo{ framebuffer->size.x, framebuffer->size.y, 3, nullptr };
  framebuffer->colorbuffer = CreateTextureObject(create_info);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorbuffer, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    throw std::runtime_error("Framebuffer is not complete...");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  all_framebuffers.push_back(framebuffer);
  return framebuffer;
}

// Buffer and Texture-related functions
unsigned int CreateVertexArrayObject(VertexArrayCreateInfo info) {
  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  for (auto& [type, buffer] : info.buffers) {
    glBindBuffer(type, buffer);
  }
  for (auto& attrib : info.attributes) {
    glVertexAttribPointer(attrib.index, attrib.size, attrib.type, attrib.normalized,
                          attrib.stride, attrib.pointer);
    glEnableVertexAttribArray(attrib.index);
  }
  loaded_vertex_arrays.push_back(vao);
  return vao;
}



unsigned int CreateTextureObject(TextureCreateInfo info) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  GLenum format;
  switch (info.channels) {
    case 1:
      format = GL_RED;
      break;
    case 2:
      format = GL_RG;
      break;
    case 3:
      format = GL_RGB;
      break;
    case 4:
      format = GL_RGBA;
      break;
    default:
      format = GL_RGB;
      break;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, format, info.width, info.height, 0, format, GL_UNSIGNED_BYTE, info.data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  loaded_textures.push_back(texture);
  return texture;
}
