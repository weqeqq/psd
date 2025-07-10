
#pragma once

#include <psd/llapi/structure/resource_info/resource.h>

namespace PSD::llapi {
//
class DefaultResource : public ResourceFor<DefaultResource> {
  friend ResourceFor<DefaultResource>;
  struct FromStreamFn {
    void operator()(Stream &stream, DefaultResource &output, const ResourceHeader &header) {
      output.header_ = header;
      stream += header.content_length;
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &, const DefaultResource &) {}
  }; // struct ToStreamFn
  friend Stream;
public:
  DefaultResource() = default;
  constexpr bool operator==(const DefaultResource &other) const { return true; }
  constexpr bool operator!=(const DefaultResource &other) const { return false; }
private:
  ResourceHeader header_;
}; // class DefaultResource
}; // namespace PSD::llapi
