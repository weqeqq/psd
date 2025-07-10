
#pragma once

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

  Compression     compression;
  std::vector<U8> data;
  void Decompress(unsigned row_count, unsigned column_count) {
    if (data.empty()) {
      return;
    }
    switch (compression) {
      case Compression::None    : {break;}
      case Compression::Default : data = llapi::Decompress(data, row_count * 3, column_count);
    }
    compression = Compression::None;
  }
  void Compress(Compression ocompression, unsigned row_count, unsigned column_count) {
    switch (ocompression) {
      case Compression::None    : {break;}
      case Compression::Default : throw Error("Unsupported");
    }
    compression = ocompression;
  }
}; // class Image
}; // namespace PSD::llapi
