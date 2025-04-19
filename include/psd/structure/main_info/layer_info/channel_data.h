
#pragma once

#include <psd/core/compressor.h>
#include <image/convertor/channel_array.h>
#include <psd/core/decompressor.h>

#include <image/convertor/sequence.h>

#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/header.h>

#include <psd/core/type/error.h>
#include <psd/core/type/depth.h>
#include <psd/core/type/color.h>

#include <image/buffer.h>

namespace PSD {

template <Color::Tp ColorV = Color::RGB>
class CompressionInfo {
public:
  explicit CompressionInfo() {
    for (auto &value : channel) {
      value = DefCompression;
    }
  }
  explicit CompressionInfo(Compression::Tp compression)
    : user_mask      (compression)
    , real_user_mask (compression)
    , final_image    (compression) {
    for (auto &value : channel) {
      value = compression;
    }
  }
  Compression::Tp channel[Color::ChannelCount<ColorV, Image::EnableAlpha>];
  Compression::Tp 
    user_mask      = DefCompression,
    real_user_mask = DefCompression,
    final_image    = DefCompression;
};

template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB,
          bool DataState   = Decompressed>
class ChannelData {
  static_assert(false);
};

namespace ChannelDataImpl {

template <bool DataState = Decompressed>
class Channel {
  static_assert(false);
};
template <>
class Channel<Compressed> {
public:
  using Container = std::vector<std::uint8_t>;

  class LengthCalculator {
  public:
    explicit LengthCalculator(const Channel &input) : input_(input) {}

    auto Calculate() const {
      return GetCompressionLength () +
             GetDataLength        ();
    }
  private:
    const Channel &input_;

    std::uint64_t GetCompressionLength() const {
      return PSD::LengthCalculator(input_.compression_).Calculate();
    }
    std::uint64_t GetDataLength() const {
      return PSD::LengthCalculator(input_.data_).Calculate();
    }
  };
  class Reader {
  public:
    explicit Reader(Stream &stream) : stream_(stream) {}

    auto Read(std::uint64_t length) {
      Channel output;

      ReadCompression (output);
      ReadData        (output, length);

      return output;
    }
  private:
    Stream &stream_;

    void ReadCompression(Channel &output) {
      output.compression_ = stream_.Read<decltype(output.compression_)>();
    }
    void ReadData(Channel &output, std::uint64_t length) {
      output.data_ = stream_.Read<decltype(output.data_)>(
        length - PSD::LengthCalculator(output.compression_).Calculate()
      );
    } 
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const Channel &input) : stream_(stream), input_(input) {}

    void Write() {
      stream_.Write(input_.compression_);
      stream_.Write(input_.data_);
    }
  private:
    Stream &stream_;
    const Channel &input_;
  };
  class Decompressor;

  friend class Channel<Decompressed>;

  bool operator==(const Channel &other) const {
    return compression_ == other.compression_ && 
           data_        == other.data_;
  }
  bool operator!=(const Channel &other) const {
    return !operator==(other);
  }

private:
  Compression::Tp compression_;
  Container       data_;
};

using CompChannel = Channel<Compressed>;

template <>
class Channel<Decompressed> {
private:
  using Container = std::vector<std::uint8_t>;

public:
  class Compressor {
  public:
    using UsedCompChannel = Channel<Compressed>;

    explicit Compressor(const Channel &channel) : channel_(channel) {}

    auto Compress(
      Compression::Tp compression,
      std::uint64_t row_count,
      std::uint64_t column_count
    ) const {
      return Create(
        compression,
        PSD::Compressor(channel_.data).Compress(
          compression,
          row_count, 
          column_count
        )
      );
    }
  private:
    const Channel &channel_;

    UsedCompChannel Create(
      decltype(UsedCompChannel::compression_) compression,
      decltype(UsedCompChannel::data_)        data
    ) const {
      UsedCompChannel comp_channel;

      comp_channel.compression_ = std::move(compression);
      comp_channel.data_        = std::move(data);

      return comp_channel;
    }
  };
  explicit Channel() = default;
  explicit Channel(Container data) : data(std::move(data)) {}

  Container data;
};

class CompChannel::Decompressor {
public:
  using UsedChannel = Channel<Decompressed>;

  explicit Decompressor(const Channel &comp_channel) 
    : comp_channel_(comp_channel) {}

  auto Decompress(
    std::uint64_t row_count,
    std::uint64_t column_count
  ) const {
    return UsedChannel(
      PSD::Decompressor(comp_channel_.data_).Decompress(
        comp_channel_.compression_,
        row_count,
        column_count
      )
    );
  }
private:
  const Channel &comp_channel_;
};
};

PSD_REGISTER_LENGTH_CALCULATOR (ChannelDataImpl::CompChannel);
PSD_REGISTER_READER            (ChannelDataImpl::CompChannel);
PSD_REGISTER_WRITER            (ChannelDataImpl::CompChannel);
PSD_REGISTER_DECOMPRESSOR      (ChannelDataImpl::CompChannel);
PSD_REGISTER_COMPRESSOR        (ChannelDataImpl::Channel<>);

template <Depth::Tp DepthV,
          Color::Tp ColorV>
class ChannelData<DepthV, ColorV, Compressed> {
public:

  static constexpr Depth::Tp DataValue = DepthV;
  static constexpr Color::Tp DataColor = ColorV;

  static constexpr bool IsCompressed = Compressed;

  using Channel = ChannelDataImpl::Channel<Compressed>;

  class LengthCalculator {
  public:
    explicit LengthCalculator(const ChannelData &input) : input_(input) {}

