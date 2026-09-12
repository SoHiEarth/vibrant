#pragma once
#include <memory>
#include <vector>

#include "object.h"

struct Scene {
  std::vector<std::shared_ptr<Object>> objects;
};

Scene LoadScene(std::string_view path);
void SaveScene(const Scene& scene, std::string_view path);
