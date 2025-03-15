
#pragma once

#include <psd/structure/main_info/extra_info.h>
#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/main_info/layer_info/channel_data.h>

namespace PSD {

template <typename ChannelDataT>
class LayerInfoTemplate {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Compressor;
  class Decompressor;

  static constexpr Depth::Tp DepthValue = ChannelDataT::DepthValue;
  static constexpr Color::Tp ColorValue = ChannelDataT::ColorValue;

  static constexpr bool IsDecompressed = std::is_same_v<ChannelDataT,           ChannelData<DepthValue, ColorValue>>;
  static constexpr bool IsCompressed   = std::is_same_v<ChannelDataT, CompressedChannelData<DepthValue, ColorValue>>;

  static_assert(IsCompressed || IsDecompressed);

  explicit LayerInfoTemplate() = default;

  bool operator==(const LayerInfoTemplate &other) const {
    return layer_count  == other.layer_count &&
           layer_data   == other.layer_data  &&
           channel_data == other.channel_data;
  }
  bool operator!=(const LayerInfoTemplate &other) const {
    return !operator==(other);
  }

  std::uint16_t             layer_count = 0;
  std::vector<LayerData>    layer_data;
  std::vector<ChannelDataT> channel_data;

private:
  std::uint64_t CalculateContentLength() const {
    if (layer_count == 0) { 
      return 0;
    }
    std::uint64_t output = sizeof layer_count;
    for (auto index = 0u; 
              index < layer_count; 
              index++) {
      output += PSD::LengthCalculator(layer_data   [index]).Calculate();
      output += PSD::LengthCalculator(channel_data [index]).Calculate();
    }
    return output;
  }

}; // LayerInfoTemplate

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
using LayerInfo = LayerInfoTemplate<ChannelData<DepthV, ColorV>>;

template <typename ChannelDataT>
using ToLayerInfo = LayerInfo<ChannelDataT::DepthValue, ChannelDataT::ColorValue>;

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
using CompressedLayerInfo = LayerInfoTemplate<CompressedChannelData<DepthV, ColorV>>;

template <typename ChannelDataT>
using ToCompressedLayerInfo = CompressedLayerInfo<ChannelDataT::DepthValue, ChannelDataT::ColorValue>;

template <typename ChannelDataT>
class LayerInfoTemplate<ChannelDataT>::LengthCalculator {
public:
  explicit LengthCalculator(const LayerInfoTemplate<ChannelDataT> &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + sizeof(std::uint32_t);
  }

private:
  const LayerInfoTemplate<ChannelDataT> &input_;

}; // LayerInfoTemplate<ChannelDataT>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(LayerInfo);
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedLayerInfo);

template <typename ChannelDataT>
class LayerInfoTemplate<ChannelDataT>::Reader {
public:

  static_assert(IsCompressed);

  explicit Reader(Stream &stream) : stream_(stream) {}

  LayerInfoTemplate<ChannelDataT> Read() {
    LayerInfoTemplate<ChannelDataT> output;

    std::uint64_t start = stream_.GetPos();

    if (stream_.Read<std::uint32_t>() == 0) {
      return output;
    }
    output.layer_count = stream_.Read<std::uint16_t>();
    for (auto index = 0u; 
              index < output.layer_count; 
              index++) {
      output.layer_data.push_back(stream_.Read<LayerData>());
    }
    for (auto index = 0u; 
              index < output.layer_count; 
              index++) {
      output.channel_data.push_back(stream_.Read<ChannelDataT>(CrTuple(
        output.layer_data[index] 
      )));
    }
    if ((stream_.GetPos() - start) % 2) {
      stream_.IncPos();
    }
    return output;
  }

private:
  Stream &stream_;

}; // LayerInfoTemplate<ChannelDataT>::Reader

PSD_REGISTER_READER_FOR_BUFFER(CompressedLayerInfo);


template <typename ChannelDataT>
class LayerInfoTemplate<ChannelDataT>::Writer {
public:

  explicit Writer(Stream &stream, const LayerInfoTemplate<ChannelDataT> &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
    if (!input_.layer_count) {
      return;
    }
    stream_.Write(input_.layer_count);
    for (auto index = 0u;
              index < input_.layer_count;
              index++) {
      stream_.Write(input_.layer_data[index]);
    }
    for (auto index = 0u;
              index < input_.layer_count;
              index++) {
      stream_.Write(input_.channel_data[index]);
    }
  }

private:
  Stream &stream_;
  const LayerInfoTemplate<ChannelDataT> &input_;

}; // LayerInfoTemplate<ChannelDataT>::Writer

PSD_REGISTER_WRITER_FOR_BUFFER(LayerInfo);
PSD_REGISTER_WRITER_FOR_BUFFER(CompressedLayerInfo);

template <typename ChannelDataT>
class LayerInfoTemplate<ChannelDataT>::Compressor {
public:

  static_assert(IsDecompressed);

  explicit Compressor(LayerInfoTemplate<ChannelDataT> input) : input_(std::move(input)) {}

  ToCompressedLayerInfo<ChannelDataT> Compress(
    std::vector<std::map<std::int16_t, Compression::Tp>> compression
  ) {
    ToCompressedLayerInfo<ChannelDataT> output;

    output.layer_count  = std::move(input_.layer_count);
    output.layer_data   = std::move(input_.layer_data);

    for (auto index = 0u;
              index < output.layer_count;
              index++) {
      output.channel_data.push_back(
        PSD::Compressor(input_.channel_data).Compress(
          compression       [index],
          output.layer_data [index]
        )
      );
    }
    return output;
  }
  ToCompressedLayerInfo<ChannelDataT> Compress(Compression::Tp compression) {
    ToCompressedLayerInfo<ChannelDataT> output;

    output.layer_count  = std::move(input_.layer_count);
    output.layer_data   = std::move(input_.layer_data);

    for (auto index = 0u;
              index < output.layer_count;
              index++) {
      output.channel_data.push_back(
        PSD::Compressor(input_.channel_data[index]).Compress(
          compression,
          output.layer_data[index]
        )
      );
    }
    return output;
  }

private:
  LayerInfoTemplate<ChannelDataT> input_;

}; // LayerInfoTemplate<ChannelDataT>::Compressor

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(LayerInfo);

template <typename ChannelDataT>
class LayerInfoTemplate<ChannelDataT>::Decompressor {
public:

  static_assert(IsCompressed);

  explicit Decompressor(const LayerInfoTemplate<ChannelDataT> &input) : input_(input) {}

  ToLayerInfo<ChannelDataT> Decompress(const Header &header) const {
    ToLayerInfo<ChannelDataT> output;

    output.layer_count = std::move(input_.layer_count);
    output.layer_data  = std::move(input_.layer_data);

    for (auto index = 0u;
              index < output.layer_count;
              index++) {
      output.channel_data.push_back(
        PSD::Decompressor(input_.channel_data[index]).Decompress(
          header,
          output.layer_data[index]
        )
      );
    }
    return output;
  }

private:
  const LayerInfoTemplate<ChannelDataT> &input_;

}; // LayerInfoTemplate<ChannelDataT>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedLayerInfo);
  
}; // PSD
