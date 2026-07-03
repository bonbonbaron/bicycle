#pragma once

template <typename T>
struct TypeTag {
  static inline const char dummy{};
};

template <typename T>
inline const void* getTypeTag() noexcept {
  return &TypeTag<T>::dummy;
}


