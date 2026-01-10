#include <glad/glad.h>
// CODE BLOCK: To stop clang from messing with my include
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <print>
#include <array>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <format>
#include "light.h"

struct Transform {
  glm::vec3 position;
  glm::vec3 scale;
  float rotation;
};

struct TexturePack {
  unsigned int color, normal;
};

struct Framebuffer {
  unsigned int id;
  unsigned int colorbuffer;
};

constexpr glm::ivec2 default_window_size = { 800, 600 };
constexpr int log_size = 512;
constexpr std::array<float, 20> sprite_vertices = {
     0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
};
constexpr std::array<float, 16> deferred_vertices = {
     1.0f, 1.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     -1.0f, -1.0f, 0.0f, 0.0f,
     -1.0f, 1.0f, 0.0f, 1.0f
};
constexpr std::array<unsigned int, 6> shared_indices = {
    0, 1, 3,
    1, 2, 3
};
std::vector<std::shared_ptr<Framebuffer>> all_framebuffers;
std::vector<std::pair<Transform, TexturePack>> sprite_objects;
std::vector<std::pair<Transform, Light>> lights;
std::vector<unsigned int> loaded_textures;
std::vector<unsigned int> loaded_vertex_arrays;
std::vector<unsigned int> loaded_buffers;

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

unsigned int LoadShaderProgram(std::string vertex_path, std::string fragment_path) {
  auto vertex = CompileShader(GL_VERTEX_SHADER, ReadFile(vertex_path)),
       fragment = CompileShader(GL_FRAGMENT_SHADER, ReadFile(fragment_path));
  auto shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  int success;
  std::array<char, log_size> log{};
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(shader, log_size, nullptr, log.data());
    throw std::runtime_error("Shader program link failed: " + std::string(log.data()));
  }
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  return shader;
}

std::shared_ptr<Framebuffer> CreateFramebuffer(GLFWwindow* window);
// This function won't store the value of new window size as a global variable
void WindowResizeCallback(GLFWwindow* window, int w, int h) {
  glViewport(0, 0, w, h);
  // Reconfigure framebuffers
  for (auto& framebuffer : all_framebuffers) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
    glBindTexture(GL_TEXTURE_2D, framebuffer->colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

// Presents error, if any
void PresentGlfwErrorInfo() {
  const char* error_desc;
  auto error = glfwGetError(&error_desc);
  if (error != GLFW_NO_ERROR)
    std::print("Error Description: {}\n", error_desc ? error_desc : "No Description Provided, gg.");
}

void SetLightUniforms(std::vector<std::pair<Transform, Light>>& lights, unsigned int shader) {
  glUniform1i(glGetUniformLocation(shader, "light_count"), lights.size());
  for (int i = 0; i < lights.size(); i++) {
    glUniform1i(glGetUniformLocation(shader, std::format("lights[{}].type", i).c_str()), (int)lights.at(i).second.type);
    glUniform2fv(glGetUniformLocation(shader, std::format("lights[{}].position", i).c_str()), 1, glm::value_ptr(lights.at(i).first.position));
    glUniform1f(glGetUniformLocation(shader, std::format("lights[{}].intensity", i).c_str()), lights.at(i).second.intensity);
    glUniform3fv(glGetUniformLocation(shader, std::format("lights[{}].color", i).c_str()), 1, glm::value_ptr(lights.at(i).second.color));
    glUniform1f(glGetUniformLocation(shader, std::format("lights[{}].falloff", i).c_str()), lights.at(i).second.radial_falloff);
    glUniform1f(glGetUniformLocation(shader, std::format("lights[{}].volumetric_intensity", i).c_str()), lights.at(i).second.volumetric_intensity);
  }
}

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
  bool has_vertex_buffer = true;
  unsigned int vertex_buffer;
  bool has_index_buffer = false;
  unsigned int index_buffer;
};

unsigned int CreateVertexArrayObject(VertexArrayCreateInfo info) {
  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  if (info.has_vertex_buffer) {
    glBindBuffer(GL_ARRAY_BUFFER, info.vertex_buffer);
  }
  if (info.has_index_buffer) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, info.index_buffer);
  }
  for (auto& attrib : info.attributes) {
    glVertexAttribPointer(attrib.index, attrib.size, attrib.type, attrib.normalized,
                          attrib.stride, attrib.pointer);
    glEnableVertexAttribArray(attrib.index);
  }
  loaded_vertex_arrays.push_back(vao);
  return vao;
}

