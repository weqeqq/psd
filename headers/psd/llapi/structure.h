
#pragma once

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
      stream.ReadTo(output.image);
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
  Header       header;
  ColorInfo    color_info;
  ResourceInfo resource_info;
  Info         info;
  Image        image;
  void Decompress() {
    info.layer_info.Decompress();
    image.Decompress(header.row_count, header.column_count);
  }
  void Compress(Compression compression) {
    info.layer_info.Compress(compression);
    image.Compress(compression, header.row_count, header.column_count);
  }
}; // class Structure
}; // namespace PSD::llapi
