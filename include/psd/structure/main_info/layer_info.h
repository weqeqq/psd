
#pragma once

#include <psd/structure/main_info/extra_info.h>
#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/main_info/layer_info/channel_data.h>

namespace PSD {
namespace StructureImpl {

template <typename ChannelDataT>
class LayerInfoElement {
public:

  class Compressor;
  class Decompressor;

  static constexpr Depth::Tp DepthValue = ChannelDataT::DepthValue;
  static constexpr Color::Tp ColorValue = ChannelDataT::ColorValue;

  bool operator==(const LayerInfoElement &other) const {
    return layer_data   == other.layer_data   &&
           channel_data == other.channel_data;
  }
  bool operator!=(const LayerInfoElement &other) const {
    return !operator==(other);
  }

  LayerData    layer_data;
  ChannelDataT channel_data;
};
};

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
using LayerInfoElement = StructureImpl::LayerInfoElement<ChannelData<DepthV, ColorV>>;

template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB> 
using CompressedLayerInfoElement = StructureImpl::LayerInfoElement<CompressedChannelData<DepthV, ColorV>>;

template <typename ChannelDataT>
class StructureImpl::LayerInfoElement<ChannelDataT>::Compressor {
public:

  explicit Compressor(StructureImpl::LayerInfoElement<ChannelDataT> input) 
    : input_(std::move(input)) {}

  auto Compress(Compression::Tp compression) {
    CompressedLayerInfoElement<DepthValue, ColorValue> output;

    output.layer_data   = std::move(input_.layer_data);
    output.channel_data = PSD::Compressor(std::move(input_.channel_data)).Compress(compression);

    return output;
  }
private:
  StructureImpl::LayerInfoElement<ChannelDataT> input_;
};
template <typename ChannelDataT>
class StructureImpl::LayerInfoElement<ChannelDataT>::Decompressor {
public:

  explicit Decompressor(StructureImpl::LayerInfoElement<ChannelDataT> input) 
    : input_(std::move(input)) {}

  auto Decompress() {
    PSD::LayerInfoElement<DepthValue, ColorValue> output;

    output.layer_data   = std::move(input_.layer_data);
    output.channel_data = PSD::Decompressor(std::move(input_.channel_data)).Decompress();

    return output;
  }
private:
  StructureImpl::LayerInfoElement<ChannelDataT> input_;
};

namespace StructureImpl {

template <typename ChannelDataT>
class LayerInfo {
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

  explicit LayerInfo() = default;

  bool operator==(const LayerInfo &other) const {
    return element_list_ == other.element_list_;
  }
  bool operator!=(const LayerInfo &other) const {
    return !operator==(other);
  }

  decltype(auto) operator[](std::uint64_t index) {
    return element_list_[index];
  }
  decltype(auto) operator[](std::uint64_t index) const {
    return element_list_[index];
  }

  using Iterator      = typename std::vector<LayerInfoElement<ChannelDataT>>::iterator;
  using ConstIterator = typename std::vector<LayerInfoElement<ChannelDataT>>::const_iterator;

  using RevIterator      = typename std::vector<LayerInfoElement<ChannelDataT>>::reverse_iterator;
  using ConstRevIterator = typename std::vector<LayerInfoElement<ChannelDataT>>::const_reverse_iterator;

  void Push(LayerInfoElement<ChannelDataT> element) {
    element_list_.push_back(std::move(element));
  }
  template <typename InputIterator>
  void Insert(Iterator iterator, InputIterator begin, InputIterator end) {
    element_list_.insert(iterator, begin, end);
  }

  void ChangeLayerCount(std::uint64_t layer_count) {
    element_list_.resize(layer_count);
  }

  std::uint64_t GetLayerCount() const {
    return element_list_.size();
  }
  bool IsEmpty() const {
    return !GetLayerCount();
  }

  auto Begin() {
    return element_list_.begin();
  }
  auto Begin() const {
    return element_list_.begin();
  }
  auto ConstBegin() const {
    return element_list_.begin();
  }
  auto RevBegin() {
    return element_list_.rbegin();
  }
  auto RevBegin() const {
    return element_list_.rbegin();
  }
  auto ConstRevBegin() const {
    return RevBegin();
  }
  auto End() {
    return element_list_.end();
  }
  auto End() const {
    return element_list_.end();
  }
  auto ConstEnd() const {
    return End();
  }
  auto RevEnd() {
    return element_list_.rend();
  }
  auto RevEnd() const {
    return element_list_.rend();
  }
  auto ConstRevEnd() const {
    return RevEnd();
  }

private:
  std::vector<StructureImpl::LayerInfoElement<ChannelDataT>> element_list_;

