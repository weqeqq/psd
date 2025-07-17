
#pragma once

#include "psd/error.h"
#include "psd/llapi/structure/header.h"
#include "psd/llapi/structure/info/layer_info/layer_data.h"
#include <cassert>
#include <map>
#include <psd/llapi/stream.h>
#include <psd/export.h>
#include <type_traits>

namespace PSD::llapi {
//
enum class Compression : U16 {
  None          = 0,
  Default       = 1,
  Deflate       = 2,
  DeflateDelta  = 3,
}; // enum class Compression
template<>
struct FromStreamFn<Compression> {
  void operator()(Stream &stream, Compression &output) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<Compression> &>(output));
    if (output != Compression::None &&
        output != Compression::Default &&
        output != Compression::Deflate &&
        output != Compression::DeflateDelta) throw Error("PSD::Error: UnsupportedCompression");
  }
}; // FromStreamFn<Compression>
template <>
struct ToStreamFn<Compression> {
  void operator()(Stream &stream, Compression input) {
    stream.Write(static_cast<std::underlying_type_t<Compression>>(input));
  }
}; // ToStreamFn<Compression>
constexpr unsigned ByteCount(Depth depth) {
  if (depth == Depth::One) {
    return 0;
  } else {
    return static_cast<std::underlying_type_t<Depth>>(depth) / 8;
  }
}
PSD_EXPORT std::vector<U8>
DecompressDefault(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
);
PSD_EXPORT std::vector<U8>
DecompressDeflate(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
);
PSD_EXPORT std::vector<U8>
DecompressDeflateDelta(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
);
inline std::vector<U8> Decompress(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  Compression compression
) {
  auto decompress = [&](auto function){
    return function(
      input,
      row_count,
      column_count,
      depth
    );
  };
  switch (compression) {
    case Compression::None         : return input;
    case Compression::Default      : return decompress(DecompressDefault);
    case Compression::Deflate      : return decompress(DecompressDeflate);
    case Compression::DeflateDelta : return decompress(DecompressDeflateDelta);
    default: throw Error("err");
  }
}
PSD_EXPORT std::vector<U8>
CompressDefault(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  unsigned level
);
PSD_EXPORT std::vector<U8>
CompressDeflate(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  unsigned level
);
PSD_EXPORT std::vector<U8>
CompressDeflateDelta(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  unsigned level
);
inline std::vector<U8> Compress(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  Compression compression,
  unsigned level
) {
  auto compress = [&](auto function){
    return function(
      input,
      row_count,
      column_count,
      depth,
      level
    );
  };
  switch (compression) {
    case Compression::None         : return input;
    case Compression::Default      : return compress(CompressDefault);
    case Compression::Deflate      : return compress(CompressDeflate);
    case Compression::DeflateDelta : return compress(CompressDeflateDelta);
    default: throw Error("err");
  }
}
class Channel {
  struct FromStreamFn {
    void operator()(Stream &stream, Channel &output, unsigned length) {
      stream.ReadTo(output.compression);
      stream.ReadTo(output.data, length - 2);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Channel &input) {
      stream.Write(input.compression);
      stream.Write(input.data);
    }
  }; // struct ToStreamFn
  friend Stream;
  auto Comparable() const {
    return std::tie(compression, data);
  }
public:
  Channel() = default;
  Channel(std::vector<U8> data) : data(std::move(data)) {}

  bool operator==(const Channel &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const Channel &other) const {
    return !operator==(other);
  }
  Compression compression = Compression::None;
  std::vector<U8> data;
}; // class Channel

