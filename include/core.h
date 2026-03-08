#pragma once

namespace core {
enum class InitializeFlags {
  kNone = 0,
  kOpengl = 1 << 0,
  kImgui = 1 << 1,
  kAll = kOpengl | kImgui
};

void Initialize(InitializeFlags flags, void* window_ptr);
}  // namespace core
