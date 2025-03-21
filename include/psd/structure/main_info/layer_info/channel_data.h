
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

template <Color::Tp ColorV>
class CompressionInfo {
public:

  static constexpr Compression::Tp DefaultCompression = Compression::RLE;

  explicit CompressionInfo() {
    for (auto &value : data_compression) {
      value = DefaultCompression;
    }
  }
  explicit CompressionInfo(Compression::Tp compression)   
    : tmask_compression       (compression)
    , umask_compression       (compression)
    , rmask_compression       (compression)
    , final_image_compression (compression) {
    for (auto &value : data_compression) {
      value = compression;
    }
  }
  explicit CompressionInfo(
    Compression::Tp data_compression[Color::ChannelCount<ColorV>],
    Compression::Tp tmask_compression,
    Compression::Tp umask_compression,
    Compression::Tp rmask_compression
  ) : data_compression  (data_compression) 
    , tmask_compression (tmask_compression)
    , umask_compression (umask_compression) 
    , rmask_compression (rmask_compression) {}
  Compression::Tp data_compression[Color::ChannelCount<ColorV>];
  
  Compression::Tp tmask_compression = DefaultCompression;
  Compression::Tp umask_compression = DefaultCompression;
  Compression::Tp rmask_compression = DefaultCompression;

  Compression::Tp final_image_compression;

}; // CompressionInfo

template <typename ChannelDataT>
class ChannelInfoCreator {
public:

  static_assert(false);

  explicit ChannelInfoCreator(ChannelDataT) {}

}; // ChannelInfoCreator

template <typename T>
class CompressionInfoCreator {
public:
  
  static_assert(false);

  explicit CompressionInfoCreator(T) {};

}; // CompressionInfoCreator

class CompressedChannel {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Decompressor;

  explicit CompressedChannel() = default;
  explicit CompressedChannel(Compression::Tp compression, std::vector<std::uint8_t> data) 
    : compression_(compression), data_(std::move(data)) {}

  bool operator==(const CompressedChannel &other) const {
    return compression_ == other.compression_ && data_ == other.data_;
  }
  bool operator!=(const CompressedChannel &other) const {
    return !operator==(other);
  }

  Compression::Tp GetCompression() const {
    return compression_;
  }

  operator std::vector<std::uint8_t>() {
    return std::move(data_);
  }

  bool IsEmpty() const {
    return data_.empty();
  }

private:
  Compression::Tp           compression_;
  std::vector<std::uint8_t> data_;
 
}; // CompressedChannel
class CompressedChannel::LengthCalculator {
public:

  explicit LengthCalculator(const CompressedChannel &input) : input_(input) {}

  std::uint64_t Calculate() const {
    if (input_.IsEmpty()) {
      return 0;
    }
    return PSD::LengthCalculator(input_.compression_).Calculate() + input_.data_.size();
  }
private:
  const CompressedChannel &input_;

}; // CompressedChannel::LengthCalculator
PSD_REGISTER_LENGTH_CALCULATOR(CompressedChannel);

class CompressedChannel::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  CompressedChannel Read(std::uint64_t length) {
    CompressedChannel output;

    output.compression_ = stream_.Read<decltype(output.compression_)>();
    output.data_        = stream_.Read<decltype(output.data_)>(
      length - PSD::LengthCalculator(output.compression_).Calculate()
    );
    return output;
  }
private:
  Stream &stream_;

}; // CompressedChannel::Reader
PSD_REGISTER_READER(CompressedChannel);

class CompressedChannel::Writer {
public:

  explicit Writer(Stream &stream, const CompressedChannel &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(input_.compression_);
    stream_.Write(input_.data_);
  }
private:
  Stream &stream_;
  const CompressedChannel &input_;
  
}; // CompressedChannel::Writer
PSD_REGISTER_WRITER(CompressedChannel);

class CompressedChannel::Decompressor {
public:

  explicit Decompressor(const CompressedChannel &input) : input_(input) {}

  std::vector<std::uint8_t> Decompress(
    std::uint64_t row_count, std::uint64_t column_count
  ) const {
    return PSD::Decompressor(input_.data_).Decompress(
      input_.compression_,
      row_count, 
      column_count
    );
  }
private:
  const CompressedChannel &input_;

}; // CompressedChannel::Decompressor
PSD_REGISTER_DECOMPRESSOR(CompressedChannel);

template <Depth::Tp DepthV = Depth::Eight, Color::Tp ColorV = Color::RGB>
class CompressedChannelData {
public:

  template <Depth::Tp, Color::Tp>
  friend class ChannelData;

  template <typename ChannelDataT>
  friend class ChannelInfoCreator;

  static constexpr bool IsCompressed   = true;
  static constexpr bool IsDecompressed = false;

