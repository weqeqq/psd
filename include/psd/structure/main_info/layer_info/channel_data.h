
#pragma once

#include <psd/core/compressor.h>
#include <psd/core/decompressor.h>

#include <image/convertor/sequence.h>

#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/header.h>

#include <psd/core/type/error.h>
#include <psd/core/type/depth.h>
#include <psd/core/type/color.h>

#include <image/buffer.h>

namespace PSD {

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
class CompressedChannelData {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Decompressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  template <Depth::Tp, Color::Tp>
  friend class ChannelData;

  explicit CompressedChannelData() = default;

  bool operator==(const CompressedChannelData &other) const {
    for (const auto &[channel_id, compression] : compression_) {
      if (compression != other.compression_.at(channel_id)) {
        return false;
      }
    }
    for (const auto &[channel_id, channel_data] : data_) {
      if (channel_data != other.data_.at(channel_id)) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const CompressedChannelData &other) const {
    return !operator==(other);
  }

  std::uint64_t GetContentLength(std::int16_t channel_id) {
    return PSD::LengthCalculator(Compression::Tp()).Calculate() + data_.at(channel_id).size();
  }

private:
  std::map<std::int16_t, Compression::Tp>           compression_;
  std::map<std::int16_t, std::vector<std::uint8_t>> data_;

}; // CompressedChannelData

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
class ChannelData {
public:

  class LengthCalculator;
  class Writer;

  class Compressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  explicit ChannelData() = default;

  std::map<std::int16_t, Image::Buffer<DepthV, Color::Grayscale>> data;

}; // ChannelData

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::LengthCalculator {
public:
  LengthCalculator(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  std::uint64_t Calculate() const { 
    std::uint64_t output = 0;

    for (const auto &[channel_id, channel_data] : input_.data_) {
      output += PSD::LengthCalculator(Compression::Tp()).Calculate() + channel_data.size();
    }
    return output;
  };
  
private:
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // CompressedChannelData<DepthV, ColorV>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  CompressedChannelData<DepthV, ColorV> Read(const LayerData &layer_data) {
    CompressedChannelData<DepthV, ColorV> output;

    for (const auto &[channel_id, channel_length] : layer_data.channel_info) {
      output.compression_ [channel_id] = stream_.Read<Compression::Tp>();
      output.data_        [channel_id] = stream_.Read<std::uint8_t>(
        channel_length - PSD::LengthCalculator(Compression::Tp()).Calculate()
      );
    }
    return output;
  }

private:
  Stream &stream_;

}; // CompressedChannelData<DepthV, ColorV>

PSD_REGISTER_READER_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV> 
class CompressedChannelData<DepthV, ColorV>::Writer {
public:
  explicit Writer(Stream &stream, const CompressedChannelData<DepthV, ColorV> &input) : stream_(stream), input_(input) {}

  void Write() {
    auto start = stream_.GetPos();

    for (const auto &[channel_id, channel_data] : input_.data_) {
      stream_.Write(input_.compression_.at(channel_id));
      stream_.Write(channel_data);
    }
    stream_.Write(std::vector<std::uint8_t>(
      PSD::LengthCalculator(input_).Calculate() - (stream_.GetPos() - start), 0
    ));
  }
private:
  Stream &stream_;
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // CompressedChannelData<DepthV, ColorV>::Writer

PSD_REGISTER_WRITER_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::Decompressor {
public:

  explicit Decompressor(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelData<DepthV, ColorV> Decompress(const Header &header, const LayerData &layer_data) const {
    ChannelData<DepthV, ColorV> output;

    std::uint64_t row_count    = layer_data.rectangle.bottom - layer_data.rectangle.top;
    std::uint64_t column_count = layer_data.rectangle.right  - layer_data.rectangle.left;


    for (const auto &[channel_id, channel_data] : input_.data_) {

      std::vector<std::uint8_t> decompressed;

      if (input_.compression_.at(channel_id) == Compression::RAW) {
        decompressed = channel_data;
      } else {
        decompressed = PSD::Decompressor(channel_data).Decompress(
          input_.compression_.at(channel_id),
          row_count,
          column_count
        );
      }
      #define CONVERT(DepthVa)                                                                                         \
        (header.depth == DepthVa) {                                                                                    \
          output.data[channel_id] = Image::DepthConvertor(Image::SequenceConvertor(decompressed).template ToBuffer< \
            DepthVa,                                                                                                   \
            Color::Grayscale,                                                                                          \
            Image::Endianness::Big>(row_count, column_count)                                                           \
          ).template Convert<DepthV>();                                                                                \
        }
      if CONVERT(Depth::Eight)     else 
      if CONVERT(Depth::Sixteen)   else 
      if CONVERT(Depth::ThirtyTwo) else 
      throw std::runtime_error("unsupported depth");

      #undef CONVERT
    }
    return output;
  }

private:
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // CompressedChannelData<DepthV, ColorV>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV> 
class ChannelData<DepthV, ColorV>::LengthCalculator {
public:
  explicit LengthCalculator(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  std::uint64_t Calculate() const {
    std::uint64_t output = 0;

    for (const auto &[channel_id, channel_buffer] : input_.data) {
      output += PSD::LengthCalculator(Compression::Tp()).Calculate() + channel_buffer.GetBCount(); 
    }
    return output;
  }

private:
  const ChannelData<DepthV, ColorV> &input_;

}; // ChannelData<DepthV, ColorV>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(ChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelData<DepthV, ColorV>::Writer {
public:
  explicit Writer(Stream &stream, const ChannelData<DepthV, ColorV> &input) : stream_(stream), input_(input) {}

  void Write() {
    auto start = stream_.GetPos();

    for (const auto &[channel_id, channel_buffer] : input_.data) {
      stream_.Write(Compression::RAW);
      stream_.Write(Image::SequenceConvertor(channel_buffer).template ToSequence<Image::Endianness::Big>());
    }
    stream_.Write(std::vector<std::uint8_t>(
      PSD::LengthCalculator(input_).Calculate() - (stream_.GetPos() - start), 0
    ));
  }

private:
  Stream &stream_;
  const ChannelData<DepthV, ColorV> &input_;

}; // ChannelData<DepthV, ColorV>::Reader

PSD_REGISTER_WRITER_FOR_BUFFER(ChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelData<DepthV, ColorV>::Compressor {
public:

  explicit Compressor(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  CompressedChannelData<DepthV, ColorV> Compress(
    std::map<std::int16_t, Compression::Tp> compression, const LayerData &layer_data
  ) const {
    CompressedChannelData<DepthV, ColorV> output;

    output.compression_ = compression;

    for (const auto &[channel_id, channel_data] : input_.data) {
      output.data_[channel_id] = PSD::Compressor(Image::SequenceConvertor(channel_data).ToSequence()).Compress(
        compression.at(channel_id),
        layer_data.rectangle.bottom - layer_data.rectangle.top,
        layer_data.rectangle.right - layer_data.rectangle.left
      );
    }
    return output;
  }
  CompressedChannelData<DepthV, ColorV> Compress(
    Compression::Tp compression, const LayerData &layer_data
  ) const {
    std::map<std::int16_t, Compression::Tp> compression_map;
    for (const auto &[channel_id, channel_data] : input_.data) {
      compression_map[channel_id] = compression;
    }
    return Compress(compression_map, layer_data);
  }

private:
  const ChannelData<DepthV, ColorV> &input_;

}; // ChannelData<DepthV, ColorV>

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(ChannelData);

}; // PSD
