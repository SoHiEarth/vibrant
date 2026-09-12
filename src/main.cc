#include <glad/glad.h>
// CODE BLOCK: To stop clang from messing with my include
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <tinyfiledialogs/tinyfiledialogs.h>

#include <array>
#include <filesystem>
#include <format>
#include <print>
#include <string>

#include "core.h"
#include "helpers.h"
#include "scene.h"

namespace {
constexpr glm::ivec2 kDefaultWindowSize = {800, 600};
constexpr std::array<float, 20> kSpriteVertices = {
    0.5F,  0.5F,  0.0F, 1.0F, 1.0F, 0.5F,  -0.5F, 0.0F, 1.0F, 0.0F,
    -0.5F, -0.5F, 0.0F, 0.0F, 0.0F, -0.5F, 0.5F,  0.0F, 0.0F, 1.0F};
constexpr std::array<float, 16> kDeferredVertices = {
    1.0F,  1.0F,  1.0F, 1.0F, 1.0F,  -1.0F, 1.0F, 0.0F,
    -1.0F, -1.0F, 0.0F, 0.0F, -1.0F, 1.0F,  0.0F, 1.0F};
constexpr std::array<unsigned int, 6> kSharedIndices = {0, 1, 3, 1, 2, 3};
Scene scene;
glm::vec3 clear_color = {0.1F, 0.1F, 0.1F};
std::vector<AttributeTemplate> attribute_templates;
bool show_demo_window = false;
bool show_template_window = false;
bool show_edit_window = true;

unsigned int LoadTexture(const std::string& path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("Path does not exist");
  }
  int w;
  int h;
  int nr_channels;
  auto* data = stbi_load(path.c_str(), &w, &h, &nr_channels, 0);
  if (data) {
    auto texture = CreateTextureObject(
        {.width = w, .height = h, .channels = nr_channels, .data = data});
    stbi_image_free(data);
    return texture;
  }
  stbi_image_free(data);
  throw std::runtime_error("Failed to load texture");
}

