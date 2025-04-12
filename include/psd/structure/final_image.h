
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
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
      return Create(Image::SequenceConvertor(
        PSD::Decompressor(input_.data_).Decompress(
          input_.compression_, 
          header.row_count,
          header.column_count
        )
      ).template Convert<
        DepthV, 
        ColorV, 
        Image::DisableAlpha,
        Image::Endianness::Big>
      (
        header.row_count,
        header.column_count,
        header.depth,
        header.color
      ));
    }
  private:
    const FinalImage &input_;

    auto Create(decltype(Output::buffer) buffer) const {
      Output output;
      output.buffer = std::move(buffer);

      return output;
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

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class FinalImage<DepthV, ColorV, Decompressed> {
public:

  class Compressor {
  public:
    using Output = FinalImage<DepthV, ColorV, Compressed>;

    explicit Compressor(const FinalImage &input) : input_(input) {}

    auto Compress(Compression::Tp compression) {
      return Create(
        compression,
        PSD::Compressor(Convert()).Compress(compression)
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

      for (decltype(auto) channel : Image::ChannelArrayConvertor(input_.buffer).Convert()) {
        sequence.insert(sequence.end(), channel.begin(), channel.end());
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
