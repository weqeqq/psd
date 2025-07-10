
#pragma once

#include <psd/llapi/stream.h>

namespace PSD::llapi {
//
class ColorInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, ColorInfo &) {
      stream += stream.Read<U32>();
    }
  };
  struct ToStreamFn {
    void operator()(Stream &stream, const ColorInfo &) {
      stream.Write(U32(0));
    }
  };
  friend Stream;
public:
  ColorInfo() = default;
  constexpr bool operator==(const ColorInfo &other) const { return true; }
  constexpr bool operator!=(const ColorInfo &other) const { return false; }
  constexpr unsigned Length() const { return 4; }
}; // class ColorInfo
}; // namespace PSD::llapi
