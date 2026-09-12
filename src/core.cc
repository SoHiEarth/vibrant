#include "core.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdexcept>

void core::Initialize(InitializeFlags flags, void* window_ptr) {
  if (static_cast<int>(flags) & static_cast<int>(InitializeFlags::OPENGL)) {
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  }

  if (static_cast<int>(flags) & static_cast<int>(InitializeFlags::IMGUI)) {
    if (window_ptr == nullptr) {
      throw std::runtime_error("IMGUI initialization requires a valid GLFWwindow pointer");
    }
    auto window = static_cast<GLFWwindow*>(window_ptr);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
  }
}
