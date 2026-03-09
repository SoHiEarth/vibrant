#include "scene.h"
#include <pugixml.hpp>
#include "texture.h"
Scene LoadScene(std::string_view path) {
  Scene scene;
  pugi::xml_document doc;
  auto result = doc.load_file(path.data());
  if (!result) {
    throw std::runtime_error("Failed to load scene");
  }
  auto root = doc.child("scene");
  for (auto object_node : root.children("object")) {
    auto object = std::make_shared<Object>();
    for (auto attribute_node : object_node.children("attribute")) {
      const std::string name = attribute_node.attribute("name").as_string();
      const std::string type = attribute_node.attribute("type").as_string();
      const std::string value = attribute_node.attribute("value").as_string();
      if (type == "int") {
        object->SetAttribute(name, std::stoi(value));
      } else if (type == "float") {
        object->SetAttribute(name, std::stof(value));
      } else if (type == "vec2") {
        glm::vec2 vec;
        sscanf(value.c_str(), "%f,%f", &vec.x, &vec.y);
        object->SetAttribute(name, vec);
      } else if (type == "vec3") {
        glm::vec3 vec;
        sscanf(value.c_str(), "%f,%f,%f", &vec.x, &vec.y,
                &vec.z);
        object->SetAttribute(name, vec);
      } else if (type == "texture") {
        object->SetAttribute(name, LoadTexture(value));
      } else {
        throw std::runtime_error("Unknown attribute type");
      }
    }
    for (auto tag_node : object_node.children("tag")) {
      object->tags.emplace_back(tag_node.attribute("name").as_string());
    }
    scene.objects.push_back(object);
  }
  return scene;
}

void SaveScene(const Scene& scene, std::string_view path) {
  pugi::xml_document doc;
  auto root = doc.append_child("scene");
  for (const auto& object : scene.objects) {
    auto object_node = root.append_child("object");
    for (auto& [name, value] : object->attributes) {
      auto attribute_node = object_node.append_child("attribute");
      attribute_node.append_attribute("name") = name.c_str();
      if (std::holds_alternative<int>(value)) {
        attribute_node.append_attribute("type") = "int";
        attribute_node.append_attribute("value") =
            std::to_string(std::get<int>(value)).c_str();
      } else if (std::holds_alternative<float>(value)) {
        attribute_node.append_attribute("type") = "float";
        attribute_node.append_attribute("value") =
            std::to_string(std::get<float>(value)).c_str();
      } else if (std::holds_alternative<glm::vec2>(value)) {
        attribute_node.append_attribute("type") = "vec2";
        auto vec = std::get<glm::vec2>(value);
        attribute_node.append_attribute("value") =
            std::to_string(vec.x) + "," + std::to_string(vec.y);
      } else if (std::holds_alternative<glm::vec3>(value)) {
        attribute_node.append_attribute("type") = "vec3";
        auto vec = std::get<glm::vec3>(value);
        attribute_node.append_attribute("value") =
            (std::to_string(vec.x) + "," + std::to_string(vec.y) + "," +
             std::to_string(vec.z))
                .c_str();
      } else if (std::holds_alternative<Texture>(value)) {
        attribute_node.append_attribute("type") = "texture";
        Texture texture = std::get<Texture>(value);
        attribute_node.append_attribute("value") = texture.path.c_str();
      }
    }
    for (auto& tag : object->tags) {
      auto tag_node = object_node.append_child("tag");
      tag_node.append_attribute("name") = tag.c_str();
    }
  }
  doc.save_file(path.data());
}
