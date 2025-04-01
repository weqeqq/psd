
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
    : transparency   (compression)
    , user_mask      (compression)
    , real_user_mask (compression)
    , final_image    (compression) {
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
    , transparency (tmask_compression)
    , user_mask (umask_compression) 
    , real_user_mask (rmask_compression) {}
  Compression::Tp data_compression[Color::ChannelCount<ColorV>];
  
  Compression::Tp transparency = DefaultCompression;
  Compression::Tp user_mask = DefaultCompression;
  Compression::Tp real_user_mask = DefaultCompression;

  Compression::Tp final_image;

}; // CompressionInfo

template <typename ChannelDataT>
class ChannelInfoCreator {
public:

  static_assert(false);

  explicit ChannelInfoCreator(ChannelDataT) {}

}; // ChannelInfoCreator

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
      return PSD::LengthCalculator(input_.compression_).Calculate();
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

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
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
  class Decompressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr std::int16_t TMaskID = -1;
  static constexpr std::int16_t UMaskID = -2;
  static constexpr std::int16_t RMaskID = -3;

  explicit CompressedChannelData() = default;

  bool operator==(const CompressedChannelData &other) const {
    return channel_list_  == other.channel_list_ &&
           tmask_         == other.tmask_        &&
           umask_         == other.umask_        &&
           rmask_         == other.rmask_;
  }
  bool operator!=(const CompressedChannelData &other) const {
    return !operator==(other);
  }