  class LengthCalculator;

  class Reader;
  class Writer;

  class Compressor;
  class Decompressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr std::int16_t TransparencyID = -1;
  static constexpr std::int16_t MaskID         = -2;

  explicit CompressedChannelData() = default;

  bool operator==(const CompressedChannelData &other) const {
    return data_         == other.data_         &&
           transparency_ == other.transparency_ &&
           mask_         == other.mask_;
  }
  bool operator!=(const CompressedChannelData &other) const {
    return !operator==(other);
  }
private:
  CompressedChannel data_[Color::ChannelCount<ColorV>];

  CompressedChannel transparency_;
  CompressedChannel mask_;

}; // CompressedChannelData

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
class ChannelData {
public:
  class Compressor;

  static constexpr bool IsCompressed   = false;
  static constexpr bool IsDecompressed = true;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr std::int16_t TransparencyID = -1;
  static constexpr std::int16_t MaskID         = -2;

  explicit ChannelData() = default;

  Image::Buffer<DepthV, ColorV> data;

  Image::Buffer<DepthV, Color::Grayscale> transparency;
  Image::Buffer<DepthV, Color::Grayscale> mask;

}; // ChannelData

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelInfoCreator<CompressedChannelData<DepthV, ColorV>> {
public:

  explicit ChannelInfoCreator(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelInfo Create() const {
    ChannelInfo output;

    for (auto index = 0u;
              index < Color::ChannelCount<ColorV>;
              index++) {
      output[index] = PSD::LengthCalculator(input_.data_[index]).Calculate();
    }
    if (!input_.transparency_.IsEmpty()) {
      output[input_.TransparencyID] = PSD::LengthCalculator(input_.transparency_).Calculate();
    }
    if (!input_.mask_.IsEmpty()) {
      output[input_.MaskID] = PSD::LengthCalculator(input_.mask_).Calculate();
    }
    return output;
  }

private:
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // ChannelInfoCreator<CompressedChannelData>

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelInfoCreator<ChannelData<DepthV, ColorV>> {
public:

  explicit ChannelInfoCreator(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelInfo Create() const {
    ChannelInfo output;

    for (auto index = 0u;
              index < Color::ChannelCount<ColorV>;
              index++) {
      output[index] = input_.data.GetBCount() / Color::ChannelCount<ColorV>;
    }
    if (input_.transparency.GetLength()) {
      output[input_.TransparencyID] = input_.transparency.GetBCount();
    }
    if (input_.mask.GetLength()) {
      output[input_.MaskID] = input_.mask.GetBCount();
    }
    return output;
  }

private:
  const ChannelData<DepthV, ColorV> &input_;

}; // ChanenlInfoCreator<ChannelData<DepthV, ColorV>>

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::LengthCalculator {
public:
  LengthCalculator(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  std::uint64_t Calculate() const { 
    std::uint64_t output = 0;

    for (const auto &compressed_channel : input_.data_) {
      output += PSD::LengthCalculator(compressed_channel).Calculate();
    }
    output += PSD::LengthCalculator(input_.transparency_) .Calculate();
    output += PSD::LengthCalculator(input_.mask_)         .Calculate();

    return output;
  };
private:
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // CompressedChannelData<DepthV, ColorV>::LengthCalculator
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV,
          Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  CompressedChannelData<DepthV, ColorV> Read(const LayerData &layer_data) {
    CompressedChannelData<DepthV, ColorV> output;

    for (const auto &[channel_id, channel_length] : layer_data.channel_info) {
      auto channel_data = stream_.Read<CompressedChannel>(
        PSD::CrTuple(channel_length)
      );
      switch (channel_id) {
        case TransparencyID: {
          output.transparency_ = std::move(channel_data);
          break;
        }
        case MaskID: {
          output.mask_ = std::move(channel_data);
          break;
        }
        default: {
          output.data_[channel_id] = std::move(channel_data);
          break;
        }
      }
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

    if (!input_.transparency_.IsEmpty()) {
      stream_.Write(input_.transparency_);
    }
    for (auto channel : input_.data_) {
      stream_.Write(channel);
    }
    if (!input_.mask_.IsEmpty()) {
      stream_.Write(input_.mask_);
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

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::Decompressor {
public:

  explicit Decompressor(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelData<DepthV, ColorV> Decompress(const Header &header, const LayerData &layer_data) const {
    ChannelData<DepthV, ColorV> output;

    output.data = DecompressData(header, layer_data);
    if (!input_.transparency_.IsEmpty()) {
      output.transparency = DecompressChannel(input_.transparency_, header, layer_data);
    } 
    if (!input_.mask_.IsEmpty()) {
      output.mask = DecompressChannel (input_.mask_, header, layer_data);
    }
    return output;
  }
private:
  const CompressedChannelData<DepthV, ColorV> &input_;

  Image::Buffer<DepthV, Color::Grayscale> DecompressChannel(const CompressedChannel &input, const Header &header, const LayerData &layer_data) const {
    std::uint32_t row_count    = layer_data.rectangle.bottom - layer_data.rectangle.top;
    std::uint32_t column_count = layer_data.rectangle.right  - layer_data.rectangle.left;

    #define CONVERT(InputDepthV)                                                                             \
      (header.depth == InputDepthV) {                                                                        \
        return Image::DepthConvertor(Image::SequenceConvertor(                                               \
          PSD::Decompressor(input).Decompress(                                                               \
            row_count,                                                                                       \
            column_count                                                                                     \
          )                                                                                                  \
        ).template ToBuffer<InputDepthV, Color::Grayscale, Image::Endianness::Big>(row_count, column_count)) \
         .template Convert<DepthV>();                                                                        \
      }
    if CONVERT(Depth::Eight)     else 
    if CONVERT(Depth::Sixteen)   else 
    if CONVERT(Depth::ThirtyTwo) else 
    throw std::runtime_error("Unsupported depth");
    #undef CONVERT
  }

  Image::Buffer<DepthV, ColorV> DecompressData(const Header &header, const LayerData &layer_data) const {

    #define CONVERT(InputColorV)                                                                                       \
      (header.color == InputColorV) {                                                                                  \
        std::array<Image::Buffer<DepthV, Color::Grayscale>, Color::ChannelCount<InputColorV>> channel_list;            \
        for (auto index = 0u;                                                                                          \
                  index < Color::ChannelCount<InputColorV>;                                                            \
                  index++) {                                                                                           \
          channel_list[index] = DecompressChannel(input_.data_[index], header, layer_data);                                          \
        }                                                                                                              \
      return Image::ColorConvertor(Image::Buffer<DepthV, InputColorV>::From(channel_list)).template Convert<ColorV>(); \
    }
    if CONVERT(Color::Grayscale) else 
    if CONVERT(Color::RGB)       else
    if CONVERT(Color::CMYK)      else
    throw std::runtime_error("err");
    #undef CONVERT
  }

}; // CompressedChannelData<DepthV, ColorV>::Decompressor
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelData<DepthV, ColorV>::Compressor {
public:

  explicit Compressor(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  CompressedChannelData<DepthV, ColorV> Compress(
    CompressionInfo<ColorV> compression_info, const LayerData &layer_data
  ) const {
    CompressedChannelData<DepthV, ColorV> output;

    if (input_.transparency.GetLength()) {
      output.transparency_ = PSD::CompressedChannel(
        compression_info.tmask_compression, 
        PSD::Compressor(Image::SequenceConvertor(input_.transparency).ToSequence()).Compress(
          compression_info.tmask_compression,
          input_.transparency.GetRCount(),
          input_.transparency.GetCCount()
        )
      );
    }
    if (input_.mask.GetLength()) {
      output.mask_ = PSD::CompressedChannel(
        compression_info.umask_compression, 
        PSD::Compressor(Image::SequenceConvertor(input_.mask).ToSequence()).Compress(
          compression_info.umask_compression,
          input_.mask.GetRCount(),
          input_.mask.GetCCount()
        )
      );
    }

    Image::Buffer<DepthV, Color::Grayscale> channel_list[Color::ChannelCount<ColorV>];

    for (auto &buffer : channel_list) {
      buffer = Image::Buffer<DepthV, Color::Grayscale>(input_.data.GetRCount(), input_.data.GetCCount());
    }
    for (auto index = 0u; 
              index < input_.data.GetLength();
              index++) {
      for (auto channel_index = 0u;
                channel_index < Color::ChannelCount<ColorV>;
                channel_index++) {
        channel_list[channel_index][index] = input_.data[index][channel_index];
      }
    }

    for (auto index = 0u; 
              index < Color::ChannelCount<ColorV>;
              index++) {
      auto compression = compression_info.data_compression[index];
      output.data_[index] = PSD::CompressedChannel(
        compression,
        PSD::Compressor(Image::SequenceConvertor(channel_list[index]).ToSequence()).Compress(
          compression,
          input_.data.GetRCount(),
          input_.data.GetCCount()
        )
      );
    }
    return output;
  }

  CompressedChannelData<DepthV, ColorV> Compress(Compression::Tp compression, const LayerData &layer_data) const {
    return Compress(CompressionInfo<ColorV>(compression), layer_data);
  }

private:
  const ChannelData<DepthV, ColorV> &input_;

}; // ChannelData<DepthV, ColorV>
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(ChannelData);

}; // PSD
