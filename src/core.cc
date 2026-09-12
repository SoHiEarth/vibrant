#include "core.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>
#include <iostream>

void core::Initialize(InitializeFlags flags, void* window_ptr) {
  if (static_cast<int>(flags) & static_cast<int>(InitializeFlags::kOpengl)) {
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

  if (static_cast<int>(flags) & static_cast<int>(InitializeFlags::kImgui)) {
    if (window_ptr == nullptr) {
      throw std::runtime_error(
          "IMGUI initialization requires a valid GLFWwindow pointer");
    }
    auto* window = static_cast<GLFWwindow*>(window_ptr);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
    float dpi_scale_x = 0.0F;
    float dpi_scale_y = 0.0F;
    glfwGetWindowContentScale(window, &dpi_scale_x, &dpi_scale_y);
    std::cout << "Using dpi scale " << std::max(dpi_scale_x, dpi_scale_y) << std::endl;
    ImGui::GetStyle().ScaleAllSizes(std::max(dpi_scale_x, dpi_scale_y));
    ImGui::GetIO().FontGlobalScale = std::max(dpi_scale_x, dpi_scale_y);
  }
}