void GetInspector(AttributeData& data) {
  std::visit(
      [&](auto& v) {
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
            const char* filters[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
            auto* path = tinyfd_openFileDialog("Select Texture", "", 4, filters,
                                               "Image Files", 0);
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
      },
      data);
}

void FramebufferResizeCallback(GLFWwindow* /*window*/, int w, int h) {
  for (auto& framebuffer : all_framebuffers) {
    framebuffer->size =
        glm::ivec2(std::max(1, static_cast<int>(w * framebuffer->scale)),
                   std::max(1, static_cast<int>(h * framebuffer->scale)));
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
    glBindTexture(GL_TEXTURE_2D, framebuffer->colorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, framebuffer->size.x,
                 framebuffer->size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void PresentGlfwErrorInfo() {
  const char* error_desc;
  auto error = glfwGetError(&error_desc);
  if (error != GLFW_NO_ERROR)
    std::print("Error Description: {}\n",
               error_desc ? error_desc : "No Description Provided, gg.");
}

void SetLightUniforms(std::vector<std::shared_ptr<Object>>& lights,
                             unsigned int shader) {
  auto light_objects = std::vector<std::shared_ptr<Object>>{};
  for (auto& object : lights) {
    if (object->HasTag("light")) {
      light_objects.push_back(object);
    }
  }
  glUniform1i(glGetUniformLocation(shader, "light_count"),
              light_objects.size());
  for (int i = 0; i < light_objects.size(); i++) {
    auto type = std::get<int>(light_objects[i]->GetAttribute("light.type"));
    auto position = std::get<glm::vec3>(
        light_objects[i]->GetAttribute("transform.position"));
    auto intensity =
        std::get<float>(light_objects[i]->GetAttribute("light.intensity"));
    auto color =
        std::get<glm::vec3>(light_objects[i]->GetAttribute("light.color"));
    auto falloff =
        std::get<float>(light_objects[i]->GetAttribute("light.radial_falloff"));
    auto volumetric_intensity = std::get<float>(
        light_objects[i]->GetAttribute("light.volumetric_intensity"));
    auto prefix = std::format("lights[{}].", i);
    glUniform1i(glGetUniformLocation(shader, (prefix + "type").c_str()), type);
    glUniform3fv(glGetUniformLocation(shader, (prefix + "position").c_str()), 1,
                 glm::value_ptr(position));
    glUniform1f(glGetUniformLocation(shader, (prefix + "intensity").c_str()),
                intensity);
    glUniform3fv(glGetUniformLocation(shader, (prefix + "color").c_str()), 1,
                 glm::value_ptr(color));
    glUniform1f(glGetUniformLocation(shader, (prefix + "falloff").c_str()),
                falloff);
    glUniform1f(
        glGetUniformLocation(shader, (prefix + "volumetric_intensity").c_str()),
        volumetric_intensity);
  }
}
}

int main(int /*argc*/, char* /*argv*/[]) {
  core::Initialize(core::InitializeFlags::kOpengl, nullptr);
  auto* window = glfwCreateWindow(kDefaultWindowSize.x, kDefaultWindowSize.y,
                                  "Vibrant", nullptr, nullptr);
  if (window == nullptr) {
    std::print("Something wrong happened with the window... :(\n");
    PresentGlfwErrorInfo();
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
    std::print(
        "Something wrong happened with glad :( (Trying again probably won't "
        "fix this)\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }
  glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
  core::Initialize(core::InitializeFlags::kImgui, window);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  auto color_buffer = CreateFramebuffer(window, 0.1F);
  auto normal_buffer = CreateFramebuffer(window, 0.1F);
  auto deferred_buffer = CreateFramebuffer(window, 0.1F);
  auto sprite_shader =
      LoadShaderProgram({{GL_VERTEX_SHADER, "assets/sprite_vertex.glsl"},
                         {GL_FRAGMENT_SHADER, "assets/sprite_fragment.glsl"}});
  auto deferred_shader = LoadShaderProgram(
      {{GL_VERTEX_SHADER, "assets/deferred_vertex.glsl"},
       {GL_FRAGMENT_SHADER, "assets/deferred_fragment.glsl"}});

  auto sprite_uniform_buffer_create_info =
      BufferCreateInfo<float>{.type = GL_UNIFORM_BUFFER,
                              .usage = GL_STATIC_DRAW,
                              .size = 2 * sizeof(glm::mat4),
                              .data = NULL};
  auto sprite_uniform_buffer =
      CreateBufferObject(sprite_uniform_buffer_create_info);
  auto uniform_block_index = glGetUniformBlockIndex(sprite_shader, "Matrices");
  glUniformBlockBinding(sprite_shader, uniform_block_index, 0);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, sprite_uniform_buffer, 0,
                    2 * sizeof(glm::mat4));
  glBindBuffer(GL_UNIFORM_BUFFER, 0);

  auto sprite_vertices_create_info =
      BufferCreateInfo<float>{.type = GL_ARRAY_BUFFER,
                              .usage = GL_STATIC_DRAW,
                              .size = kSpriteVertices.size() * sizeof(float),
                              .data = kSpriteVertices.data()};
  auto sprite_indices_create_info = BufferCreateInfo<unsigned int>{
      .type = GL_ELEMENT_ARRAY_BUFFER,
      .usage = GL_STATIC_DRAW,
      .size = kSharedIndices.size() * sizeof(unsigned int),
      .data = kSharedIndices.data()};
  auto sprite_vertex_buffer = CreateBufferObject(sprite_vertices_create_info);
  auto sprite_indices_buffer = CreateBufferObject(sprite_indices_create_info);
  auto sprite_vertex_array_create_info = VertexArrayCreateInfo{
      .attributes = {{0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                      static_cast<void*>(nullptr)},
                     {1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                      (void*)(3 * sizeof(float))}},
      .buffers = {
          {GL_ARRAY_BUFFER, sprite_vertex_buffer},
          {GL_ELEMENT_ARRAY_BUFFER, sprite_indices_buffer},
      }};
  auto sprite_vertex_array =
      CreateVertexArrayObject(sprite_vertex_array_create_info);
  glBindVertexArray(0);

  auto deferred_vertices_create_info =
      BufferCreateInfo<float>{.type = GL_ARRAY_BUFFER,
                              .usage = GL_STATIC_DRAW,
                              .size = kDeferredVertices.size() * sizeof(float),
                              .data = kDeferredVertices.data()};
  auto deferred_indices_create_info = BufferCreateInfo<unsigned int>{
      .type = GL_ELEMENT_ARRAY_BUFFER,
      .usage = GL_STATIC_DRAW,
      .size = kSharedIndices.size() * sizeof(unsigned int),
      .data = kSharedIndices.data()};
  auto deferred_vertex_buffer =
      CreateBufferObject(deferred_vertices_create_info);
  auto deferred_indices_buffer =
      CreateBufferObject(deferred_indices_create_info);
  auto deferred_vertex_array_create_info = VertexArrayCreateInfo{
      .attributes = {{0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                      (void*)nullptr},
                     {1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                      (void*)(2 * sizeof(float))}},
      .buffers = {{GL_ARRAY_BUFFER, deferred_vertex_buffer},
                  {GL_ELEMENT_ARRAY_BUFFER, deferred_indices_buffer}}};
  auto deferred_vertex_attrib =
      CreateVertexArrayObject(deferred_vertex_array_create_info);
  glBindVertexArray(0);

  auto combine_shader =
      LoadShaderProgram({{GL_VERTEX_SHADER, "assets/deferred_vertex.glsl"},
                         {GL_FRAGMENT_SHADER, "assets/combine_fragment.glsl"}});

  if (std::filesystem::exists("attributes.xml")) {
    attribute_templates = attributes::LoadTemplates();
  }

  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    auto model = glm::mat4(1.0F);
    auto view = glm::mat4(1.0F);
    auto projection = glm::mat4(1.0F);
    float ortho_scale = 10.0F;
    int window_width;
    int window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);
    float aspect =
        static_cast<float>(window_width) / static_cast<float>(window_height);
    projection = glm::ortho(-ortho_scale * aspect, ortho_scale * aspect,
                            -ortho_scale, ortho_scale, -1.0F, 1.0F);
    glBindBuffer(GL_UNIFORM_BUFFER, sprite_uniform_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4),
                    glm::value_ptr(projection));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                    glm::value_ptr(view));
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, color_buffer->id);
    glViewport(0, 0, color_buffer->size.x, color_buffer->size.y);
    glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& object : scene.objects) {
      if (!object->HasTag("sprite")) continue;
      auto position =
          std::get<glm::vec3>(object->GetAttribute("transform.position"));
      auto scale = std::get<glm::vec3>(object->GetAttribute("transform.scale"));
      auto rotation =
          std::get<float>(object->GetAttribute("transform.rotation"));
      unsigned int texture =
          std::get<unsigned int>(object->GetAttribute("texture.color"));
      model = glm::translate(glm::mat4(1.0F), position);
      model = glm::rotate(model, glm::radians(rotation),
                          glm::vec3(0.0F, 0.0F, 1.0F));
      model = glm::scale(model, scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, normal_buffer->id);
    glViewport(0, 0, normal_buffer->size.x, normal_buffer->size.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(sprite_shader);
    glBindVertexArray(sprite_vertex_array);
    for (auto& object : scene.objects) {
      if (!object->HasTag("sprite")) continue;
      auto position =
          std::get<glm::vec3>(object->GetAttribute("transform.position"));
      auto scale = std::get<glm::vec3>(object->GetAttribute("transform.scale"));
      auto rotation =
          std::get<float>(object->GetAttribute("transform.rotation"));
      unsigned int texture =
          std::get<unsigned int>(object->GetAttribute("texture.normal"));
      model = glm::translate(glm::mat4(1.0F), position);
      model = glm::rotate(model, glm::radians(rotation),
                          glm::vec3(0.0F, 0.0F, 1.0F));
      model = glm::scale(model, scale);
      glUniformMatrix4fv(glGetUniformLocation(sprite_shader, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      glUniform1i(glGetUniformLocation(sprite_shader, "sprite"), 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);
      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, deferred_buffer->id);
    glViewport(0, 0, deferred_buffer->size.x, deferred_buffer->size.y);
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
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(combine_shader);
    glUniform1i(glGetUniformLocation(combine_shader, "deferred_buffer"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, deferred_buffer->colorbuffer);
    glBindVertexArray(deferred_vertex_attrib);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    ImGui::BeginMainMenuBar();
    if (ImGui::BeginMenu("File")) {
      // Due for implementation
      ImGui::MenuItem("New", nullptr, nullptr, false);
      ImGui::MenuItem("Open", nullptr, nullptr, false);
      ImGui::MenuItem("Save", nullptr, nullptr, false);
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Edit Window", nullptr, &show_edit_window);
      ImGui::MenuItem("ImGui Demo Window", nullptr, &show_demo_window);
      ImGui::MenuItem("Attribute Templates", nullptr, &show_template_window);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    if (show_edit_window) {
      std::vector<std::shared_ptr<Object>> objects_to_erase;
      ImGui::Begin("Edit");
      ImGui::ColorEdit3("Clear Color", glm::value_ptr(clear_color));
      if (ImGui::Button("New Object")) {
        auto object = std::make_shared<Object>();
        object->SetAttribute("transform.position", glm::vec3(0.0F, 0.0F, 0.0F));
        object->SetAttribute("transform.scale", glm::vec3(1.0F, 1.0F, 1.0F));
        object->SetAttribute("transform.rotation", 0.0F);
        scene.objects.push_back(object);
      }
      for (auto& object : scene.objects) {
        if (ImGui::CollapsingHeader(std::format("Object {}", object->name).c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID(object.get());
        if (ImGui::Button("Delete")) {
          objects_to_erase.push_back(object);
        }
        ImGui::InputText("Name", &object->name);
        ImGui::SeparatorText("Tags");
        for (auto& tag : object->tags) {
          ImGui::PushID(&tag);
          ImGui::InputText("", &tag);
          ImGui::SameLine();
          if (ImGui::Button("Remove")) {
            objects_to_erase.push_back(object);
          }
          ImGui::PopID();
        }
        if (ImGui::Button("Add Tag")) {
          object->tags.emplace_back("guten tag");
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
        ImGui::SameLine();
        if (ImGui::Button("Use Template")) {
          ImGui::OpenPopup("Select Template");
        }
        if (ImGui::BeginPopupModal("Select Attribute Type")) {
          static int attr_type = 0;
          ImGui::RadioButton("int", &attr_type, 0);
          ImGui::RadioButton("float", &attr_type, 1);
          ImGui::RadioButton("vec2", &attr_type, 2);
          ImGui::RadioButton("vec3", &attr_type, 3);
          ImGui::RadioButton("texture", &attr_type, 4);
          if (ImGui::Button("Add")) {
            switch (attr_type) {
              case 0:
                object->SetAttribute("new attribute", 0);
                break;
              case 1:
                object->SetAttribute("new attribute", 0.0F);
                break;
              case 2:
                object->SetAttribute("new attribute", glm::vec2(0.0F));
                break;
              case 3:
                object->SetAttribute("new attribute", glm::vec3(0.0F));
                break;
              case 4:
                object->SetAttribute("new attribute", 0U);
                break;
              default:
                break;
            }
            ImGui::CloseCurrentPopup();
          }
          ImGui::SameLine();
          if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("Select Template")) {
          static int selected_template = 0;
          int i = 0;
          for (const auto& [template_name, _] : attribute_templates) {
            ImGui::RadioButton(template_name.c_str(), &selected_template, i);
            i++;
          }
          if (attribute_templates.empty()) {
            ImGui::Text("No templates available");
          }
          if (ImGui::Button("Apply")) {
            auto it = attribute_templates.begin();
            std::advance(it, selected_template);
            for (const auto& [attr_name, attr_data] : it->attributes)
              object->SetAttribute(attr_name, attr_data);
            ImGui::CloseCurrentPopup();
          }
          ImGui::SameLine();
          if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
          }
          ImGui::EndPopup();
        }
        ImGui::PopID();
      }
      }
      for (const auto& object : objects_to_erase) {
        for (auto it = scene.objects.begin(); it != scene.objects.end(); it++) {
          if (*it == object) {
            scene.objects.erase(it);
            break;
          }
        }
      }
      ImGui::End();
    }

    if (show_demo_window) {
      ImGui::ShowDemoWindow(&show_demo_window);
    }

    if (show_template_window) {
      std::vector<AttributeTemplate> templates_to_erase;
      ImGui::Begin("Attribute Templates");
      int index = 0;
      for (auto& attr_template : attribute_templates) {
        ImGui::PushID(index++);
        if (ImGui::CollapsingHeader(attr_template.name.c_str(),
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
          ImGui::InputText("Template Name", &attr_template.name);
          if (ImGui::Button("Add Attribute")) {
            ImGui::OpenPopup("Select Attribute Type##template");
          }
          if (ImGui::BeginPopupModal("Select Attribute Type##template")) {
            static int attr_type = 0;
            ImGui::RadioButton("int", &attr_type, 0);
            ImGui::RadioButton("float", &attr_type, 1);
            ImGui::RadioButton("vec2", &attr_type, 2);
            ImGui::RadioButton("vec3", &attr_type, 3);
            ImGui::RadioButton("vec4", &attr_type, 4);
            ImGui::RadioButton("texture", &attr_type, 5);
            if (ImGui::Button("Add")) {
              switch (attr_type) {
                case 0:
                  attr_template.attributes.emplace_back("new attribute", 0);
                  break;
                case 1:
                  attr_template.attributes.emplace_back("new attribute", 0.0F);
                  break;
                case 2:
                  attr_template.attributes.emplace_back("new attribute",
                                                        glm::vec2(0.0F));
                  break;
                case 3:
                  attr_template.attributes.emplace_back("new attribute",
                                                        glm::vec3(0.0F));
                  break;
                case 4:
                  attr_template.attributes.emplace_back("new attribute",
                                                        glm::vec4(0.0F));
                  break;
                case 5:
                  attr_template.attributes.emplace_back("new attribute", 0U);
                  break;
                default:
                  break;
              }
              ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
              ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
          }
          int attr_index = 0;
          for (auto& attribute : attr_template.attributes) {
            ImGui::PushID(attr_index++);
            ImGui::InputText("Attribute Name", &attribute.first);
            GetInspector(attribute.second);
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
              attr_template.attributes.erase(
                  std::remove(attr_template.attributes.begin(),
                              attr_template.attributes.end(),
                              attribute),
                  attr_template.attributes.end());
            }
            ImGui::PopID();
          }
          if (ImGui::Button("Delete Template")) {
            templates_to_erase.push_back(attr_template);
          }
        }
        ImGui::PopID();
      }
      if (ImGui::Button("Add Template")) {
        attribute_templates.emplace_back(AttributeTemplate{
            .name = "New Template",
            .attributes = {}});
      }
      ImGui::SameLine();
      if (ImGui::Button("Load Templates")) {
        if (std::filesystem::exists("attributes.xml"))
          attribute_templates = attributes::LoadTemplates();
        else {
          ImGui::OpenPopup("Load Failed");
        }
      }
      if (ImGui::BeginPopupModal("Load Failed")) {
      ImGui::Text("No templates found to load.");
      if (ImGui::Button("OK")) {
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
      }
      ImGui::SameLine();
      if (ImGui::Button("Save Templates")) {
        attributes::SaveTemplates(attribute_templates);
      }
      for (const auto& attr_template : templates_to_erase) {
        for (auto it = attribute_templates.begin(); it != attribute_templates.end();
             it++) {
          if (it->name == attr_template.name) {
            attribute_templates.erase(it);
            break;
          }
        }
      }
      ImGui::End();
    }

    ImGui::Render();
    ImGui::EndFrame();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(loaded_vertex_arrays.size(),
                       loaded_vertex_arrays.data());
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
