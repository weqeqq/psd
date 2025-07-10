
#pragma once

#include <psd/llapi/stream.h>

namespace PSD::llapi {
//
class GlobalInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, GlobalInfo &) {
      stream += stream.Read<U32>();
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const GlobalInfo &input) {
      stream.Write(input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  GlobalInfo() = default;
  constexpr bool operator==(const GlobalInfo &other) const { return true; }
  constexpr bool operator!=(const GlobalInfo &other) const { return false; }
  constexpr unsigned Length() const {
    return 4 + ContentLength();
  }
private:
  constexpr U32 ContentLength() const {
    return 0;
  }
}; // class GlobalInfo
}; // namespace PSD::llapi
