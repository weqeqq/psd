
#pragma once

#include "psd/llapi/structure/info/info_extra/layer16.h"
#include <psd/llapi/structure/info/info_extra.h>
#include <psd/llapi/structure/info/layer_info.h>
#include <psd/llapi/structure/info/global_info.h>

namespace PSD::llapi {
//
class Info {
  struct FromStreamFn {
    void operator()(Stream &stream, Info &output) {
      auto length = stream.Read<U32>();
      auto start  = stream.Pos();

      stream.ReadTo(output.layer_info);
      // stream += (start - stream.Pos()) % 4;
      stream.ReadTo(output.global_info);
      stream.ReadTo(output.extra_info, length - (stream.Pos() - start));
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Info &input) {
      stream.Write(input.ContentLength());
      stream.Write(input.layer_info);
      stream.Write(input.global_info);
      stream.Write(input.extra_info);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Info() = default;
  Info(LayerInfo layer_info) : layer_info(std::move(layer_info)) {}

  LayerInfo  layer_info;
  GlobalInfo global_info;
  InfoExtra  extra_info;

  void EnsureSupported(const Header &header) {
    if (extra_info.Exists<Layer16>()) {
      layer_info = std::move(extra_info.At<Layer16>().data);
    }
    if (extra_info.Exists<Layer32>()) {
      layer_info = std::move(extra_info.At<Layer32>().data);
    }
  }
  void Decompress(const Header &header) {
    if (extra_info.Exists<Layer16>()) {
      return extra_info.At<Layer16>().data.Decompress(header);
    }
    if (extra_info.Exists<Layer32>()) {
      return extra_info.At<Layer32>().data.Decompress(header);
    }
    layer_info.Decompress(header);
  }
  void Compress(Compression compression, unsigned level, const Header &header) {
    if (extra_info.Exists<Layer16>()) {
      return extra_info.At<Layer16>().data.Compress(compression, level, header);
    }
    if (extra_info.Exists<Layer32>()) {
      return extra_info.At<Layer32>().data.Compress(compression, level, header);
    }
    layer_info.Compress(compression, level, header);
  }
  unsigned Length() const {
    return 4 + ContentLength();
  }
private:
  U32 ContentLength() const {
    return layer_info.Length() + global_info.Length() + extra_info.Length();
  }
};
inline void ConvertColorInPlace(Info &output, Color input_color, Color output_color) {
  ConvertColorInPlace(output.layer_info, input_color, output_color);
}
inline void ConvertDepthInPlace(Info &output, Depth output_depth) {
  const auto current_depth = [&]() {
    if (output.extra_info.Exists<Layer16>()) return Depth::Sixteen;
    if (output.extra_info.Exists<Layer32>()) return Depth::ThirtyTwo;
    return Depth::Eight;
  }();
  if (current_depth == output_depth) {
    return;
  }
  LayerInfo source_data;
  switch (current_depth) {
    case Depth::Sixteen:
      source_data = std::move(output.extra_info.At<Layer16>().data);
      output.extra_info.Erase<Layer16>();
      break;
    case Depth::ThirtyTwo:
      source_data = std::move(output.extra_info.At<Layer32>().data);
      output.extra_info.Erase<Layer32>();
      break;
    case Depth::Eight:
      source_data = std::exchange(output.layer_info, LayerInfo());
      break;
    default:
      throw Error("Unsupported current depth");
  }
  auto converted = ConvertDepth(std::move(source_data), current_depth, output_depth);
  switch (output_depth) {
    case Depth::Eight:
      output.layer_info = std::move(converted);
      break;
    case Depth::Sixteen:
      output.extra_info.Insert(Layer16(std::move(converted)));
      break;
    case Depth::ThirtyTwo:
      output.extra_info.Insert(Layer32(std::move(converted)));
      break;
    default:
      throw Error("Unsupported target depth");
  }
}
inline Info ConvertColor(Info input, Color input_color, Color output_color) {
  Info output = std::move(input);
  ConvertColorInPlace(output, input_color, output_color);
  return output;
}
inline Info ConvertDepth(Info input, Depth output_depth) {
  Info output = std::move(input);
  ConvertDepthInPlace(output, output_depth);
  return output;
}
inline Info Decompress(Info input, const Header &header) {
  input.Decompress(header);
  return input;
}
inline void DecompressInPlace(Info &output, const Header &header) {
  output = Decompress(std::move(output), header);
}
inline Info Compress(Info input, const Header &header, Compression compression, unsigned level) {
  input.Compress(compression, level, header);
  return input;
}
inline void CompressInPlace(Info &output, const Header &header, Compression compression, unsigned level) {
  output = Compress(std::move(output), header, compression, level);
}
}; // namespace PSD::llapi
