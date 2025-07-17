
#pragma once

#include "extra.h"

namespace PSD::llapi {
//
class DefaultExtra : public ExtraFor<DefaultExtra> {
  friend ExtraFor<DefaultExtra>;
  struct FromStreamFn {
    void operator()(Stream &stream, DefaultExtra &output, const ExtraHeader &header) {
      output.content_length_ = header.content_length;
      stream += header.content_length;
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &, const DefaultExtra &) {}
  }; // struct ToStreamFn
  friend Stream;
public:
  DefaultExtra() = default;
  constexpr bool operator==(const DefaultExtra &other) const { return true; }
  constexpr bool operator!=(const DefaultExtra &other) const { return false; }
private:
  U32 ContentLength() const override final {
    return content_length_;
  }
  unsigned content_length_;
  // ExtraHeader header_;
}; // class DefaultExtra
}; // namespace PSD::llapi
