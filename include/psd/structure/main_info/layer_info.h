
#pragma once

#include <psd/structure/main_info/extra_info.h>
#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/main_info/layer_info/channel_data.h>

namespace PSD {

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor,
          bool DataState = Decompressed>
class LayerInfoElement {
public:

  class LengthCalculator {
  public:
    explicit LengthCalculator(const LayerInfoElement &input) : input_(input) {}

    std::uint64_t Calculate() const {
      return PSD::LengthCalculator(input_.layer_data).Calculate() +
             PSD::LengthCalculator(input_.channel_data).Calculate();
    }
  private:
    const LayerInfoElement &input_;
  };
  class Compressor {
  public:
    explicit Compressor(const LayerInfoElement &input) : input_(input) {}

    using Output = LayerInfoElement<
      DepthV,
      ColorV,
      Compressed>;

    auto Compress(Compression::Tp compression) const {
      Output output;

      CopyLayerData       (output);
      CompressChannelData (output, compression);

      return output;
    }
  private:
    const LayerInfoElement &input_;

    void CopyLayerData(Output &output) const {
      output.layer_data = input_.layer_data;
    }
    void CompressChannelData(Output &output, Compression::Tp compression) const {
      output.channel_data = PSD::Compressor(
        input_.channel_data
      ).Compress(compression);
    }
  };
  class Decompressor {
  public:
    explicit Decompressor(const LayerInfoElement &input) : input_(input) {}

    using Output = LayerInfoElement<
      DepthV,
      ColorV,
      Decompressed>;

    auto Decompress() const {
      Output output;

      CopyLayerData         (output);
      DecompressChannelData (output);
      
      return output;
    }
  private:
    const LayerInfoElement &input_;

    void CopyLayerData(Output &output) const {
      output.layer_data = input_.layer_data;
    }
    void DecompressChannelData(Output &output) const {
      output.channel_data = PSD::Decompressor(
        input_.channel_data
      ).Decompress(
          input_.layer_data.rectangle.bottom - input_.layer_data.rectangle.top,
          input_.layer_data.rectangle.right  - input_.layer_data.rectangle.left
        );
    }
  };
  using UsedChannelData = ChannelData<DepthV, ColorV, DataState>;

  explicit LayerInfoElement() = default;

  LayerData       layer_data;
  UsedChannelData channel_data;
};

template <Depth::Tp DepthV = DefDepth, 
          Color::Tp ColorV = DefColor>
using CompLayerInfoElement = LayerInfoElement<DepthV, ColorV, Compressed>;

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER (LayerInfoElement);
PSD_REGISTER_COMPRESSOR_FOR_BUFFER        (LayerInfoElement);

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER (CompLayerInfoElement);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER      (CompLayerInfoElement);

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor,
          bool DataState   = Decompressed>
class LayerInfo {
public:

  using Element = LayerInfoElement<DepthV, ColorV, DataState>;
  using Container = std::vector<Element>;

  static constexpr Depth::Tp DepthValue = DepthV;
  static constexpr Color::Tp ColorValue = ColorV;

  static constexpr bool IsCompressed   = DataState == Compressed;
  static constexpr bool IsDecompressed = DataState == Decompressed;

  class LengthCalculator {
  public:
    explicit LengthCalculator(const LayerInfo &input) : input_(input) {}

    std::uint64_t Calculate() const {
      return sizeof(std::uint32_t) + input_.CalculateContentLength();
    }
  private:
    const LayerInfo &input_;
  };
  class Reader {
  public:
    explicit Reader(Stream &stream) : stream_(stream) {}

    auto Read() {
      LayerInfo output;

      if (!stream_.Read<std::uint32_t>()) {
        return output;
      }
      auto start = stream_.GetPos();

      output.ChangeLayerCount(
        stream_.Read<std::uint16_t>()
      );
      for (decltype(auto) element : output) {
        element.layer_data = stream_.Read<LayerData>();
      }
      for (decltype(auto) element : output) {
        element.channel_data = stream_.Read<typename Element::UsedChannelData>(CrTuple(
          element.layer_data
        ));
      }
      if ((stream_.GetPos() - start) % 2) {
        stream_.IncPos();
      }
      return output;
    }

  private:
    Stream &stream_;
  };
  class Writer {
  public:
    explicit Writer(Stream &stream, const LayerInfo &input) : stream_(stream), input_(input) {}

