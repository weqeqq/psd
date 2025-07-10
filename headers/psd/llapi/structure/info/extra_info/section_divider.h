
#pragma once

#include <psd/llapi/structure/info/extra_info/extra.h>

namespace PSD::llapi {
//
enum class DividerType : U32 {
  Layer        = 0,
  OpenFolder   = 1,
  ClosedFolder = 2,
  Hidden       = 3,
};
template <>
struct FromStreamFn<DividerType> {
  void operator()(Stream &stream, DividerType &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<DividerType> &>(output));
  }
}; // struct FromStreamFn<DividerType>
template <>
struct ToStreamFn<DividerType> {
  void operator()(Stream &stream, DividerType input) {
    stream.Write(static_cast<std::underlying_type_t<DividerType>>(input));
  }
}; // struct ToStreamFn<DividerType>
enum class Blending : U32 {
  Normal      = 0x6E6F726D,
  PassThrough = 0x70617373,
}; // enum class Blending
template <>
struct FromStreamFn<Blending> {
  void operator()(Stream &stream, Blending &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Blending> &>(output));
    if (output != Blending::Normal &&
        output != Blending::PassThrough) {
      throw Error("PSD::Error: UnsupportedBlending");
    }
  }
}; // struct FromStreamFn<Blending>
template <>
struct ToStreamFn<Blending> {
  void operator()(Stream &stream, Blending input) {
    stream.Write(static_cast<std::underlying_type_t<Blending>>(input));
  }
}; // struct ToStreamFn<Blending>
enum class DividerSubType : U32 {
  Normal     = 0,
  SceneGroup = 1,
  None       = 2
};
template <>
struct FromStreamFn<DividerSubType> {
  void operator()(Stream &stream, DividerSubType &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<DividerSubType> &>(output));
  }
}; // struct FromStreamFn<DividerSubType>
template <>
struct ToStreamFn<DividerSubType> {
  void operator()(Stream &stream, DividerSubType input) {
    stream.Write(static_cast<std::underlying_type_t<DividerSubType>>(input));
  }
}; // struct ToStreamFn<DividerSubType>
class SectionDivider : public ExtraFor<SectionDivider>{
  friend ExtraFor<SectionDivider>;
  struct FromStreamFn {
    void operator()(Stream &stream, SectionDivider &output, const ExtraHeader &header) {
      stream.ReadTo(output.type);
      if (header.content_length < 12) {
        return;
      }
      if (stream.Read<U32>() != 0x3842494D) {
        throw Error("PSD::Error: SectionDividerSignatureError");
      }
      stream.ReadTo(output.blending);
      if (header.content_length < 16) {
        return;
      }
      stream.ReadTo(output.subtype);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const SectionDivider &input) {
      stream.Write(input.type);
      if (input.type == DividerType::Layer ||
          input.type == DividerType::Hidden) {
        return;
      }
      stream.Write(U32(0x3842494D));
      stream.Write(input.blending);
      if (input.subtype == DividerSubType::None) {
        return;
      }
      stream.Write(input.subtype);
    }
  }; // struct ToStreamFn
  friend Stream;
  auto Comparable() const {
    return std::tie(type, blending, subtype);
  }
public:
  SectionDivider() = default;
  SectionDivider(DividerType type) : type(type) {};

  bool operator==(const SectionDivider &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const SectionDivider &other) const {
    return !operator==(other);
  }
  DividerType    type     = DividerType::Layer;
  Blending       blending = Blending::PassThrough;
  DividerSubType subtype  = DividerSubType::None;
protected:
  U32 ContentLength() const override final {
    if (type == DividerType::Layer || type == DividerType::Hidden) {
      return 4;
    } else {
      auto output = 12;
      if (subtype != DividerSubType::None) {
        output += 4;
      }
      return output;
    }
  }
};
namespace detail {
//
template <>
struct ExtraToID<SectionDivider> : UseExtraID<ExtraID::SectionDivider> {};
}; // namespace detail
}; // namespace PSD::llapi
