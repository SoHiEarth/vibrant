#pragma once
#include <glm/glm.hpp>

enum class LightType : int {
  Global,
  Point
};

struct Light {
  LightType type = LightType::Point;
  float intensity = 5;
  glm::vec3 color = { 1.0f, 1.0f, 1.0f };
  float radial_falloff = 0.1;
  float volumetric_intensity = 0.1;
};
