cmake_minimum_required(VERSION 4.0)
project(vibrant)
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(ImGui CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Stb REQUIRED)
find_package(pugixml CONFIG REQUIRED)

add_executable(ion-base)
target_compile_features(ion-base PRIVATE cxx_std_23)
target_sources(ion-base PRIVATE
  src/main.cc
)
target_include_directories(ion-base PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ion-base PUBLIC
    glad::glad
    glfw
    glm::glm
    imgui::imgui
    OpenGL::GL
    pugixml::pugixml
)
