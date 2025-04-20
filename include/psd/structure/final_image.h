
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
#include <image/convertor/data.h>
#include <psd/structure/header.h>
#include <psd/core/type/depth.h>
#include <image/channel_array.h>
#include <image/convertor/channel_array.h>
#include <psd/core/type/color.h>

#include <psd/core/compressor.h>
#include <psd/core/decompressor.h>

#include <image/buffer.h>
#include <image/convertor/depth.h>
#include <image/convertor/color.h>
#include <image/convertor/sequence.h>

namespace PSD {

template <Depth::Tp DepthV = DefDepth, 
          Color::Tp ColorV = DefColor,
          bool DataState   = Decompressed>
class FinalImage {
  static_assert(false);
};

template <Depth::Tp DepthV = DefDepth, 
          Color::Tp ColorV = DefColor>
using CompFinalImage = FinalImage<DepthV, ColorV, Compressed>; 

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class FinalImage<DepthV, ColorV, Compressed> {
public:

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr bool IsCompressed   = true;
  static constexpr bool IsDecompressed = false;

  class LengthCalculator {
  public:
    explicit LengthCalculator(const FinalImage &input) : input_(input) {}

    std::uint64_t Calculate() {
      return GetCompressionLength () + 
             GetDataLength        ();
    }
  private:
    const FinalImage &input_;

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

    FinalImage Read() {
      FinalImage output;

      ReadCompression (output);
      ReadData        (output);

      return output;
    }
  private:
    Stream &stream_;

    void ReadCompression(FinalImage &output) {
      output.compression_ = stream_.Read<decltype(output.compression_)>();
    }
    void ReadData(FinalImage &output) {
      output.data_ = stream_.Read<decltype(output.data_)>(
        stream_.GetLength () - 
        stream_.GetPos    ()
      );
    }
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const FinalImage &input) : stream_(stream), input_(input) {}

    void Write() {
      stream_.Write(input_.compression_);
      stream_.Write(input_.data_);
    }
  private:
    Stream &stream_; const FinalImage &input_;
  };
  class Decompressor {
  public:
    using Output = FinalImage<DepthV, ColorV>;

    explicit Decompressor(const FinalImage &input) : input_(input) {}

    auto Decompress(const Header &header) {
      auto data = DecompressData(header);

      #define IF_CONVERT(InputDepthV, InputColorV)       \
        if (Compare<InputDepthV, InputColorV>(header)) { \
          return Process<InputDepthV, InputColorV>(      \
            header,                                      \
            data                                         \
          );                                             \
        }                                                
      IF_CONVERT(Depth::Eight, Color::RGB)       else 
      IF_CONVERT(Depth::Eight, Color::CMYK)      else 
      IF_CONVERT(Depth::Eight, Color::Grayscale) else throw std::runtime_error("final image cvt fail");

      #undef IF_CONVERT
    }
  private:
    const FinalImage &input_;

    auto DecompressData(const Header &header) const {
      return PSD::Decompressor(input_.data_).Decompress(
        input_.compression_,
        header.row_count * header.channel_count,
        header.column_count
      ); 
    }
    template <Depth::Tp InputDepthV, 
              Color::Tp InputColorV>
    bool Compare(const Header &header) const {
      return header.depth == InputDepthV &&
             header.color == InputColorV;
    }

    template <Depth::Tp InputDepthV>
    auto GetChannelFrom(
      const Header &header,
      Image::Sequence::const_iterator iterator,
      Image::Sequence::const_iterator end) const {

      return Image::SequenceConvertor(
        iterator, 
        end
      ).template Convert<InputDepthV, Color::Grayscale, Image::DisableAlpha, Image::Endianness::Big>(
        header.row_count,
        header.column_count
      );
    }
    template <Depth::Tp InputDepthV,
              Color::Tp InputColorV>
    auto ToChannels(const Header &header, const Image::Sequence &data) const {
      Image::ChannelArray<
        InputDepthV,
        InputColorV> channels(header.row_count, header.column_count);

      auto iterator = data.cbegin();
      for (auto index = 0u;
                index < channels.ChannelCount;
                index++) {
        channels[index] = GetChannelFrom<InputDepthV>(
          header,
          iterator  - channels[index].GetByteCount(),
          iterator += channels[index].GetByteCount()
        );
      }
      return channels;
    }
    template <Depth::Tp InputDepthV,
              Color::Tp InputColorV>
    auto ToBuffer(const Header &header, const Image::Sequence &data) const {
      return Image::ChannelArrayConvertor(
        ToChannels<InputDepthV, InputColorV>(
          header, 
          data
        )
      ).Convert();
    }
    auto Create(decltype(Output::buffer) buffer) const {
      Output output;
      output.buffer = std::move(buffer);

      return output;
    }
    template <Depth::Tp InputDepthV, 
              Color::Tp InputColorV>
    auto Process(const Header &header, const Image::Sequence &data) {
      return Create(
        Image::DataConvertor(
          ToBuffer<InputDepthV, InputColorV>(
            header, 
            data
          )
        ).template Convert<DepthV, ColorV>()
      );
    }

  };

  explicit FinalImage() = default;

  friend FinalImage<DepthV, ColorV, Decompressed>;

  bool operator==(const FinalImage &other) const {
    return data_ == other.data_;
  }
  bool operator!=(const FinalImage &other) const {
    return !operator==(other);
  }
private:
  Compression::Tp compression_; std::vector<std::uint8_t> data_;
};

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER (CompFinalImage);
PSD_REGISTER_READER_FOR_BUFFER            (CompFinalImage);
PSD_REGISTER_WRITER_FOR_BUFFER            (CompFinalImage);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER      (CompFinalImage);

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class FinalImage<DepthV, ColorV, Decompressed> {
public:

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr bool IsCompressed   = false;
  static constexpr bool IsDecompressed = true;

  class Compressor {
  public:
    using Output = FinalImage<DepthV, ColorV, Compressed>;

    explicit Compressor(const FinalImage &input) : input_(input) {}

    auto Compress(Compression::Tp compression) {
      return Create(
        compression,
        PSD::Compressor(Convert()).Compress(
          compression, 
          input_.buffer.GetRowCount    () * input_.buffer.ChannelCount, 
          input_.buffer.GetColumnCount ()
        )
      );
    }
  private:
    const FinalImage &input_;

    auto Create(
      decltype(Output::compression_) compression,
      decltype(Output::data_)        data
    ) {
      Output output;

      output.compression_ = std::move(compression);
      output.data_        = std::move(data);

      return output;
    }
    auto Convert() {
      Image::Sequence sequence;

      if (input_.buffer.IsEmpty()) {
        return sequence;
      }
      for (decltype(auto) channel : Image::ChannelArrayConvertor(input_.buffer).Convert()) {
        sequence.insert(sequence.end(), channel.Begin(), channel.End());
      }
      return sequence;
    }
  };
  using UsedBuffer = Image::Buffer<DepthV, ColorV>;

  explicit FinalImage() = default;
  explicit FinalImage(UsedBuffer buffer) : buffer(std::move(buffer)) {}

  UsedBuffer buffer;
};
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(FinalImage);
}
