
#pragma once

#include <psd/structure/main_info/extra_info.h>
#include <psd/structure/main_info/global_info.h>
#include <psd/structure/main_info/layer_info.h>

namespace PSD {

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor,
          bool DataState = Decompressed>
class MainInfo {
public:
  class LengthCalculator {
  public:
    explicit LengthCalculator(const MainInfo &input) : input_(input) {}

    std::uint64_t Calculate() const {
      return input_.CalculateContentLength() + 4;
    }
  private:
    const MainInfo &input_;
  };
  class Reader {
  public:
    explicit Reader(Stream &stream) : stream_(stream) {}

    auto Read() {
      MainInfo output;

      std::uint64_t length = stream_.Read<std::uint32_t>();
      std::uint64_t start  = stream_.GetPos();

      output.layer_info  = stream_.Read<UsedLayerInfo>();
      output.global_info = stream_.Read<GlobalInfo>();
      output.extra_info  = stream_.Read<ExtraInfo>(CrTuple(
        length - (stream_.GetPos() - start)
      ));
      return output;
    }
  private:
    Stream &stream_;
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const MainInfo &input) : stream_(stream), input_(input) {}

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
    const MainInfo &input_;
  };
  class Compressor {
  public:
    explicit Compressor(const MainInfo &input) : input_(input) {}

    using Output = MainInfo<DepthV, ColorV, Compressed>;

    Output Compress(Compression::Tp compression) {
      Output output;

      output.layer_info  = PSD::Compressor(input_.layer_info).Compress(compression);
      output.global_info = input_.global_info;
      output.extra_info  = input_.extra_info;

      return output;
    }
  private:
    const MainInfo &input_;
  };
  class Decompressor {
  public:
    explicit Decompressor(const MainInfo &input) : input_(input) {}

    using Output = MainInfo<DepthV, ColorV, Decompressed>;

    Output Decompress() {
      Output output;

      output.layer_info  = PSD::Decompressor(input_.layer_info).Decompress();
      output.global_info = input_.global_info;
      output.extra_info  = input_.extra_info;

      return output;
    }
  private:
    const MainInfo &input_;
  };

  explicit MainInfo() = default;

  bool operator==(const MainInfo &other) const {
    return layer_info  == other.layer_info  &&
           global_info == other.global_info &&
           extra_info  == other.extra_info;
  }
  bool operator!=(const MainInfo &other) const {
    return !operator==(other);
  }

  using UsedLayerInfo = LayerInfo<DepthV, ColorV, DataState>;
  
  UsedLayerInfo layer_info;
  GlobalInfo    global_info;
  ExtraInfo     extra_info; 

private:
  std::uint64_t CalculateContentLength() const {
    return PSD::LengthCalculator(layer_info)  .Calculate() +
           PSD::LengthCalculator(global_info) .Calculate() +
           PSD::LengthCalculator(extra_info)  .Calculate();
  }
};

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor>
using CompMainInfo = MainInfo<DepthV, ColorV, Compressed>;

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompMainInfo);
PSD_REGISTER_READER_FOR_BUFFER(CompMainInfo);
PSD_REGISTER_WRITER_FOR_BUFFER(CompMainInfo);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompMainInfo);

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(MainInfo);

};
