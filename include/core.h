#pragma once

namespace core {
  enum class InitializeFlags {
    NONE = 0,
    OPENGL = 1 << 0,
    IMGUI = 1 << 1,
    ALL = OPENGL | IMGUI
  };

  void Initialize(InitializeFlags flags, void* window_ptr);
}
