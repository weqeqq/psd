
#pragma once

#include <psd/structure/header.h>
#include <psd/structure/color_info.h>
#include <psd/structure/resource_info.h>
#include <psd/structure/main_info.h>
#include <psd/structure/final_image.h>

namespace PSD {

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate {
public:

  static_assert(MainInfoT::DepthValue == FinalImageT::DepthValue);
  static_assert(MainInfoT::ColorValue == FinalImageT::ColorValue);

  static_assert(MainInfoT::IsCompressed   == FinalImageT::IsCompressed);
  static_assert(MainInfoT::IsDecompressed == FinalImageT::IsDecompressed);

  class LengthCalculator;
  class Reader;
  class Writer;

  class Compressor;
  class Decompressor;

  static constexpr Depth::Tp DepthValue = MainInfoT::DepthValue;
  static constexpr Color::Tp ColorValue = MainInfoT::ColorValue;

  static constexpr bool IsCompressed   = MainInfoT::IsCompressed;
  static constexpr bool IsDecompressed = MainInfoT::IsDecompressed;

  static_assert(IsCompressed || IsDecompressed);

  bool operator==(const StructureTemplate<MainInfoT, FinalImageT> &other) const {
    return header        == other.header        &&
           color_info    == other.color_info    &&
           resource_info == other.resource_info &&
           main_info     == other.main_info     &&
           final_image   == other.final_image;
  }
  bool operator!=(const StructureTemplate<MainInfoT, FinalImageT> &other) const {
    return !operator==(other);
  }

  Header       header;
  ColorInfo    color_info;
  ResourceInfo resource_info;
  MainInfoT    main_info;
  FinalImageT  final_image;

}; // StructureTemplate

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
using Structure = StructureTemplate<MainInfo<DepthV, ColorV>, FinalImage<DepthV, ColorV>>;

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
using CompressedStructure = StructureTemplate<CompressedMainInfo<DepthV, ColorV>, CompressedFinalImage<DepthV, ColorV>>;

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate<MainInfoT, FinalImageT>::LengthCalculator {
public:

  explicit LengthCalculator(const StructureTemplate<MainInfoT, FinalImageT> &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return PSD::LengthCalculator(input_.header)        .Calculate() + 
           PSD::LengthCalculator(input_.color_info)    .Calculate() +
           PSD::LengthCalculator(input_.resource_info) .Calculate() +
           PSD::LengthCalculator(input_.main_info)     .Calculate() +
           PSD::LengthCalculator(input_.final_image)   .Calculate();
  }

private:
  const StructureTemplate<MainInfoT, FinalImageT> &input_;

}; // StructureTemplate<MainInfoT, FinalImageT>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(Structure);
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedStructure);

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate<MainInfoT, FinalImageT>::Reader {
public:

  static_assert(IsCompressed);

  explicit Reader(Stream &stream) : stream_(stream) {}

  StructureTemplate<MainInfoT, FinalImageT> Read() {
    StructureTemplate<MainInfoT, FinalImageT> output;
    output.header        = stream_.Read<Header>();
    output.color_info    = stream_.Read<ColorInfo>();
    output.resource_info = stream_.Read<ResourceInfo>();
    output.main_info     = stream_.Read<MainInfoT>();
    output.final_image   = stream_.Read<FinalImageT>();
    return output;
  }

private:
  Stream &stream_;

}; // StructureTemplate<MainInfoT, FinalImageT>::Reader

PSD_REGISTER_READER_FOR_BUFFER(CompressedStructure);

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate<MainInfoT, FinalImageT>::Writer {
public:

  explicit Writer(Stream &stream, const StructureTemplate<MainInfoT, FinalImageT> &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(input_.header);
    stream_.Write(input_.color_info);
    stream_.Write(input_.resource_info); 
    stream_.Write(input_.main_info);
    stream_.Write(input_.final_image);
  }

private:
  Stream &stream_;
  const StructureTemplate<MainInfoT, FinalImageT> &input_;

}; // StructureTemplate<MainInfoT, FinalImageT>::Writer

PSD_REGISTER_WRITER_FOR_BUFFER(CompressedStructure);

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate<MainInfoT, FinalImageT>::Compressor {
public:

  static_assert(IsDecompressed);

  explicit Compressor(StructureTemplate<MainInfoT, FinalImageT> input) : input_(std::move(input)) {}

  CompressedStructure<DepthValue, ColorValue> Compress(Compression::Tp compression = Compression::RLE) {
    CompressedStructure<DepthValue, ColorValue> output;

    output.header        = std::move(input_.header);
    output.color_info    = std::move(input_.color_info);
    output.resource_info = std::move(input_.resource_info);
    output.main_info     = PSD::Compressor(input_.main_info)   .Compress(compression);
    output.final_image   = PSD::Compressor(input_.final_image) .Compress(compression);

    return output;
  }

private:
  StructureTemplate<MainInfoT, FinalImageT> input_;

}; // StructureTemplate<MainInfoT, FinalImageT>::Compressor

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(Structure);

template <typename MainInfoT, typename FinalImageT>
class StructureTemplate<MainInfoT, FinalImageT>::Decompressor {
public:

  static_assert(IsCompressed);

  explicit Decompressor(CompressedStructure<DepthValue, ColorValue> input) : input_(std::move(input)) {}

  Structure<DepthValue, ColorValue> Decompress() { 
    Structure<DepthValue, ColorValue> output;

    output.header        = std::move(input_.header);
    output.color_info    = std::move(input_.color_info);
    output.resource_info = std::move(input_.resource_info);
    output.main_info     = PSD::Decompressor(input_.main_info)   .Decompress(output.header);
    output.final_image   = PSD::Decompressor(input_.final_image) .Decompress(output.header);

    return output;
  }

private:
  CompressedStructure<DepthValue, ColorValue> input_;

}; // StructureTemplate<MainInfoT, FinalImageT>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedStructure);
  
}; // PSD
