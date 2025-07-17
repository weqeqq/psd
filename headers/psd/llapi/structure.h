
#pragma once

#include "psd/llapi/structure/info/layer_info.h"
#include <psd/llapi/structure/header.h>
#include <psd/llapi/structure/color_info.h>
#include <psd/llapi/structure/resource_info.h>
#include <psd/llapi/structure/info.h>
#include <psd/llapi/structure/image.h>

namespace PSD::llapi {
//
class Structure {
  struct FromStreamFn {
    void operator()(Stream &stream, Structure &output) {
      stream.ReadTo(output.header);
      stream.ReadTo(output.color_info);
      stream.ReadTo(output.resource_info);
      stream.ReadTo(output.info);
      // stream.ReadTo(output.image);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Structure &input) {
      stream.Write(input.header);
      stream.Write(input.color_info);
      stream.Write(input.resource_info);
      stream.Write(input.info);
      stream.Write(input.image);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Structure() = default;
  Structure(
    Header header,
    ResourceInfo resource_info,
    Info info,
    Image image
  ) : header(std::move(header))
    , resource_info(std::move(resource_info))
    , info(std::move(info))
    , image(std::move(image)) {}

  Header       header;
  ColorInfo    color_info;
  ResourceInfo resource_info;
  Info         info;
  Image        image;
  void Decompress() {
    info.Decompress(header);
    image.Decompress(header);
  }
  void Compress(Compression compression, unsigned level) {
    info.layer_info.Compress(compression, level, header);
    image.Compress(compression, level, header);
  }
}; // class Structure

inline Structure ConvertColor(Structure input, Color color) {
  ConvertColorInPlace(input.info  , input.header.color , color);
  ConvertColorInPlace(input.image , input.header.color , color);
  input.header.color = color;
  return input;
}
inline Structure ConvertDepth(Structure input, Depth depth) {
  ConvertDepthInPlace(input.info  , depth);
  ConvertDepthInPlace(input.image , input.header.depth, depth);
  input.header.depth = depth;
  return input;
}
inline void ConvertColorInPlace(Structure &output, Color color) {
  output = ConvertColor(std::move(output), color);
}
inline void ConvertDepthInPlace(Structure &output, Depth depth) {
  output = ConvertDepth(std::move(output), depth);
}
inline Structure Decompress(Structure input) {
  DecompressInPlace(input.info, input.header);
  input.image.Decompress(input.header);
  return input;
}
inline void DecompressInPlace(Structure &output) {
  output = Decompress(std::move(output));
}
inline void CompressInPlace(Structure &output, Compression compression, unsigned level = 6) {
  output.Compress(compression, level);
}
inline Structure Compress(Structure input, Compression compression, unsigned level = 6) {
  Structure output = std::move(input);
  CompressInPlace(output, compression, level);
  return output;
}

inline Structure StructureFrom(const std::filesystem::path &input) {
  return Stream(input).Read<Structure>();
}
inline Structure StructureFrom(std::vector<U8> input) {
  return Stream(input).Read<Structure>();
}
inline void DumpStructure(const Structure &input, const std::filesystem::path &output) {
  Stream stream;
  stream.Write(input);
  stream.Dump(output);
}
inline void DumpStructure(const Structure &input, std::vector<U8> &output) {
  Stream stream;
  stream.Write(input);
  stream.Dump(output);
}
}; // namespace PSD::llapi