template <typename T>
struct BufferCreateInfo {
  unsigned int type; // GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, etc.
  unsigned int usage; // GL_STATIC_DRAW, GL_DYNAMIC_DRAW, etc.
  std::vector<T> data; // Data
};

template <typename T>
unsigned int CreateBufferObject(BufferCreateInfo<T> info) {
  unsigned int buffer;
  glGenBuffers(1, &buffer);
  glBindBuffer(info.type, buffer);
  glBufferData(info.type, info.data.size() * sizeof(T), info.data.data(),
                info.usage);
  loaded_buffers.push_back(buffer);
  return buffer;
}

struct TextureCreateInfo {
  int width;
  int height;
  int channels;
  unsigned char* data;
};

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

std::shared_ptr<Framebuffer> CreateFramebuffer(GLFWwindow* window) {
  std::shared_ptr<Framebuffer> framebuffer = std::make_shared<Framebuffer>();
  glGenFramebuffers(1, &framebuffer->id);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  auto create_info = TextureCreateInfo{ width, height, 3, nullptr };
  framebuffer->colorbuffer = CreateTextureObject(create_info);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer->colorbuffer, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    throw std::runtime_error("Framebuffer is not complete...");
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  all_framebuffers.push_back(framebuffer);
  return framebuffer;
}

unsigned int LoadTexture(std::string path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("Path does not exist");
  }
  int w, h, nr_channels;
  auto data = stbi_load(path.c_str(), &w, &h, &nr_channels, 0);
  if (data) {
    auto texture = CreateTextureObject({ w, h, nr_channels, data });
    stbi_image_free(data);
    return texture;
  } else {
    stbi_image_free(data);
    throw std::runtime_error("Failed to load texture");
  }
}

