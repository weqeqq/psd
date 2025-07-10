
#pragma once

#include <cstdint>
#include <psd/llapi/stream.h>
#include <psd/error.h>
#include <tuple>
#include <type_traits>

namespace PSD::llapi {
//
enum class Version : U16 {
  PSD = 1,
  PSB = 2,
};
template <>
struct FromStreamFn<Version> {
  void operator()(Stream &stream, Version &version) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Version> &>(version));
    if (version != Version::PSD &&
        version != Version::PSB) throw Error("PSD::Error: VersionError");
  }
};
template <>
struct ToStreamFn<Version> {
  void operator()(Stream &stream, Version version) {
    stream.Write(static_cast<std::underlying_type_t<Version>>(version));
  }
};
enum class Depth : U16 {
  One       = 1,
  Eight     = 8,
  Sixteen   = 16,
  ThirtyTwo = 32,
};

template <>
struct FromStreamFn<Depth> {
  void operator()(Stream &stream, Depth &depth) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Depth> &>(depth));
    if (depth != Depth::One     &&
        depth != Depth::Eight   &&
        depth != Depth::Sixteen &&
        depth != Depth::ThirtyTwo)
      throw Error(
          "PSD::Error: DepthError\n"
          "Expected: 1, 8, 16, or 32\n"
          "Received: " + std::to_string(static_cast<std::underlying_type_t<Depth>>(depth))
      );
  }
};
template <>
struct ToStreamFn<Depth> {
  void operator()(Stream &stream, Depth depth) {
    stream.Write(static_cast<std::underlying_type_t<Depth>>(depth));
  }
};
enum class Color : std::uint16_t {
  Bitmap       = 0,
  Grayscale    = 1,
  Indexed      = 2,
  Rgb          = 3,
  Cmyk         = 4,
  Multichannel = 7,
  Duotone      = 8,
  Lab          = 9,
};
template <>
struct FromStreamFn<Color> {
  void operator()(Stream &stream, Color &color) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Color> &>(color));
  }
};
template <>
struct ToStreamFn<Color> {
  void operator()(Stream &stream, Color color) {
    stream.Write(static_cast<std::underlying_type_t<Depth>>(color));
  }
};
class Header {
  struct FromStreamFn {
    void operator()(Stream &stream, Header &header) {
      if (stream.Read<U32>() != 0x38425053) {
        throw Error("PSD::Error: HeaderSignatureError");
      }
      stream.ReadTo(header.version);
      stream += 6;
      stream.ReadTo(header.channel_count);
      stream.ReadTo(header.row_count);
      stream.ReadTo(header.column_count);
      stream.ReadTo(header.depth);
      stream.ReadTo(header.color);
    }
  };
  struct ToStreamFn {
    void operator()(Stream &stream, const Header &header) {
      stream.Write<U32>(0x38425053);
      stream.Write(header.version);
      stream.Write<U32>(0x00);
      stream.Write<U16>(0x00);
      stream.Write(header.channel_count);
      stream.Write(header.row_count);
      stream.Write(header.column_count);
      stream.Write(header.depth);
      stream.Write(header.color);
    }
  };
  friend Stream;
  auto Comparable() const {
    return std::tie(
      version   , channel_count,
      row_count , column_count,
      depth     , color
    );
  }
public:
  Header() = default;
  Header(unsigned row_count, unsigned column_count)
    : row_count(row_count), column_count(column_count) {}

  bool operator==(const Header &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const Header &other) const {
    return !operator==(other);
  }
  Version       version       = Version::PSD;
  std::uint16_t channel_count = 3;
  std::uint32_t row_count     = 0;
  std::uint32_t column_count  = 0;
  Depth         depth         = Depth::Eight;
  Color         color         = Color::Rgb;
}; // class Header
}; // namespace PSD::llapi
