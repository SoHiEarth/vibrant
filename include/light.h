#pragma once
#include <glm/glm.hpp>

enum class LightType : int {
  Global,
  Point
};

struct Light {
  LightType type = LightType::Point;
  float intensity = 100;
  glm::vec3 color = { 255, 255, 255 };
  float radial_falloff = 0.1;
  float volumetric_intensity = 0.1;
};