int main(int argc, char *argv[]) {
  // Initializing stuff, move to seperate function sometime later
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Make the window really small first, then resize it to normal size
  // This ensures GLFW calls the resize callbacks
  auto window = glfwCreateWindow(1, 1, "Vibrant", NULL, NULL);
  if (window == NULL) {
    std::print("Something wrong happened with the window... :(\n");
    PresentGlfwErrorInfo();
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::print("Something wrong happened with glad :( (Trying again probably won't fix this)\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwSetWindowSizeCallback(window, WindowResizeCallback);
  glfwSetWindowSize(window, default_window_size.x, default_window_size.y);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  // Setting up GL objects
  auto color_buffer = CreateFramebuffer(window), normal_buffer = CreateFramebuffer(window);
  auto sprite_shader = LoadShaderProgram("assets/sprite.vert", "assets/sprite.frag"),
       deferred_shader = LoadShaderProgram("assets/deferred.vert", "assets/deferred.frag");

  auto sprite_vertices_create_info = BufferCreateInfo<float>{
    GL_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    std::vector<float>(sprite_vertices.begin(), sprite_vertices.end())
  };
  auto sprite_indices_create_info = BufferCreateInfo<unsigned int>{
    GL_ELEMENT_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    std::vector<unsigned int>(shared_indices.begin(), shared_indices.end())
  };
  auto sprite_vertex_buffer = CreateBufferObject(sprite_vertices_create_info);
  auto sprite_indices_buffer = CreateBufferObject(sprite_indices_create_info);

  auto sprite_vertex_array_create_info = VertexArrayCreateInfo{
    {
      {0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0},
      {1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))}
    },
    true,
    sprite_vertex_buffer,
    true,
    sprite_indices_buffer
  };
  auto sprite_vertex_array = CreateVertexArrayObject(sprite_vertex_array_create_info);
  glBindVertexArray(0);

  auto deferred_vertices_create_info = BufferCreateInfo<float>{
    GL_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    std::vector<float>(deferred_vertices.begin(), deferred_vertices.end())
  };
  auto deferred_indices_create_info = BufferCreateInfo<unsigned int>{
    GL_ELEMENT_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    std::vector<unsigned int>(shared_indices.begin(), shared_indices.end())
  };
  auto deferred_vertex_buffer = CreateBufferObject(deferred_vertices_create_info);
  auto deferred_indices_buffer = CreateBufferObject(deferred_indices_create_info);

  auto deferred_vertex_array_create_info = VertexArrayCreateInfo{
    {
      {0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0},
      {1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))}
    },
    true,
    deferred_vertex_buffer,
    true,
    deferred_indices_buffer
  };
  auto deferred_vertex_attrib = CreateVertexArrayObject(deferred_vertex_array_create_info);
  glBindVertexArray(0);


  // Main loop
  // Again, gut this part out of main sometime.
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    auto model = glm::mat4(1.0f), view = glm::mat4(1.0f), projection = glm::mat4(1.0f);
    float ortho_scale = 10.0f;
    int window_width, window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);
    float aspect = static_cast<float>(window_width) / static_cast<float>(window_height);
    projection = glm::ortho(
    -ortho_scale * aspect,
     ortho_scale * aspect,
    -ortho_scale,
     ortho_scale,
    -1.0f,
     1.0f
);

    // Draw Color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, color_buffer->id);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& [transform, texture_pack] : sprite_objects) {
      model = glm::translate(glm::mat4(1.0f), transform.position);
      model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, transform.scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_pack.color);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Draw Normal buffer
    glBindFramebuffer(GL_FRAMEBUFFER, normal_buffer->id);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& [transform, texture_pack ] : sprite_objects) {
      model = glm::translate(glm::mat4(1.0f), transform.position);
      model = glm::rotate(model, glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, transform.scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_pack.normal);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    // Draw final buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(deferred_shader);
    SetLightUniforms(lights, deferred_shader);
    glUniform1i(glGetUniformLocation(deferred_shader, "color_buffer"), 0);
    glUniform1i(glGetUniformLocation(deferred_shader, "normal_buffer"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_buffer->colorbuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_buffer->colorbuffer);
    glBindVertexArray(deferred_vertex_attrib);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    {
      ImGui::Begin("Inspector");
      if (ImGui::Button("New Light")) {
        lights.push_back({});
      }
      ImGui::SameLine();
      if (ImGui::Button("New Sprite")) {
        sprite_objects.push_back({});
      }
      int light_counter = 0;
      for (auto& [transform, light] : lights) {
        light_counter++;
        ImGui::PushID(light_counter);
        ImGui::SeparatorText("Light Object");
        int type = static_cast<int>(light.type);
        ImGui::InputInt("Light Type (0: Global, 1: Point)", &type);
        light.type = static_cast<LightType>(type);
        ImGui::DragFloat3("Light Position", glm::value_ptr(transform.position), 0.1f);
        ImGui::DragFloat("Intensity", &light.intensity, 0.1f);
        ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
        ImGui::DragFloat("Falloff", &light.radial_falloff, 0.1f);
        ImGui::DragFloat("Volumetric Intensity", &light.volumetric_intensity, 0.1f);
        ImGui::PopID();
      }
      int sprite_counter = 0;
      for (auto& [transform, texture_pack] : sprite_objects) {
        sprite_counter++;
        ImGui::PushID(sprite_counter);
        ImGui::SeparatorText("Sprite Object");
        ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1f);
        ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
        ImGui::DragFloat("Rotation", &transform.rotation, 0.1f);
        ImGui::Image((ImTextureID)(intptr_t)texture_pack.color, ImVec2(50, 50));
        ImGui::SameLine();
        ImGui::Image((ImTextureID)(intptr_t)texture_pack.normal, ImVec2(50, 50));
        if (ImGui::Button("Load")) {
          texture_pack.color = LoadTexture("assets/color.png");
          texture_pack.normal = LoadTexture("assets/normal.png");
        }
        ImGui::PopID();
      }
      ImGui::Image((ImTextureID)(intptr_t)color_buffer->colorbuffer, ImVec2(200, 200));
      ImGui::Image((ImTextureID)(intptr_t)normal_buffer->colorbuffer, ImVec2(200, 200));
      ImGui::End();
    }

    ImGui::Render();
    ImGui::EndFrame();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(loaded_vertex_arrays.size(), loaded_vertex_arrays.data());
  glDeleteBuffers(loaded_buffers.size(), loaded_buffers.data());
  glDeleteProgram(sprite_shader);
  glDeleteProgram(deferred_shader);
  glDeleteTextures(loaded_textures.size(), loaded_textures.data());
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
