
#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
#include <psd/core/type/depth.h>
#include <psd/structure/header.h>
#include <psd/core/type/color.h>

#include <psd/core/compressor.h>
#include <psd/core/decompressor.h>

#include <image/buffer.h>
#include <image/convertor/depth.h>
#include <image/convertor/color.h>
#include <image/convertor/sequence.h>

namespace PSD {

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedFinalImage {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  class Decompressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr bool IsCompressed   = true;
  static constexpr bool IsDecompressed = false;

  template <Depth::Tp, Color::Tp>
  friend class FinalImage;

  explicit CompressedFinalImage() = default;

  bool operator==(const CompressedFinalImage &other) const {
    return data_ == other.data_;
  }
  bool operator!=(const CompressedFinalImage &other) const {
    return !operator==(other);
  }

private:
  Compression::Tp compression_;
  std::vector<std::uint8_t> data_;

}; // CompressedFinalImage

template <Depth::Tp DepthV, Color::Tp ColorV>
class FinalImage {
public:

  class LengthCalculator;
  class Writer;

  class Compressor;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr bool IsCompressed   = false;
  static constexpr bool IsDecompressed = true;

  explicit FinalImage() = default;

  explicit FinalImage(std::uint64_t row_count, std::uint64_t column_count) : buffer(row_count, column_count) {}

  Image::Buffer<DepthV, ColorV> buffer;

}; // FinalImage 

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedFinalImage<DepthV, ColorV>::LengthCalculator {
public:

  explicit LengthCalculator(const CompressedFinalImage<DepthV, ColorV> &input) : input_(input) {}

  std::uint64_t Calculate() const { 
    return PSD::LengthCalculator(input_.compression_).Calculate() + input_.data_.size(); 
  }

private:
  const CompressedFinalImage<DepthV, ColorV> &input_;

}; // CompressedFinalImage<DepthV, ColorV>::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedFinalImage);

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedFinalImage<DepthV, ColorV>::Reader {
public:

  explicit Reader(Stream &stream) : stream_(stream) {}

  CompressedFinalImage<DepthV, ColorV> Read() {
    CompressedFinalImage<DepthV, ColorV> output;

    output.compression_ = stream_.Read<Compression::Tp>();
    output.data_        = stream_.Read<std::uint8_t>(
      stream_.GetLength() - stream_.GetPos()
    );
    return output;
  }

private:
  Stream &stream_;

}; // CompressedFinalImage<DepthV, ColorV>::Reader

PSD_REGISTER_READER_FOR_BUFFER(CompressedFinalImage);

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedFinalImage<DepthV, ColorV>::Writer {
public:

  explicit Writer(Stream &stream, const CompressedFinalImage<DepthV, ColorV> &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(input_.compression_);
    stream_.Write(input_.data_);
  }

private:
  Stream &stream_;
  const CompressedFinalImage<DepthV, ColorV> &input_;

}; // CompressedFinalImage<DepthV, ColorV>::Writer

PSD_REGISTER_WRITER_FOR_BUFFER(CompressedFinalImage);

template <Depth::Tp DepthV, Color::Tp ColorV>
class CompressedFinalImage<DepthV, ColorV>::Decompressor {
public:

  explicit Decompressor(const CompressedFinalImage<DepthV, ColorV> &input) : input_(input) {}

  FinalImage<DepthV, ColorV> Decompress(const Header &header) const {
    FinalImage<DepthV, ColorV> output(header.row_count, header.column_count);

    std::vector<std::uint8_t> decompressed = PSD::Decompressor(input_.data_).Decompress(
      input_.compression_, 
      header.row_count * header.channel_count, 
      header.column_count
    );

    auto iterator = decompressed.cbegin();

    #define CONVERT(InputDepthV, InputColorV)                                                                                    \
      (header.depth == InputDepthV && header.color == InputColorV) {                                                             \
        std::array<Image::Buffer<InputDepthV, Color::Grayscale>, Color::ChannelCount<InputColorV>> channel_list;                 \
        for (auto index = 0u;                                                                                                    \
                  index < Color::ChannelCount<InputColorV>;                                                                      \
                  index++) {                                                                                                     \
          channel_list[index] = Image::SequenceConvertor(                                                                        \
            iterator  - header.row_count * header.column_count * Image::Element<InputDepthV, Color::Grayscale>::BCount,          \
            iterator += header.row_count * header.column_count * Image::Element<InputDepthV, Color::Grayscale>::BCount           \
          ).template ToBuffer<InputDepthV, Color::Grayscale>(header.row_count, header.column_count);                             \
        }                                                                                                                        \
        output.buffer = Image::DepthConvertor(Image::ColorConvertor(Image::Buffer<InputDepthV, InputColorV>::From(channel_list)) \
          .template Convert<ColorV>())                                                                                           \
          .template Convert<DepthV>();                                                                                           \
      } 

    if CONVERT(Depth::Eight,     Color::RGB)       else 
    if CONVERT(Depth::Sixteen,   Color::RGB)       else 
    if CONVERT(Depth::ThirtyTwo, Color::RGB)       else 
    if CONVERT(Depth::Eight,     Color::CMYK)      else 
    if CONVERT(Depth::Sixteen,   Color::CMYK)      else 
    if CONVERT(Depth::ThirtyTwo, Color::CMYK)      else 
    if CONVERT(Depth::Eight,     Color::Grayscale) else 
    if CONVERT(Depth::Sixteen,   Color::Grayscale) else 
    if CONVERT(Depth::ThirtyTwo, Color::Grayscale) else
    throw std::runtime_error("Failure");

    #undef CONVERT

    return output;
  }

private:
  const CompressedFinalImage<DepthV, ColorV> &input_;

}; // CompressedFinalImage<DepthV, ColorV>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedFinalImage);

template <Depth::Tp DepthV, Color::Tp ColorV> 
class FinalImage<DepthV, ColorV>::Compressor {
public:
  explicit Compressor(const FinalImage<DepthV, ColorV> &input) : input_(input) {}

  CompressedFinalImage<DepthV, ColorV> Compress(Compression::Tp compression) const {
    CompressedFinalImage<DepthV, ColorV> output;

    std::array<Image::Buffer<DepthV, Color::Grayscale>, Color::ChannelCount<ColorV>> channel_list;

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
    for (auto buffer : channel_list) {
      auto data = Image::SequenceConvertor(buffer).ToSequence();
      output.data_.insert(
        output.data_.end(), 
        data.begin(),
        data.end());
    }
    output.data_ = PSD::Compressor(output.data_).Compress(
      compression,
      input_.buffer.GetRCount() * Color::ChannelCount<ColorV>,
      input_.buffer.GetCCount()
    );
    output.compression_ = compression;
    return output;
  }

private:
  const FinalImage<DepthV, ColorV> &input_;

}; // FinalImage<DepthV, ColorV>::Compressor

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(FinalImage);

}; // PSD