    void Write() {
      stream_.Write(std::uint32_t(
        input_.CalculateContentLength()
      ));
      if (input_.IsEmpty()) {
        return;
      }
      stream_.Write<std::uint16_t>(input_.GetLayerCount());
      for (decltype(auto) element : input_) {
        stream_.Write(element.layer_data);
      }
      for (decltype(auto) element : input_) {
        stream_.Write(element.channel_data);
      }
    }
  private:
    Stream &stream_;
    const LayerInfo &input_;
  };
  class Compressor {
  public:
    explicit Compressor(const LayerInfo &input) : input_(input) {}

    using Output = LayerInfo<
      DepthV,
      ColorV, 
      Compressed>;

    auto Compress(Compression::Tp compression) const {
      Output output;

      for (decltype(auto) element : input_) {
        output.Push(PSD::Compressor(element).Compress(compression));

        output.Back().layer_data.channel_info = ChannelInfoCreator(output.Back().channel_data).Create();
      }
      return output;
    }
  private:
    const LayerInfo &input_;
  };
  class Decompressor {
  public:
    explicit Decompressor(const LayerInfo &input) : input_(input) {}

    using Output = LayerInfo<
      DepthV,
      ColorV, 
      Decompressed>;

    auto Decompress() const {
      Output output;

      for (decltype(auto) element : input_) {
        output.Push(PSD::Decompressor(element).Decompress());
      }
      return output;
    }
  private:
    const LayerInfo &input_;
  };

  explicit LayerInfo() = default;

  bool operator==(const LayerInfo &other) const {
    return data_ == other.data_;
  }
  bool operator!=(const LayerInfo &other) const {
    return !operator==(other);
  }

  decltype(auto) operator[](std::uint64_t index) {
    return data_[index];
  }
  decltype(auto) operator[](std::uint64_t index) const {
    return data_[index];
  }

  using Iterator      = typename Container::iterator;
  using ConstIterator = typename Container::const_iterator;

  void Push(Element element) {
    data_.push_back(std::move(element));
  }
  template <typename InputIterator>
  void Insert(Iterator iterator, InputIterator begin, InputIterator end) {
    data_.insert(iterator, begin, end);
  }
  void ChangeLayerCount(std::uint64_t layer_count) {
    data_.resize(layer_count);
  }
  std::uint64_t GetLayerCount() const {
    return data_.size();
  }
  bool IsEmpty() const {
    return GetLayerCount() == 0;
  }

  auto Begin() {
    return data_.begin();
  }
  auto Begin() const {
    return data_.begin();
  }
  auto ConstBegin() const {
    return data_.cbegin();
  }
  auto End() {
    return data_.end();
  }
  auto End() const {
    return data_.end();
  }
  auto ConstEnd() const {
    return data_.cend();
  }

  decltype(auto) Back() {
    return data_.back();
  }
  decltype(auto) Back() const {
    return data_.back();
  }
  decltype(auto) Front() {
    return data_.front();
  }
  decltype(auto) Front() const {
    return data_.front();
  }

private:
  Container data_;

  std::uint64_t CalculateContentLength() const {
    if (data_.empty()) {
      return 0;
    }
    std::uint64_t output = sizeof(std::uint16_t);
    for (auto &element : data_) {
      output += PSD::LengthCalculator(element).Calculate();
    } 
    return output;
  }
};

template <Depth::Tp DepthV = DefDepth,
          Color::Tp ColorV = DefColor>
using CompLayerInfo = LayerInfo<DepthV, ColorV, Compressed>;

PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompLayerInfo);
PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompLayerInfo);
PSD_REGISTER_READER_FOR_BUFFER(CompLayerInfo);
PSD_REGISTER_WRITER_FOR_BUFFER(CompLayerInfo);

PSD_REGISTER_COMPRESSOR_FOR_BUFFER(LayerInfo);

template <Depth::Tp DepthV,
          Color::Tp ColorV,
          bool DataState>
auto begin(LayerInfo<DepthV, ColorV, DataState> &input) {
  return input.Begin();
}
template <Depth::Tp DepthV,
          Color::Tp ColorV,
          bool DataState>
auto begin(const LayerInfo<DepthV, ColorV, DataState> &input) {
  return input.Begin();
}
template <Depth::Tp DepthV,
          Color::Tp ColorV,
          bool DataState>
auto end(LayerInfo<DepthV, ColorV, DataState> &input) {
  return input.End();
}
template <Depth::Tp DepthV,
          Color::Tp ColorV,
          bool DataState>
auto end(const LayerInfo<DepthV, ColorV, DataState> &input) {
  return input.End();
}
};
