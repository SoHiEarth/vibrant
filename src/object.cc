#include "object.h"
#include <pugixml.hpp>
#include <sstream>
#include <iostream>

namespace {
std::vector<std::string> Split(std::string_view s, char delimiter) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream token_stream(s.data());
  while (std::getline(token_stream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}
}

std::vector<AttributeTemplate> attributes::LoadTemplates() {
  std::vector<AttributeTemplate> templates;
  pugi::xml_document doc;
  pugi::xml_parse_result result = doc.load_file("attributes.xml");
  if (!result) {
    std::cerr << "Failed to load attributes.xml: " << result.description()
              << std::endl;
    return templates;
  }

  for (pugi::xml_node template_node : doc.child("Attributes").children("Template")) {
    std::string template_name = template_node.attribute("name").as_string();
    AttributeTemplate attr_template;
    attr_template.name = template_name;

    for (pugi::xml_node attr_node : template_node.children("Attribute")) {
      std::string attr_name = attr_node.attribute("name").as_string();
      std::string type = attr_node.attribute("type").as_string();
      std::string value_str = attr_node.attribute("value").as_string();

      if (type == "int") {
        attr_template.attributes.emplace_back(attr_name, std::stoi(value_str));
      } else if (type == "float") {
        attr_template.attributes.emplace_back(attr_name, std::stof(value_str));
      } else if (type == "vec2") {
        auto parts = Split(value_str, ',');
        if (parts.size() == 2) {
          attr_template.attributes.emplace_back(
              attr_name, glm::vec2(std::stof(parts[0]), std::stof(parts[1])));
        }
      } else if (type == "vec3") {
        auto parts = Split(value_str, ',');
        if (parts.size() == 3) {
          attr_template.attributes.emplace_back(
              attr_name, glm::vec3(std::stof(parts[0]), std::stof(parts[1]),
                                  std::stof(parts[2])));
        }
      } else if (type == "vec4") {
        auto parts = Split(value_str, ',');
        if (parts.size() == 4) {
          attr_template.attributes.emplace_back(
              attr_name,
              glm::vec4(std::stof(parts[0]), std::stof(parts[1]),
                        std::stof(parts[2]), std::stof(parts[3])));
        }
      } else if (type == "uint") {
        attr_template.attributes.emplace_back(attr_name,
                                             static_cast<unsigned int>(
                                                 std::stoul(value_str)));
      }
    }

    templates.push_back(attr_template);
  }

  return templates;
}

void attributes::SaveTemplates(const std::vector<AttributeTemplate> &templates) {
  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child("Attributes");

  for (const auto &attr_template : templates) {
    pugi::xml_node template_node = root.append_child("Template");
    template_node.append_attribute("name") = attr_template.name.c_str();

    for (const auto &[attr_name, attr_data] : attr_template.attributes) {
      pugi::xml_node attr_node = template_node.append_child("Attribute");
      attr_node.append_attribute("name") = attr_name.c_str();
      if (std::holds_alternative<int>(attr_data)) {
        attr_node.append_attribute("type") = "int";
        attr_node.append_attribute("value") =
            std::to_string(std::get<int>(attr_data)).c_str();
      } else if (std::holds_alternative<float>(attr_data)) {
        attr_node.append_attribute("type") = "float";
        attr_node.append_attribute("value") =
            std::to_string(std::get<float>(attr_data)).c_str();
      } else if (std::holds_alternative<glm::vec2>(attr_data)) {
        attr_node.append_attribute("type") = "vec2";
        const auto &v = std::get<glm::vec2>(attr_data);
        attr_node.append_attribute("value") =
            (std::to_string(v.x) + "," + std::to_string(v.y)).c_str();
      } else if (std::holds_alternative<glm::vec3>(attr_data)) {
        attr_node.append_attribute("type") = "vec3";
        const auto &v = std::get<glm::vec3>(attr_data);
        attr_node.append_attribute("value") =
            (std::to_string(v.x) + "," + std::to_string(v.y) + "," +
             std::to_string(v.z))
                .c_str();
      } else if (std::holds_alternative<glm::vec4>(attr_data)) {
        attr_node.append_attribute("type") = "vec4";
        const auto &v = std::get<glm::vec4>(attr_data);
        attr_node.append_attribute("value") =
            (std::to_string(v.x) + "," + std::to_string(v.y) + "," +
             std::to_string(v.z) + "," + std::to_string(v.w))
                .c_str();
      } else if (std::holds_alternative<unsigned int>(attr_data)) {
        attr_node.append_attribute("type") = "uint";
        attr_node.append_attribute("value") =
            std::to_string(std::get<unsigned int>(attr_data)).c_str();
      }
    }
  }

  if (!doc.save_file("attributes.xml")) {
    std::cerr << "Failed to save attributes.xml" << std::endl;
  }
}