    auto Calculate() const {
      std::uint64_t output = 0;

      for (decltype(auto) channel : input_.data_) {
        output += PSD::LengthCalculator(channel).Calculate();
      } 
      return output;
    }
  private:
    const ChannelData &input_;
  };
  class Reader {
  public:
    explicit Reader(Stream &stream) : stream_(stream) {}

    auto Read(LayerData &layer_data) {
      ChannelData output;

      output.data_[Color::AlphaIndex<ColorV>] = stream_.Read<Channel>(CrTuple(
        layer_data.channel_info[-1]
      ));
      for (auto index = 0u;
                index < Color::ChannelCount<ColorV, Image::DisableAlpha>;
                index++) {
        output.data_[index] = stream_.Read<Channel>(CrTuple(
          layer_data.channel_info[index]
        ));
      }
      return output;
    }
  private:
    Stream &stream_;
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const ChannelData &input) : stream_(stream), input_(input) {}

    void Write() {
      stream_.Write(input_[Color::AlphaIndex<ColorV>]);
      for (auto index = 0u;
                index < Color::ChannelCount<ColorV, Image::DisableAlpha>;
                index++) {
        stream_.Write(input_.data_[index]);
      }
    }
  private:
    Stream &stream_;
    const ChannelData &input_;
  };
  class Decompressor {
  public:
    using Output = ChannelData<
      DepthV, 
      ColorV, 
      Decompressed>;

    using ChannelArray = Image::ChannelArray<
      DepthV,
      ColorV,
      Image::EnableAlpha>;

    explicit Decompressor(const ChannelData &input) : input_(input) {}

    auto Decompress(
      std::uint64_t row_count, 
      std::uint64_t column_count
    ) const {
      ChannelArray channel_array(row_count, column_count);

      for (auto index = 0u;
                index < Color::ChannelCount<ColorV, Image::EnableAlpha>;
                index++) {
        channel_array[index] = Image::SequenceConvertor(
          PSD::Decompressor(
            input_.data_[index]
          ).Decompress(row_count, column_count).data
        ).template Convert< 
          DepthV,
          Color::Grayscale,
          Image::DisableAlpha,
          Image::Endianness::Big>(row_count, column_count);
      }
      return Image::ChannelArrayConvertor(channel_array).Convert();
    }
  private:
    const ChannelData &input_;
  };
  friend ChannelData<DepthV, ColorV, Decompressed>;

  explicit ChannelData() = default;

private:
  Channel data_[Color::ChannelCount<ColorV, Image::EnableAlpha>];
};

template <Depth::Tp DepthV,
          Color::Tp ColorV>
using CompChannelData = ChannelData<DepthV, ColorV, Compressed>;

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompChannelData);
PSD_REGISTER_READER_FOR_BUFFER(CompChannelData);
PSD_REGISTER_WRITER_FOR_BUFFER(CompChannelData);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompChannelData);

template <Depth::Tp DepthV,
          Color::Tp ColorV>
class ChannelData<DepthV, ColorV, Decompressed> {
public:
  static constexpr Depth::Tp DataValue = DepthV;
  static constexpr Color::Tp DataColor = ColorV;

  static constexpr bool IsCompressed = Decompressed;

  class Compressor {
  public:
    using UsedCompChannelData = ChannelData<DepthV, ColorV, Compressed>;
    using Channel     = ChannelDataImpl::Channel<>;
    using CompChannel = ChannelDataImpl::CompChannel;
    using ChannelArray = Image::ChannelArray<
      DepthV,
      ColorV,
      Image::EnableAlpha>;

    explicit Compressor(const ChannelData &input) : channel_data_(input) {}

    auto Compress(Compression::Tp compression) {
      UsedCompChannelData output;

      auto channels = ToChannels();
      for (auto index = 0u; 
                index < Color::ChannelCount<ColorV, Image::EnableAlpha>; 
                index++) {
        output.data_[index] = CompressChannel(
          channels[index], 
          compression
        );
      }
      return output;
    }
  private:
    const ChannelData &channel_data_;

    ChannelArray ToChannels() {
      return Image::ChannelArrayConvertor(channel_data_.buffer).Convert(); 
    }
    auto ConvertBuffer(
      const typename ChannelArray::UsedBuffer &buffer
    ) const {
      return Image::SequenceConvertor(
        buffer
      ).template Convert<Image::Endianness::Big>();
    }
    CompChannel CompressChannel(
      const typename ChannelArray::UsedBuffer &buffer, Compression::Tp compression
    ) const {
      return PSD::Compressor(Channel(ConvertBuffer(buffer))).Compress(
        compression,
        channel_data_.buffer.GetRowCount    (),
        channel_data_.buffer.GetColumnCount ()
      );
    }
  };
  using UsedBuffer = Image::AlphaBuffer<DepthV, ColorV>;

  explicit ChannelData() = default;
  explicit ChannelData(UsedBuffer buffer) : buffer(std::move(buffer)) {}

  UsedBuffer buffer;
};
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(ChannelData);

template <Depth::Tp DepthV,
          Color::Tp ColorV>
class ChannelInfoCreator {
public:
  using UsedCompChannelData = CompChannelData<DepthV, ColorV>;

  explicit ChannelInfoCreator(const UsedCompChannelData &input) : input_(input) {}
  
  auto Create() {
    ChannelInfo output;

    output[-1] = PSD::LengthCalculator(input_[Color::AlphaIndex<ColorV>]).Calculate();
    for (auto index = 0u;
              index < Color::ChannelCount<ColorV, Image::DisableAlpha>;
              index++) {
      output[index] = PSD::LengthCalculator(input_[index]).Calculate();
    } 
    return output;
  }
private:
  const UsedCompChannelData &input_;
};
};
