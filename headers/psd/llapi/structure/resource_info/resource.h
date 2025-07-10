
#pragma once

#include <psd/error.h>
#include <memory>
#include <psd/llapi/stream.h>
#include <type_traits>

namespace PSD::llapi {
//
enum class ResourceID : U16 {
  LayerState     = 0x0400,
  ResolutionInfo = 0x03ED,
}; // enum class ResourceID
template <>
struct FromStreamFn<ResourceID> {
  void operator()(Stream &stream, ResourceID &id) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<ResourceID> &>(id));
  }
}; // struct FromStreamFn<ResourceID>
template <>
struct ToStreamFn<ResourceID> {
  void operator()(Stream &stream, ResourceID id) {
    stream.Write(static_cast<std::underlying_type_t<ResourceID>>(id));
  }
}; // struct ToStreamFn<ResourceID>
class Resource {
  friend FromStreamFn<std::shared_ptr<Resource>>;
  friend ToStreamFn<std::shared_ptr<Resource>>;
public:
  virtual ~Resource() = default;
  virtual U32 Length() const = 0;
  virtual bool Compare(
    const std::shared_ptr<Resource> &other
  ) const = 0;
  virtual std::shared_ptr<Resource>
  Clone() const = 0;
protected:
  virtual U32 ContentLength() const = 0;
  virtual void ToStream(Stream &stream) const = 0;
}; // class Resource
template <>
struct FromStreamFn<std::shared_ptr<Resource>> {
  void operator()(Stream &stream, std::shared_ptr<Resource> &output, std::shared_ptr<Resource> resource) {
    if (resource->ContentLength() % 2) {
      stream++;
    }
    output = resource;
  }
}; // struct FromStreamFn<std::shared_ptr<Resource>>
template <>
struct ToStreamFn<std::shared_ptr<Resource>> {
  void operator()(Stream &stream, std::shared_ptr<Resource> resource) {
    resource->ToStream(stream);
  }
}; // struct ToStreamFn<std::shared_ptr<Resource>>
class ResourceHeader {
  struct FromStreamFn {
    void operator()(Stream &stream, ResourceHeader &header) {
      if (stream.Read<U32>() != 0x3842494D) {
        throw Error("PSD::Error: ResourceHeaderSignatureError");
      }
      stream.ReadTo(header.id);
      auto length = stream.Read<U8>();
      stream += length
        ? length + 0
        : length + 1;
      stream.ReadTo(header.content_length);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const ResourceHeader &header) {
      stream.Write(U32(0x3842494D));
      stream.Write(header.id);
      stream.Write(U16(0));
      stream.Write(header.content_length);
    }
  }; // struct ToStreamFn
  friend Stream;
  auto Comparable() const {
    return std::tie(id, content_length );
  }
public:
  ResourceHeader() = default;
  bool operator==(const ResourceHeader &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const ResourceHeader &other) const {
    return !operator==(other);
  }
  ResourceID id; U32 content_length;
  constexpr unsigned Length() const {
    return 12;
  }
}; // class ResourceHeader
template <typename T>
class ResourceFor : public Resource {
public:
  U32 Length() const override final {
    return Header().Length() + Padding(ContentLength());
  }
  bool Compare(
    const std::shared_ptr<Resource> &resource
  ) const override final {
    return std::static_pointer_cast<T>(resource)->operator==(Self());
  }
  std::shared_ptr<Resource>
  Clone() const override final {
    return std::make_shared<T>(Self());
  }
protected:
  U32 ContentLength() const override final {
    return Header().content_length;
  }
  void ToStream(Stream &stream) const override final {
    stream.Write(Header());
    stream.Write(Self());
    if (ContentLength() % 2) {
      stream.Write(U8(0));
    }
  }
private:
  const T &Self() const {
    return *reinterpret_cast<const T *>(this);
  }
  const ResourceHeader &Header() const { return Self().header_; }
  static U32 Padding(U32 input) {
    return input % 2 ? input + 1 : input;
  }
}; // class ResourceFor
}; // namespace PSD::llapi
