#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "log.h"
#include "texture.h"

using AttributeData =
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, Texture>;

struct Object {
  std::string name;
  std::vector<std::string> tags;
  std::vector<std::pair<std::string, AttributeData>> attributes;
  bool HasTag(std::string_view tag) const {
    return std::ranges::any_of(tags,
                               [&](std::string_view t) { return t == tag; });
  }

  AttributeData& GetAttribute(std::string_view attribute_name) {
    for (auto& [name, data] : attributes) {
      if (name == attribute_name) {
        return data;
      }
    }
    output_log["Attribute Not Found: " + std::string(attribute_name)] =
        LogLevel::kError;
    throw std::runtime_error("Attribute not found: " +
                             std::string(attribute_name));
  }

  void SetAttribute(std::string_view attribute_name,
                    const AttributeData& value) {
    attributes.emplace_back(attribute_name, value);
    output_log["Attribute Set: " + std::string(attribute_name)] =
        LogLevel::kInfo;
  }
};

struct AttributeTemplate {
  std::string name;
  std::vector<std::pair<std::string, AttributeData>> attributes;
};

namespace attributes {
std::vector<AttributeTemplate> LoadTemplates();
void SaveTemplates(const std::vector<AttributeTemplate>& templates);
}  // namespace attributes