  std::uint64_t CalculateContentLength() const {
    if (element_list_.empty()) {
      return 0;
    }
    std::uint64_t output = sizeof(std::uint16_t);
    for (auto &element : element_list_) {
      output += PSD::LengthCalculator(element.layer_data)   .Calculate();
      output += PSD::LengthCalculator(element.channel_data) .Calculate();
    }
    return output;
  }
};

template <typename ChannelDataT>
static auto begin(LayerInfo<ChannelDataT> &input) {
  return input.Begin();
}
template <typename ChannelDataT>
static auto begin(const LayerInfo<ChannelDataT> &input) {
  return input.Begin();
};
template <typename ChannelDataT>
static auto end(LayerInfo<ChannelDataT> &input) {
  return input.End();
}
template <typename ChannelDataT>
static auto end(const LayerInfo<ChannelDataT> &input) {
  return input.End();
}
};

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
using LayerInfo = StructureImpl::LayerInfo<ChannelData<DepthV, ColorV>>;

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
using CompressedLayerInfo = StructureImpl::LayerInfo<CompressedChannelData<DepthV, ColorV>>;

template <typename ChannelDataT>
class StructureImpl::LayerInfo<ChannelDataT>::LengthCalculator {
public:
  explicit LengthCalculator(const StructureImpl::LayerInfo<ChannelDataT> &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + sizeof(std::uint32_t);
  }
private:
  const StructureImpl::LayerInfo<ChannelDataT> &input_;
}; 
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(LayerInfo);
PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(CompressedLayerInfo);

template <typename ChannelDataT>
class StructureImpl::LayerInfo<ChannelDataT>::Reader {
public:

  static_assert(IsCompressed);

  explicit Reader(Stream &stream) : stream_(stream) {}

  StructureImpl::LayerInfo<ChannelDataT> Read() {
    if (stream_.Read<std::uint32_t>() == 0) {
      return StructureImpl::LayerInfo<ChannelDataT>();
    }
    StructureImpl::LayerInfo<ChannelDataT> output;

    auto start       = stream_.GetPos();
    auto layer_count = stream_.Read<std::uint16_t>();

    output.ChangeLayerCount(layer_count);

    for (auto &element : output) {
      element.layer_data = stream_.Read<LayerData>();
    }
    for (auto &element : output) {
      element.channel_data = stream_.Read<ChannelDataT>(CrTuple(
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
PSD_REGISTER_READER_FOR_BUFFER(CompressedLayerInfo);

template <typename ChannelDataT>
class StructureImpl::LayerInfo<ChannelDataT>::Writer {
public:

  explicit Writer(Stream &stream, const StructureImpl::LayerInfo<ChannelDataT> &input) 
    : stream_ (stream)
    , input_  (input) {}

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
  const StructureImpl::LayerInfo<ChannelDataT> &input_;
}; 
PSD_REGISTER_WRITER_FOR_BUFFER(LayerInfo);
PSD_REGISTER_WRITER_FOR_BUFFER(CompressedLayerInfo);

template <typename ChannelDataT>
class StructureImpl::LayerInfo<ChannelDataT>::Compressor {
public:

  static_assert(IsDecompressed);

  explicit Compressor(StructureImpl::LayerInfo<ChannelDataT> input) : input_(std::move(input)) {}

  auto Compress(Compression::Tp compression) {
    CompressedLayerInfo<DepthValue, ColorValue> output;

    for (decltype(auto) element : input_) {
      CompressedLayerInfoElement<DepthValue, ColorValue> output_element;

      output_element.layer_data   = std::move(element.layer_data);
      output_element.channel_data = PSD::Compressor(element.channel_data).Compress(
        compression,
        output_element.layer_data
      );
      output_element.layer_data.channel_info = PSD::ChannelInfoCreator(output_element.channel_data).Create();

      output.Push(output_element);
    }
    return output;
  }

private:
  StructureImpl::LayerInfo<ChannelDataT> input_;

};
PSD_REGISTER_COMPRESSOR_FOR_BUFFER(LayerInfo);

template <typename ChannelDataT>
class StructureImpl::LayerInfo<ChannelDataT>::Decompressor {
public:

  static_assert(IsCompressed);

  explicit Decompressor(const StructureImpl::LayerInfo<ChannelDataT> &input) : input_(input) {}

  auto Decompress(const Header &header) const {
    PSD::LayerInfo<DepthValue, ColorValue> output;

    for (auto &element : input_) {
      PSD::LayerInfoElement<DepthValue, ColorValue> output_element;

      output_element.layer_data   = std::move(element.layer_data);
      output_element.channel_data = PSD::Decompressor(element.channel_data).Decompress(
        header,
        output_element.layer_data
      );
      output.Push(output_element);
    }
    return output;
  }

private:
  const StructureImpl::LayerInfo<ChannelDataT> &input_;

}; // StructureImpl::LayerInfo<ChannelDataT>::Decompressor

PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(CompressedLayerInfo);
  
}; // PSD
