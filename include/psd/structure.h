
#pragma once

#include <psd/structure/header.h>
#include <psd/structure/color_info.h>
#include <psd/structure/resource_info.h>
#include <psd/structure/main_info.h>
#include <psd/structure/final_image.h>

namespace PSD {

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor,
          bool DataState   = Decompressed>
class Structure {
public:
  class LengthCalculator {
  public:
    explicit LengthCalculator(const Structure &input) : input_(input) {}

    std::uint64_t Calculate() const {
      return PSD::LengthCalculator(input_.header)        .Calculate() + 
             PSD::LengthCalculator(input_.color_info)    .Calculate() + 
             PSD::LengthCalculator(input_.resource_info) .Calculate() + 
             PSD::LengthCalculator(input_.main_info)     .Calculate() + 
             PSD::LengthCalculator(input_.final_image)   .Calculate();
    }
  private:
    const Structure &input_;
  };
  class Reader {
  public:
    explicit Reader(Stream &stream) : stream_(stream) {}

    auto Read() {
      Structure output;

      output.header        = stream_.Read<decltype(output.header)>        ();
      output.color_info    = stream_.Read<decltype(output.color_info)>    ();
      output.resource_info = stream_.Read<decltype(output.resource_info)> ();
      output.main_info     = stream_.Read<decltype(output.main_info)>     ();
      output.final_image   = stream_.Read<decltype(output.final_image)>   ();

      return output;
    }
  private:
    Stream &stream_;
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const Structure &input) : stream_(stream), input_(input) {}

    void Write() {
      stream_.Write(input_.header);
      stream_.Write(input_.color_info);
      stream_.Write(input_.resource_info);
      stream_.Write(input_.main_info);
      stream_.Write(input_.final_image);
    }
  private:
    Stream &stream_; const Structure &input_;
  };
  class Compressor {
  public:
    explicit Compressor(const Structure &input) : input_(input) {}

    using Output = Structure<
      DepthV, 
      ColorV, 
      Compressed>;

    auto Compress(Compression::Tp compression = Compression::RLE) {
      Output output;

      output.header        = input_.header;
      output.color_info    = input_.color_info;
      output.resource_info = input_.resource_info;
      output.main_info     = PSD::Compressor(input_.main_info)   .Compress(compression);
      output.final_image   = PSD::Compressor(input_.final_image) .Compress(compression);

      return output;
    }
  private:
    const Structure &input_;
  };
  class Decompressor {
  public:
    explicit Decompressor(const Structure &input) : input_(input) {}

    using Output = Structure<
      DepthV, 
      ColorV, 
      Decompressed>;

    auto Decompress() {
      Output output;

      output.header        = input_.header;
      output.color_info    = input_.color_info;
      output.resource_info = input_.resource_info;
      output.main_info     = PSD::Decompressor(input_.main_info)   .Decompress();
      output.final_image   = PSD::Decompressor(input_.final_image) .Decompress(output.header);

      return output;
    }
  private:
    const Structure &input_;
  };
  using UsedMainInfo   = MainInfo   <DepthV, ColorV, DataState>;
  using UsedFinalImage = FinalImage <DepthV, ColorV, DataState>;

  bool operator==(const Structure &other) const {
    return header        == other.header        &&
           color_info    == other.color_info    &&
           resource_info == other.resource_info &&
           main_info     == other.main_info     &&
           final_image   == other.final_image;
  }
  bool operator!=(const Structure &other) const {
    return !operator==(other); 
  }

  Header         header;
  ColorInfo      color_info;
  ResourceInfo   resource_info;
  UsedMainInfo   main_info;
  UsedFinalImage final_image;
};
template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor>
using CompStructure = Structure<DepthV, ColorV, Compressed>;

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompStructure);
PSD_REGISTER_READER_FOR_BUFFER(CompStructure);
PSD_REGISTER_WRITER_FOR_BUFFER(CompStructure);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompStructure);
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(Structure);

};
