#pragma once
#include <glm/glm.hpp>

enum class LightType : int {
  Global,
  Point
};

struct Light {
  LightType type = LightType::Point;
  float intensity = 1;
  glm::vec3 color = { 1.0f, 1.0f, 1.0f };
  float radial_falloff = 1.0;
  float volumetric_intensity = 1.0;
};
