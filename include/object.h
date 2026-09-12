#pragma once
#include <string>
#include <variant>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>

using AttributeData = std::variant<int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4, unsigned int>;

struct Object {
  std::string name;
  std::vector<std::string> tags;
  std::vector<std::pair<std::string, AttributeData>> attributes;
  bool HasTag(const std::string& tag) {
    for (auto& t : tags) {
      if (t == tag) {
        return true;
      }
    }
    return false;
  }

  AttributeData& GetAttribute(const std::string& attribute_name) {
    for (auto& [name, data] : attributes) {
      if (name == attribute_name) {
        return data;
      }
    }
    throw std::runtime_error("Attribute not found: " + attribute_name);
  }

  void SetAttribute(const std::string& attribute_name, const AttributeData& value) {
    attributes.push_back({attribute_name, value});
    std::cout << "Registered attribute: " << attribute_name << std::endl;
  }
};
