#include <glad/glad.h>
// CODE BLOCK: To stop clang from messing with my include
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <print>
#include <array>
#include <filesystem>
#include <string>
#include <format>
#include <tinyfiledialogs/tinyfiledialogs.h>
#include "helpers.h"
#include "core.h"
#include "scene.h"

unsigned int LoadTexture(std::string path);

void GetInspector(AttributeData& data) {
  std::visit([&](auto& v) {
    using T = std::decay_t<decltype(v)>;
    if constexpr (std::is_same_v<T, int>) {
      ImGui::InputInt("Value", &v);
    } else if constexpr (std::is_same_v<T, float>) {
      ImGui::InputFloat("Value", &v);
    } else if constexpr (std::is_same_v<T, glm::vec2>) {
      ImGui::InputFloat2("Value", glm::value_ptr(v));
    } else if constexpr (std::is_same_v<T, glm::vec3>) {
      ImGui::InputFloat3("Value", glm::value_ptr(v));
    } else if constexpr (std::is_same_v<T, unsigned int>) {
    ImGui::Image(v, ImVec2(64, 64));
    if (ImGui::Button("Change Texture")) {
      const char* filters[] = { "*.png", "*.jpg", "*.jpeg", "*.bmp" };
      auto path = tinyfd_openFileDialog("Select Texture", "", 4, filters, "Image Files", 0);
      if (path) {
        try {
          v = LoadTexture(std::string(path));
        } catch (const std::runtime_error& e) {
          std::print("Error loading texture: {}\n", e.what());
        }
      }
    }
    } else {
      ImGui::Text("Unknown Attribute Type");
    }
  }, data);
}

constexpr glm::ivec2 default_window_size = { 800, 600 };
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
Scene scene;

