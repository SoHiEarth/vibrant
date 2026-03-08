#pragma once
#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

using AttributeData =
    std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, unsigned int>;

struct Object {
  std::string name;
  std::vector<std::string> tags;
  std::vector<std::pair<std::string, AttributeData>> attributes;
  bool HasTag(const std::string& tag) {
    return std::ranges::any_of(tags,
                               [&](const std::string& t) { return t == tag; });
  }

  AttributeData& GetAttribute(std::string_view attribute_name) {
    for (auto& [name, data] : attributes) {
      if (name == attribute_name) {
        return data;
      }
    }
    throw std::runtime_error("Attribute not found: " +
                             std::string(attribute_name));
  }

  void SetAttribute(std::string_view attribute_name,
                    const AttributeData& value) {
    attributes.emplace_back(attribute_name, value);
    std::cout << "Registered attribute: " << attribute_name << std::endl;
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
