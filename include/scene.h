#pragma once
#include <vector>
#include "object.h"

struct Scene {
  std::vector<std::shared_ptr<Object>> objects;
};
