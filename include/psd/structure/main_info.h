
#pragma once

#include <psd/structure/main_info/extra_info.h>
#include <psd/structure/main_info/global_info.h>
#include <psd/structure/main_info/layer_info.h>

namespace PSD {

template <typename LayerInfoT>
class MainInfoTemplate {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Compressor;
  class Decompressor;

  static constexpr Depth::Tp DepthValue = LayerInfoT::DepthValue;
  static constexpr Color::Tp ColorValue = LayerInfoT::ColorValue;

  static constexpr bool IsCompressed   = LayerInfoT::IsCompressed;
  static constexpr bool IsDecompressed = LayerInfoT::IsDecompressed;

  static_assert(IsCompressed || IsDecompressed);

  explicit MainInfoTemplate() = default;

  bool operator==(const MainInfoTemplate<LayerInfoT> &other) const {
    return layer_info  == other.layer_info  &&
           global_info == other.global_info &&
           extra_info  == other.extra_info;
  }
  bool operator!=(const MainInfoTemplate<LayerInfoT> &other) const {
    return !operator==(other);
  }

  LayerInfoT layer_info;
  GlobalInfo global_info;
  ExtraInfo  extra_info;

private:
  std::uint64_t CalculateContentLength() const {
    return PSD::LengthCalculator(layer_info)  .Calculate() +
           PSD::LengthCalculator(global_info) .Calculate() +
           PSD::LengthCalculator(extra_info)  .Calculate();
  }

}; // MainInfo

template <Depth::Tp DepthV, Color::Tp ColorV>
using MainInfo = MainInfoTemplate<LayerInfo<DepthV, ColorV>>;

template <typename LayerInfoT>
using ToMainInfo = MainInfo<LayerInfoT::DepthValue, LayerInfoT::ColorValue>;

template <Depth::Tp DepthV, Color::Tp ColorV>
using CompressedMainInfo = MainInfoTemplate<CompressedLayerInfo<DepthV, ColorV>>;

template <typename LayerInfoT> 
using ToCompressedMainInfo = CompressedMainInfo<LayerInfoT::DepthValue, LayerInfoT::ColorValue>;

template <typename LayerInfoT>
class MainInfoTemplate<LayerInfoT>::LengthCalculator {
public:

  explicit LengthCalculator(const MainInfoTemplate<LayerInfoT> &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + 4;
  }

private:
  const MainInfoTemplate<LayerInfoT> &input_;

}; // MainInfoTemplate<LayerInfoT>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(MainInfo);
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedMainInfo);

template <typename LayerInfoT> 
class MainInfoTemplate<LayerInfoT>::Reader {
public:

  static_assert(IsCompressed);

  explicit Reader(Stream &stream) : stream_(stream) {}

  MainInfoTemplate<LayerInfoT> Read() {
    MainInfoTemplate<LayerInfoT> output;

    std::uint64_t length = stream_.Read<std::uint32_t>();
    std::uint64_t start  = stream_.GetPos();

    output.layer_info  = stream_.Read<LayerInfoT> ();
    output.global_info = stream_.Read<GlobalInfo> ();
    output.extra_info  = stream_.Read<ExtraInfo>  (CrTuple(
      length - (stream_.GetPos() - start)
    ));
    return output;
  }

private:
  Stream &stream_;

}; // MainInfoTemplate<LayerInfoT>::Reader

PSD_REGISTER_READER_FOR_BUFFER(CompressedMainInfo);

template <typename LayerInfoT>
class MainInfoTemplate<LayerInfoT>::Writer {
public:

  explicit Writer(Stream &stream, const MainInfoTemplate<LayerInfoT> &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
    stream_.Write(input_.layer_info);
    stream_.Write(input_.global_info);
    stream_.Write(input_.extra_info);
  }

private:
  Stream &stream_;
  const MainInfoTemplate<LayerInfoT> &input_;

}; // MainInfoTemplate<LayerInfoT>::Writer 

PSD_REGISTER_WRITER_FOR_BUFFER(MainInfo);
PSD_REGISTER_WRITER_FOR_BUFFER(CompressedMainInfo);

template <typename LayerInfoT> 
class MainInfoTemplate<LayerInfoT>::Compressor {
public:

  static_assert(IsDecompressed);

  explicit Compressor(MainInfoTemplate<LayerInfoT> input) : input_(std::move(input)) {}

  ToCompressedMainInfo<LayerInfoT> Compress(
    std::vector<std::map<std::int16_t, Compression::Tp>> compression
  ) {
    ToCompressedMainInfo<LayerInfoT> output;

    output.layer_info  = PSD::Compressor(input_.layer_info).Compress(compression);
    output.global_info = std::move(input_.global_info);
    output.extra_info  = std::move(input_.extra_info);

    return output;
  }
  ToCompressedMainInfo<LayerInfoT> Compress(Compression::Tp compression) {
    ToCompressedMainInfo<LayerInfoT> output;

    output.layer_info  = PSD::Compressor(input_.layer_info).Compress(compression);
    output.global_info = std::move(input_.global_info);
    output.extra_info  = std::move(input_.extra_info);

    return output; 
  }

private:
  MainInfoTemplate<LayerInfoT> input_;

}; // MainInfoTemplate<LayerInfoT>::Compressor

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(MainInfo);

template <typename LayerInfoT> 
class MainInfoTemplate<LayerInfoT>::Decompressor {
public:

  static_assert(IsCompressed);

  explicit Decompressor(MainInfoTemplate<LayerInfoT> input) : input_(std::move(input)) {}

  ToMainInfo<LayerInfoT> Decompress(Header header) const {
    ToMainInfo<LayerInfoT> output;

    output.layer_info  = PSD::Decompressor(input_.layer_info).Decompress(header);
    output.global_info = std::move(input_.global_info);
    output.extra_info  = std::move(input_.extra_info);

    return output;
  };

private:
  MainInfoTemplate<LayerInfoT> input_;

}; // MainInfoTemplate<LayerInfoT>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedMainInfo);

}; // PSD