void FramebufferResizeCallback(GLFWwindow* window, int w, int h) {
  for (auto& framebuffer : all_framebuffers) {
    framebuffer->size = glm::ivec2(std::max(1, (int)(w * framebuffer->scale)),
                                   std::max(1, (int)(h * framebuffer->scale)));
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
    glBindTexture(GL_TEXTURE_2D, framebuffer->colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, framebuffer->size.x, framebuffer->size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void PresentGlfwErrorInfo() {
  const char* error_desc;
  auto error = glfwGetError(&error_desc);
  if (error != GLFW_NO_ERROR)
    std::print("Error Description: {}\n", error_desc ? error_desc : "No Description Provided, gg.");
}

void SetLightUniforms(std::vector<std::shared_ptr<Object>>& lights, unsigned int shader) {
  auto light_objects = std::vector<std::shared_ptr<Object>>{};
  for (auto& object : lights) {
    if (object->HasTag("light")) {
      light_objects.push_back(object);
    }
  }
  glUniform1i(glGetUniformLocation(shader, "light_count"), light_objects.size());
  for (int i = 0; i < light_objects.size(); i++) {
    auto type = std::get<int>(light_objects[i]->GetAttribute("light.type"));
    auto position = std::get<glm::vec3>(light_objects[i]->GetAttribute("transform.position"));
    auto intensity = std::get<float>(light_objects[i]->GetAttribute("light.intensity"));
    auto color = std::get<glm::vec3>(light_objects[i]->GetAttribute("light.color"));
    auto falloff = std::get<float>(light_objects[i]->GetAttribute("light.radial_falloff"));
    auto volumetric_intensity = std::get<float>(light_objects[i]->GetAttribute("light.volumetric_intensity"));
    auto prefix = std::format("lights[{}].", i);
    glUniform1i(glGetUniformLocation(shader, (prefix + "type").c_str()), type);
    glUniform3fv(glGetUniformLocation(shader, (prefix + "position").c_str()), 1, glm::value_ptr(position));
    glUniform1f(glGetUniformLocation(shader, (prefix + "intensity").c_str()), intensity);
    glUniform3fv(glGetUniformLocation(shader, (prefix + "color").c_str()), 1, glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(shader, (prefix + "falloff").c_str()), falloff);
    glUniform1f(glGetUniformLocation(shader, (prefix + "volumetric_intensity").c_str()), volumetric_intensity);
  }
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
  core::Initialize(core::InitializeFlags::OPENGL, nullptr);
  auto window = glfwCreateWindow(default_window_size.x, default_window_size.y, "Vibrant", NULL, NULL);
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
  glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
  core::Initialize(core::InitializeFlags::IMGUI, window);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  auto color_buffer = CreateFramebuffer(window, 0.1f);
  auto normal_buffer = CreateFramebuffer(window, 0.1f);
  auto deferred_buffer = CreateFramebuffer(window, 0.1f);
  auto sprite_shader = LoadShaderProgram({
      {GL_VERTEX_SHADER, "assets/sprite_vertex.glsl"},
      {GL_FRAGMENT_SHADER, "assets/sprite_fragment.glsl"}
  });
  auto deferred_shader = LoadShaderProgram({
    {GL_VERTEX_SHADER, "assets/deferred_vertex.glsl"},
    {GL_FRAGMENT_SHADER, "assets/deferred_fragment.glsl"}
  });

  auto sprite_uniform_buffer_create_info = BufferCreateInfo<float>{
    GL_UNIFORM_BUFFER,
    GL_STATIC_DRAW,
    2 * sizeof(glm::mat4),
    NULL
  };
  auto sprite_uniform_buffer = CreateBufferObject(sprite_uniform_buffer_create_info);
  auto uniform_block_index = glGetUniformBlockIndex(sprite_shader, "Matrices");
  glUniformBlockBinding(sprite_shader, uniform_block_index, 0);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, sprite_uniform_buffer, 0, 2 * sizeof(glm::mat4));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  auto sprite_vertices_create_info = BufferCreateInfo<float>{
    GL_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    sprite_vertices.size() * sizeof(float),
    sprite_vertices.data()
  };
  auto sprite_indices_create_info = BufferCreateInfo<unsigned int>{
    GL_ELEMENT_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    shared_indices.size() * sizeof(unsigned int),
    shared_indices.data()
  };
  auto sprite_vertex_buffer = CreateBufferObject(sprite_vertices_create_info);
  auto sprite_indices_buffer = CreateBufferObject(sprite_indices_create_info);
  auto sprite_vertex_array_create_info = VertexArrayCreateInfo{
    {
      {0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0},
      {1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float))}
    },
    {
      {GL_ARRAY_BUFFER, sprite_vertex_buffer},
      {GL_ELEMENT_ARRAY_BUFFER, sprite_indices_buffer},
    }
  };
  auto sprite_vertex_array = CreateVertexArrayObject(sprite_vertex_array_create_info);
  glBindVertexArray(0);
  
  auto deferred_vertices_create_info = BufferCreateInfo<float>{
    GL_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    deferred_vertices.size() * sizeof(float),
    deferred_vertices.data()
  };
  auto deferred_indices_create_info = BufferCreateInfo<unsigned int>{
    GL_ELEMENT_ARRAY_BUFFER,
    GL_STATIC_DRAW,
    shared_indices.size() * sizeof(unsigned int),
    shared_indices.data()
  };
  auto deferred_vertex_buffer = CreateBufferObject(deferred_vertices_create_info);
  auto deferred_indices_buffer = CreateBufferObject(deferred_indices_create_info);
  auto deferred_vertex_array_create_info = VertexArrayCreateInfo{
    {
      {0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0},
      {1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float))}
    },
    {
      {GL_ARRAY_BUFFER, deferred_vertex_buffer},
      {GL_ELEMENT_ARRAY_BUFFER, deferred_indices_buffer}
    }
  };
  auto deferred_vertex_attrib = CreateVertexArrayObject(deferred_vertex_array_create_info);
  glBindVertexArray(0);

  auto combine_shader = LoadShaderProgram({
    {GL_VERTEX_SHADER, "assets/deferred_vertex.glsl"},
    {GL_FRAGMENT_SHADER, "assets/combine_fragment.glsl"}
  });

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
    glBindBuffer(GL_UNIFORM_BUFFER, sprite_uniform_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, color_buffer->id);
    glViewport(0, 0, color_buffer->size.x, color_buffer->size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& object : scene.objects) {
      if (!object->HasTag("sprite")) continue;
      auto position = std::get<glm::vec3>(object->GetAttribute("transform.position"));
      auto scale = std::get<glm::vec3>(object->GetAttribute("transform.scale"));
      auto rotation = std::get<float>(object->GetAttribute("transform.rotation"));
      unsigned int texture = std::get<unsigned int>(object->GetAttribute("texture.color"));
      model = glm::translate(glm::mat4(1.0f), position);
      model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, normal_buffer->id);
    glViewport(0, 0, normal_buffer->size.x, normal_buffer->size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& object : scene.objects) {
      if (!object->HasTag("sprite")) continue;
      auto position = std::get<glm::vec3>(object->GetAttribute("transform.position"));
      auto scale = std::get<glm::vec3>(object->GetAttribute("transform.scale"));
      auto rotation = std::get<float>(object->GetAttribute("transform.rotation"));
      unsigned int texture = std::get<unsigned int>(object->GetAttribute("texture.normal"));
      model = glm::translate(glm::mat4(1.0f), position);
      model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
      model = glm::scale(model, scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, deferred_buffer->id);
    glViewport(0, 0, deferred_buffer->size.x, deferred_buffer->size.y);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(deferred_shader);
    SetLightUniforms(scene.objects, deferred_shader);
    glUniform1i(glGetUniformLocation(deferred_shader, "color_buffer"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_buffer->colorbuffer);
    glUniform1i(glGetUniformLocation(deferred_shader, "normal_buffer"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal_buffer->colorbuffer);
    glBindVertexArray(deferred_vertex_attrib);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(combine_shader);
    glUniform1i(glGetUniformLocation(combine_shader, "deferred_buffer"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deferred_buffer->colorbuffer);
    glBindVertexArray(deferred_vertex_attrib);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    {
      ImGui::Begin("Scene Hierarchy");
      if (ImGui::Button("New Object")) {
        auto object = std::make_shared<Object>();
        object->SetAttribute("transform.position", glm::vec3(0.0f, 0.0f, 0.0f));
        object->SetAttribute("transform.scale", glm::vec3(1.0f, 1.0f, 1.0f));
        object->SetAttribute("transform.rotation", float(0.0f));
        scene.objects.push_back(object);
      }
      for (auto& object : scene.objects) {
        ImGui::PushID(object.get());
        ImGui::Text("Object");
        ImGui::SeparatorText("Tags");
        for (auto& tag : object->tags) {
          ImGui::InputText("Tag", &tag);
        }
        if (ImGui::Button("Add Tag")) {
          object->tags.push_back("guten tag");
        }
        ImGui::SeparatorText("Attributes");
        for (auto& attr : object->attributes) {
          ImGui::PushID(&attr);
          ImGui::InputText("Attribute Name", &attr.first);
          GetInspector(attr.second);
          ImGui::PopID();
        }
        if (ImGui::Button("Add Attribute")) {
          ImGui::OpenPopup("Select Attribute Type");
        }
        if (ImGui::BeginPopupModal("Select Attribute Type")) {
            static int attr_type = 0;
            ImGui::RadioButton("int", &attr_type, 0);
            ImGui::RadioButton("float", &attr_type, 1);
            ImGui::RadioButton("vec2", &attr_type, 2);
            ImGui::RadioButton("vec3", &attr_type, 3);
            ImGui::RadioButton("texture", &attr_type, 4);
            if (ImGui::Button("OK")) {
              switch (attr_type) {
                case 0:
                  object->SetAttribute("new attribute", int(0));
                  break;
                case 1:
                  object->SetAttribute("new attribute", float(0.0f));
                  break;
                case 2:
                  object->SetAttribute("new attribute", glm::vec2(0.0f, 0.0f));
                  break;
                case 3:
                  object->SetAttribute("new attribute", glm::vec3(0.0f, 0.0f, 0.0f));
                  break;
                case 4:
                  object->SetAttribute("new attribute", (unsigned int)(0));
                  break;
                default:
                  break;
              }
              ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
          }
        ImGui::PopID();
      }

      ImGui::Image(color_buffer->colorbuffer, ImVec2(256, 256));
      ImGui::SameLine();
      ImGui::Image(normal_buffer->colorbuffer, ImVec2(256, 256));
      ImGui::SameLine();
      ImGui::Image(deferred_buffer->colorbuffer, ImVec2(256, 256));
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
  glDeleteProgram(combine_shader);
  glDeleteTextures(loaded_textures.size(), loaded_textures.data());
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
