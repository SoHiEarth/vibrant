#pragma once
#include <string>
#include <vector>
#include <iostream>

struct Object {
  std::string name;
  std::vector<std::string> tags;
  std::vector<std::pair<std::string, void*>> attributes;
  bool HasTag(const std::string& tag) {
    for (auto& t : tags) {
      if (t == tag) {
        return true;
      }
    }
    return false;
  }

  void* GetAttribute(const std::string& attribute_name) {
    for (auto& [name, data] : attributes) {
      if (name == attribute_name) {
        return data;
      }
    }
    throw std::runtime_error("Attribute not found: " + attribute_name);
  }

  void SetAttribute(const std::string& attribute_name, void* value) {
    attributes.push_back({attribute_name, value});
    std::cout << "Registered attribute: " << attribute_name << std::endl;
  }
};
