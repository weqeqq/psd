
#pragma once

#include "psd/llapi/structure/header.h"
#include <cassert>
#include <psd/llapi/stream.h>
#include <psd/llapi/structure/info/layer_info/channel_data.h>

namespace PSD::llapi {
//
class Image {
  struct FromStreamFn {
    void operator()(Stream &stream, Image &output) {
      stream.ReadTo(output.compression);
      stream.ReadTo(output.data, stream.Length() - stream.Pos());
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Image &input) {
      stream.Write(input.compression);
      stream.Write(input.data);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Image() = default;
  Image(std::vector<U8> data) : data(std::move(data)) {}

  Compression     compression = Compression::None;
  std::vector<U8> data;

  void Decompress(const llapi::Header &header) {
    if (data.empty() || compression == Compression::None) {
      return;
    }
    data = llapi::Decompress(
      data,
      header.row_count * header.channel_count,
      header.column_count,
      header.depth,
      compression
    );
    compression = Compression::None;
  }
  void Compress(Compression compr, unsigned level, const Header &header) {
    if (data.empty() || compr == Compression::None) {
      return;
    } else {
      data = llapi::CompressDefault(
        data,
        header.row_count * header.channel_count,
        header.column_count,
        header.depth,
        level
      );
      compression = Compression::Default;
    }
  }
}; // class Image
namespace detail {
//
class ConvertImageDepthFn {
public:
  Image operator()(Image input, Depth input_depth, Depth output_depth) {
    return ConvertDepth(input.data, input_depth, output_depth);
  }
};
class ConvertImageColorFn {
public:
  Image operator()(Image input, Color input_color, Color output_color) const {
    if (input_color == output_color) {
      return input;
    }
    return ToColor(
      FromColor(std::move(input), input_color),
      output_color
    );
  }
private:
  Image ToGrayscale(Image input) const {
    Image output(std::vector<U8>(input.data.size() / 3));
    for (auto index = 0u;
              index < input.data.size() / 3;
              index++) {
      auto offset = index * 3;
      output.data[index] =
        0.299 * input.data[offset + 0] +
        0.587 * input.data[offset + 1] +
        0.114 * input.data[offset + 2];
    }
    return output;
  }
  Image FromGrayscale(Image input) const {
    Image output(std::vector<U8>(input.data.size() * 3));
    for (auto index = 0u;
              index < input.data.size() * 3;
              index++) {
      auto offset = index / 3;
      output.data[index + 0] = input.data[offset];
      output.data[index + 1] = input.data[offset];
      output.data[index + 2] = input.data[offset];
    }
    return output;
  }
  Image FromColor(Image input, Color input_color) const {
    switch (input_color) {
      case Color::Grayscale : return FromGrayscale(input);
      case Color::Rgb       : return input;
      default:
      throw Error("false");
    }
  }
  Image ToColor(Image input, Color output_color) const {
    switch (output_color) {
      case Color::Grayscale : return ToGrayscale(input);
      case Color::Rgb       : return input;
      default:
      throw Error("false");
    }
  }
};
}; // namespace detail
inline Image ConvertColor(
  Image input, Color input_color, Color output_color
) {
  return detail::ConvertImageColorFn()(std::move(input), input_color, output_color);
}
inline Image ConvertDepth(
  Image input, Depth input_depth, Depth output_depth
) {
  return detail::ConvertImageDepthFn()(std::move(input), input_depth, output_depth);
}
inline void ConvertColorInPlace(
  Image &output, Color input_color, Color output_color
) {
  output = ConvertColor(std::move(output), input_color, output_color);
}
inline void ConvertDepthInPlace(
  Image &output, Depth input_depth, Depth output_depth
) {
  output = ConvertDepth(std::move(output), input_depth, output_depth);
}
}; // namespace PSD::llapi