private:
  CompressedChannel channel_list_[Color::ChannelCount<ColorV>];

  CompressedChannel tmask_;
  CompressedChannel umask_;
  CompressedChannel rmask_;

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

  static constexpr std::int16_t TMaskID = -1;
  static constexpr std::int16_t UMaskID = -2;
  static constexpr std::int16_t RMaskID = -2;

  using BufferTp  = Image::Buffer<DepthV, ColorV>;
  using ChannelTp = Image::Buffer<DepthV, Color::Grayscale>;

  explicit ChannelData() = default;

  explicit ChannelData(
    BufferTp buffer,
    ChannelTp tmask, 
    ChannelTp umask,
    ChannelTp rmask
  ) : buffer (std::move(buffer))
    , alpha  (std::move(tmask)) 
    , user_mask  (std::move(umask)) 
    , real_user_mask  (std::move(rmask)) {}

  explicit ChannelData(
    BufferTp buffer,
    ChannelTp tmask,
    ChannelTp umask
  ) : buffer (std::move(buffer)) 
    , alpha  (std::move(tmask)) 
    , user_mask  (std::move(umask)) {}

  explicit ChannelData(
    BufferTp buffer,
    ChannelTp tmask
  ) : buffer (std::move(buffer)) 
    , alpha  (std::move(tmask)) {}

  explicit ChannelData(
    BufferTp buffer
  ) : buffer (std::move(buffer)) {}

  Image::Buffer<DepthV, ColorV> buffer;

  Image::Buffer<DepthV, Color::Grayscale> alpha;
  Image::Buffer<DepthV, Color::Grayscale> user_mask;
  Image::Buffer<DepthV, Color::Grayscale> real_user_mask;

}; // ChannelData

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedChannelData<DepthV, ColorV>::LengthCalculator {
public:
  LengthCalculator(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  std::uint64_t Calculate() const { 
    std::uint64_t output = 0;

    for (const auto &compressed_channel : input_.channel_list_) {
      output += PSD::LengthCalculator(compressed_channel).Calculate();
    }

    /*if (!input_.transparency_.IsEmpty()) {*/
      output += PSD::LengthCalculator(input_.tmask_).Calculate();
    /*}*/
    if (!input_.umask_.IsEmpty()) {
      output += PSD::LengthCalculator(input_.umask_).Calculate();
    }
    /*output += PSD::LengthCalculator(input_.transparency_) .Calculate();*/
    /*output += PSD::LengthCalculator(input_.mask_)         .Calculate();*/

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
        case TMaskID: {
          output.tmask_ = std::move(channel_data);
          break;
        }
        case UMaskID: {
          output.umask_ = std::move(channel_data);
          break;
        }
        default: {
          output.channel_list_[channel_id] = std::move(channel_data);
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

template <Depth::Tp DepthV, 
          Color::Tp ColorV> 
class CompressedChannelData<DepthV, ColorV>::Writer {
public:

  explicit Writer(Stream &stream, const CompressedChannelData<DepthV, ColorV> &input) 
    : stream_ (stream)
    , input_  (input) {}

  void Write() {
    stream_.Write(input_.tmask_);

    for (auto channel : input_.channel_list_) {
      stream_.Write(channel);
    }
    if (!input_.umask_.IsEmpty()) {
      stream_.Write(input_.umask_);
    }
    if (!input_.rmask_.IsEmpty()) {
      stream_.Write(input_.rmask_);
    }
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

    output.buffer = DecompressBuffer(header, layer_data);
    if (!input_.tmask_.IsEmpty()) {
      output.alpha = DecompressChannel(input_.tmask_, header, layer_data);
    } 
    if (!input_.umask_.IsEmpty()) {
      output.user_mask = DecompressChannel(input_.umask_, header, layer_data);
    }
    if (!input_.rmask_.IsEmpty()) {
      output.real_user_mask = DecompressChannel(input_.rmask_, header, layer_data);
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

  Image::Buffer<DepthV, ColorV> DecompressBuffer(const Header &header, const LayerData &layer_data) const {

    #define CONVERT(InputColorV)                                                                                       \
      (header.color == InputColorV) {                                                                                  \
        std::array<Image::Buffer<DepthV, Color::Grayscale>, Color::ChannelCount<InputColorV>> channel_list;            \
        for (auto index = 0u;                                                                                          \
                  index < Color::ChannelCount<InputColorV>;                                                            \
                  index++) {                                                                                           \
          channel_list[index] = DecompressChannel(input_.channel_list_[index], header, layer_data);                    \
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
class ChannelInfoCreator<CompressedChannelData<DepthV, ColorV>> {
public:

  explicit ChannelInfoCreator(const CompressedChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelInfo Create() const {
    ChannelInfo output;

    for (auto index = 0u;
              index < Color::ChannelCount<ColorV>;
              index++) {
      output[index] = PSD::LengthCalculator(input_.channel_list_[index]).Calculate();
    }
    output[input_.TMaskID] = PSD::LengthCalculator(input_.tmask_).Calculate();

    if (!input_.umask_.IsEmpty()) {
      output[input_.UMaskID] = PSD::LengthCalculator(input_.umask_).Calculate();
    }
    if (!input_.rmask_.IsEmpty()) {
      output[input_.RMaskID] = PSD::LengthCalculator(input_.rmask_).Calculate();
    }
    return output;
  }

private:
  const CompressedChannelData<DepthV, ColorV> &input_;

}; // ChannelInfoCreator<CompressedChannelData>

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelData<DepthV, ColorV>::Compressor {
public:

  explicit Compressor(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  CompressedChannelData<DepthV, ColorV> Compress(
    CompressionInfo<ColorV> compression_info, const LayerData &layer_data
  ) const {
    CompressedChannelData<DepthV, ColorV> output;

    if (input_.alpha.GetLength()) {
      output.tmask_ = CompressChannel(
        compression_info.transparency, 
        input_.alpha
      );
    }
    if (input_.user_mask.GetLength()) {
      output.umask_ = CompressChannel(
        compression_info.user_mask,
        input_.user_mask
      );
    }
    if (input_.real_user_mask.GetLength()) {
      output.rmask_ = CompressChannel(
        compression_info.real_user_mask,
        input_.real_user_mask
      );
    }

    Image::Buffer<DepthV, Color::Grayscale> channel_list[Color::ChannelCount<ColorV>];

    for (auto &buffer : channel_list) {
      buffer = Image::Buffer<DepthV, Color::Grayscale>(input_.buffer.GetRCount(), input_.buffer.GetCCount());
    }
    for (auto index = 0u; 
              index < input_.buffer.GetLength();
              index++) {
      for (auto channel_index = 0u;
                channel_index < Color::ChannelCount<ColorV>;
                channel_index++) {
        channel_list[channel_index][index] = input_.buffer[index][channel_index];
      }
    }

    for (auto index = 0u; 
              index < Color::ChannelCount<ColorV>;
              index++) {
      auto compression = compression_info.data_compression[index];
      output.channel_list_[index] = PSD::CompressedChannel(
        compression,
        PSD::Compressor(Image::SequenceConvertor(channel_list[index]).ToSequence()).Compress(
          compression,
          input_.buffer.GetRCount(),
          input_.buffer.GetCCount()
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

  CompressedChannel CompressChannel(Compression::Tp compression, const Image::Buffer<DepthV, Color::Grayscale> &input) const {
    return CompressedChannel(
      compression,
      PSD::Compressor(Image::SequenceConvertor(input).ToSequence()).Compress(
        compression,
        input.GetRCount(),
        input.GetCCount()
      )
    );
  }

}; // ChannelData<DepthV, ColorV>
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(ChannelData);

template <Depth::Tp DepthV, Color::Tp ColorV>
class ChannelInfoCreator<ChannelData<DepthV, ColorV>> {
public:

  explicit ChannelInfoCreator(const ChannelData<DepthV, ColorV> &input) : input_(input) {}

  ChannelInfo Create() const {
    ChannelInfo output;

    for (auto index = 0u;
              index < Color::ChannelCount<ColorV>;
              index++) {
      output[index] = input_.buffer.GetBCount() / Color::ChannelCount<ColorV>;
    }
    output[input_.TMaskID] = input_.alpha.GetBCount();

    if (input_.user_mask.GetLength()) {
      output[input_.UMaskID] = input_.user_mask.GetBCount();
    }
    if (input_.real_user_mask.GetLength()) {
      output[input_.RMaskID] = input_.real_user_mask.GetBCount();
    }
    return output;
  }

private:
  const ChannelData<DepthV, ColorV> &input_;

}; // ChanenlInfoCreator<ChannelData<DepthV, ColorV>>


}; // PSD
