
#pragma once

#include <psd/llapi/structure/resource_info/default_resource.h>
#include <unordered_map>

namespace PSD::llapi {
//
class ResourceInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, ResourceInfo &resource_info) {
      auto end_of_read = stream.Read<U32>() + stream.Pos();
      while (stream.Pos() < end_of_read) {
        auto header = stream.Read<ResourceHeader>();
        switch (header.id) {
          default: ReadResourceFrom<DefaultResource>(stream, header);
        }
      }
    }
    template <typename T>
    std::shared_ptr<Resource> ReadResourceFrom(Stream &stream, const ResourceHeader &header) {
      return stream.Read<std::shared_ptr<Resource>>(
        std::static_pointer_cast<Resource>(std::make_shared<T>(stream.Read<T>(header)))
      );
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const ResourceInfo &input) {
      stream.Write(input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  ResourceInfo() = default;
  unsigned Length() const {
    return 4 + ContentLength();
  }
private:
  std::unordered_map<ResourceID, std::shared_ptr<Resource>> data_;
  U32 ContentLength() const {
    U32 length = 0;
    for (const auto &[id, resource] : data_) {
      length += resource->Length();
    }
    return length;
  }
}; // class ResourceInfo
}; // namespace PSD::llapi
