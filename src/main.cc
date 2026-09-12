#include <glad/glad.h>
// CODE BLOCK: To stop clang from messing with my include
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <print>
#include <array>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
constexpr glm::ivec2 default_window_size = { 800, 600 };
constexpr int log_size = 512;
constexpr std::array<float, 12> vertices = {
     0.5f,  0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    -0.5f,  0.5f, 0.0f
};
constexpr std::array<unsigned int, 6> indices = {
    0, 1, 3,
    1, 2, 3
};

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

// This function won't store the value of new window size as a global variable
// Ask GLFW if you want the window size
void WindowResizeCallback(GLFWwindow* window, int w, int h) {
  glViewport(0, 0, w, h);
}

// Presents error, if any
void PresentGlfwErrorInfo() {
  const char* error_desc;
  auto error = glfwGetError(&error_desc);
  if (error != GLFW_NO_ERROR)
    std::print("Error Description: {}\n", error_desc ? error_desc : "No Description Provided, gg.");
}

int main (int argc, char *argv[]) {
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
  unsigned int vertex_attrib, vertex_buffer, indices_buffer, shader_program;
  auto vertex = CompileShader(GL_VERTEX_SHADER, ReadFile("assets/vert.glsl")),
       fragment = CompileShader(GL_FRAGMENT_SHADER, ReadFile("assets/frag.glsl"));
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex);
  glAttachShader(shader_program, fragment);
  glLinkProgram(shader_program);
  int success;
  std::array<char, log_size> log{};
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (success == 0) {
    glGetProgramInfoLog(shader_program, log_size, nullptr, log.data());
    throw std::runtime_error("Shader program link failed: " + std::string(log.data()));
  }
  glDeleteShader(vertex);
  glDeleteShader(fragment);

  glGenVertexArrays(1, &vertex_attrib);
  glGenBuffers(1, &vertex_buffer);
  glGenBuffers(1, &indices_buffer);
  glBindVertexArray(vertex_attrib);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(float), indices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  // Main loop
  // Again, gut this part out of main sometime.
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shader_program);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ImGui::ShowDemoWindow();
    ImGui::Render();
    ImGui::EndFrame();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteVertexArrays(1, &vertex_attrib);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &indices_buffer);
  glDeleteProgram(shader_program);
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