class ChannelData {
  struct FromStreamFn {
    void operator()(Stream &stream, ChannelData &output, const std::map<I16, U32> &channel_info) {
      for (auto [index, length] : channel_info) {
        stream.ReadTo(output.data[index], length);
      }
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const ChannelData &input) {
      for (const auto &[index, channel] : input.data) {
        stream.Write(channel);
      }
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  ChannelData() = default;
  bool operator==(const ChannelData &other) const {
    return data == other.data;
  }
  bool operator!=(const ChannelData &other) const {
    return !operator==(other);
  }
  std::map<I16, Channel> data;
  unsigned Length() const {
    auto output = 0u;
    for (const auto &[index, channel] : data) {
      output += 2 + channel.data.size();
    }
    return output;
  }
  bool Compressed() const {
    for (const auto &[index, channel] : data) {
      if (channel.compression != Compression::None) return true;
    }
    return false;
  }
  void Decompress(const Header &header, const LayerData &layer_data) {
    auto coordinates  = layer_data.coordinates;
    auto row_count    = coordinates.bottom - coordinates.top;
    auto column_count = coordinates.right  - coordinates.left;
    for (auto &[channel, pair] : data) {
      auto &[compression, data] = pair;
      data = llapi::Decompress(
        data,
        row_count,
        column_count,
        header.depth,
        compression
      );
      compression = Compression::None;
    }
  }
  void Compress(Compression compr, unsigned level, unsigned row_count, unsigned column_count, Depth depth) {
    for (auto &[channel, pair] : data) {
      auto &[compression, data] = pair;
      if (compr == Compression::None) {
        continue;
      }
      data = llapi::Compress(
        data,
        row_count,
        column_count,
        depth,
        compr,
        level
      );
      compression = compr;
    }
  }
}; // class ChannelData
namespace detail {
class ConvertChannelDataColorFn {
public:
  ChannelData operator()(ChannelData input, Color input_color, Color output_color) {
    if (input_color == output_color) {
      return input;
    }
    return ToColor(
      FromColor(std::move(input), input_color),
      output_color
    );
  }
private:
  unsigned LengthOf(const ChannelData &input) {
    return input.data.at(0).data.size();
  }
  ChannelData ToGrayscale(ChannelData input) {
    ChannelData output;
    for (auto index = 0u;
              index < LengthOf(input);
              index++)
    {
      output.data[0].data.push_back(
        0.299 * input.data.at(0).data[index] +
        0.587 * input.data.at(1).data[index] +
        0.114 * input.data.at(2).data[index]
      );
    }
    if (input.data.find(-1) != input.data.end()) {
      output.data[-1] = std::move(input.data.at(-1));
    } else {
      output.data[-1] = std::vector<U8>(LengthOf(input), 0xff);
    }
    return output;
  }
  ChannelData FromGrayscale(ChannelData input) {
    ChannelData output;

    output.data[0] = std::move(input.data.at(0));
    output.data[1] = output.data.at(0);
    output.data[2] = output.data.at(0);
    input.data.erase(0);
    if (input.data.find(-1) != input.data.end()) {
      output.data[-1] = std::move(input.data.at(-1));
    } else {
      output.data[-1] = std::vector<U8>(LengthOf(output), 0xff);
    }
    return output;
  }
  ChannelData FromColor(ChannelData input, Color input_color) {
    switch (input_color) {
      case Color::Grayscale : return FromGrayscale(input);
      case Color::Rgb       : return input;
      default: {
        throw Error("UnsupportedColor");
      }
    }
  }
  ChannelData ToColor(ChannelData input, Color output_color) {
    switch (output_color) {
      case Color::Grayscale : return ToGrayscale(input);
      case Color::Rgb       : return input;
      default: {
        throw Error("UnsupportedColor");
      }
    }
  }
}; // class ConvertChannelDataColorFn

class ConvertDepthFn {
public:
  std::vector<U8> operator()(std::vector<U8> input, Depth input_depth, Depth output_depth) const {
    if (input_depth == Depth::Eight && output_depth == Depth::Sixteen) {
      return Convert816(std::move(input));
    }
    if (input_depth == Depth::Eight && output_depth == Depth::ThirtyTwo) {
      return Convert832(std::move(input));
    }
    if (input_depth == Depth::Sixteen && output_depth == Depth::Eight) {
      return Convert168(std::move(input));
    }
    if (input_depth == Depth::Sixteen && output_depth == Depth::ThirtyTwo) {
      return Convert1632(std::move(input));
    }
    if (input_depth == Depth::ThirtyTwo && output_depth == Depth::Eight) {
      return Convert328(std::move(input));
    }
    if (input_depth == Depth::ThirtyTwo && output_depth == Depth::Sixteen) {
      return Convert3216(std::move(input));
    }
    throw Error("UnsupportedDepthcvt");
  }
private:
  template <typename T>
  std::vector<T> &Represent(std::vector<U8> &input) const {
    return reinterpret_cast<std::vector<T> &>(input);
  }
  std::vector<U8> Convert832(std::vector<U8> input) const {
    std::vector<U8> output(input.size() * sizeof(F32));
    for (auto index = 0u;
              index < input.size();
              index++) {
      Represent<F32>(output)[index] = F32(input[index]) / 0xff;
    }
    return output;
  }
  std::vector<U8> Convert816(std::vector<U8> input) const {
    std::vector<U8> output(input.size() * sizeof(U16));
    for (auto index = 0u;
              index < input.size();
              index++) {
      Represent<U16>(output)[index] = U16(input[index]) * (0xffff / 0xff);
    }
    return output;
  }
  std::vector<U8> Convert168(std::vector<U8> input) const {
    assert(input.size() % sizeof(U16) == 0);
    std::vector<U8> output(input.size() / sizeof(U16));
    for (auto index = 0u;
              index < input.size() / sizeof(U16);
              index++) {
      output[index] = (Represent<U16>(input)[index] * 0xff) / 0xffff;
    }
    return output;
  }
  std::vector<U8> Convert1632(std::vector<U8> input) const {
    assert(input.size() % sizeof(U16) == 0);
    std::vector<U8> output((input.size() / sizeof(U16)) * sizeof(F32));
    for (auto index = 0u;
              index < input.size() / sizeof(U16);
              index++) {
      Represent<F32>(output)[index] = F32(Represent<U16>(input)[index]) / 0xffff;
    }
    return output;
  }
  std::vector<U8> Convert328(std::vector<U8> input) const {
    assert(input.size() % sizeof(F32) == 0);
    std::vector<U8> output(input.size() / sizeof(F32));
    for (auto index = 0u;
              index < input.size() / sizeof(F32);
              index++) {
      output[index] =
        std::min(std::max(Represent<F32>(input)[index], 0.0f), 1.0f) * 255.0f + 0.5f;
    }
    return output;
  }
  std::vector<U8> Convert3216(std::vector<U8> input) const {
    assert(input.size() % sizeof(F32) == 0);
    std::vector<U8> output((input.size() / sizeof(F32)) * sizeof(U16));
    for (auto index = 0u;
              index < input.size() / sizeof(F32);
              index++) {
      Represent<U16>(output)[index] =
        std::min(std::max(Represent<F32>(input)[index], 0.0f), 1.0f) * 65535.0f + 0.5f;
    }
    return output;
  }
};
inline constexpr auto ConvertDepth = ConvertDepthFn();

class ConvertChannelDataDepthFn {
public:
  ChannelData operator()(ChannelData input, Depth input_depth, Depth output_depth) {
    if (input.Compressed()) {
      throw Error("CannotBeConverted");
    }
    if (input_depth == output_depth) {
      return input;
    }
    ChannelData output;
    for (auto index = 0u;
              index < input.data.size();
              index++) {
      output.data[index] = ConvertDepth(
        std::move(
          input.data[index].data
        ),
        input_depth,
        output_depth
      );
    }
    return output;
  }
private:
};
}; // namespace detail
inline ChannelData ConvertColor(ChannelData input, Color input_color, Color output_color) {
  return detail::ConvertChannelDataColorFn()(std::move(input), input_color, output_color);
}
inline void ConvertColorInPlace(ChannelData &output, Color input_color, Color output_color) {
  output = ConvertColor(std::move(output), input_color, output_color);
}
inline ChannelData ConvertDepth(ChannelData input, Depth input_depth, Depth output_depth) {
  return detail::ConvertChannelDataDepthFn()(std::move(input), input_depth, output_depth);
}
inline void ConvertDepthInPlace(ChannelData &output, Depth input_depth, Depth output_depth) {
  output = ConvertDepth(std::move(output), input_depth, output_depth);
}
}; // namespace PSD::llapi
