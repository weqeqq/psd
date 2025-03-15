
#pragma once

#include <psd/core/stream.h>
#include <psd/core/length_calculator.h>
#include <psd/core/type/rectangle.h>
#include <psd/core/type/error.h>
#include <psd/core/type/blend.h>

#include <psd/structure/main_info/extra_info.h>

#include <iomanip>

namespace PSD {

class AdjustmentLayerData {
public:
  class LengthCalculator;
  class Reader;
  class Writer;

  explicit AdjustmentLayerData() = default;

  bool operator==(const AdjustmentLayerData &other) const {
    return true;
  }
  bool operator!=(const AdjustmentLayerData &other) const {
    return !operator==(other);
  }

private:
  std::uint64_t CalculateContentLength() const {
    return 0;
  }

}; // AdjustmentLayerData

class AdjustmentLayerData::LengthCalculator {
public:
  explicit LengthCalculator(const AdjustmentLayerData &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + 4;
  }

private:
  const AdjustmentLayerData &input_;

}; // AdjustmentLayerData::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(AdjustmentLayerData);

class AdjustmentLayerData::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  AdjustmentLayerData Read() {
    stream_.AdjustPos(stream_.Read<std::uint32_t>());
    return AdjustmentLayerData();
  }

private:
  Stream &stream_;

}; // AdjustmentLayerData::Reader

PSD_REGISTER_READER(AdjustmentLayerData);

class AdjustmentLayerData::Writer {
public:
  explicit Writer(Stream &stream, const AdjustmentLayerData &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
  }

private:
  Stream &stream_;
  const AdjustmentLayerData &input_;

}; // AdjustmentLayerData::Writer

PSD_REGISTER_WRITER(AdjustmentLayerData);
 
class BlendingInfo {
public:
  class LengthCalculator;
  class Reader;
  class Writer;

  using SourceTp      = std::array<std::uint8_t, 4>;
  using DestinationTp = std::array<std::uint8_t, 4>;
  using ContainerTp   = std::vector<std::tuple<SourceTp, DestinationTp>>;

  explicit BlendingInfo() = default;

  bool operator==(const BlendingInfo &other) const {
    return this->data_ == other.data_;
  }
  bool operator!=(const BlendingInfo &other) const {
    return !operator==(other);
  }

private:
  ContainerTp data_ = {
    { { 0x00, 0x00, 0xff, 0xff }, { 0x00, 0x00, 0xff, 0xff } },
    { { 0x00, 0x00, 0xff, 0xff }, { 0x00, 0x00, 0xff, 0xff } },
    { { 0x00, 0x00, 0xff, 0xff }, { 0x00, 0x00, 0xff, 0xff } },
    { { 0x00, 0x00, 0xff, 0xff }, { 0x00, 0x00, 0xff, 0xff } },
    { { 0x00, 0x00, 0xff, 0xff }, { 0x00, 0x00, 0xff, 0xff } },
  };

  std::uint64_t CalculateContentLength() const {
    return data_.size() * 8;
  }

}; // BlendingInfo

class BlendingInfo::LengthCalculator {
public:
  explicit LengthCalculator(const BlendingInfo &input) : input_(input) {}

  std::uint64_t Calculate() const {
    return input_.CalculateContentLength() + 4;
  }

private:
  const BlendingInfo &input_;

}; // BlendingInfo::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(BlendingInfo);

class BlendingInfo::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  BlendingInfo Read() {
    BlendingInfo output;

    output.data_ = stream_.Read<ContainerTp>(stream_.Read<std::uint32_t>() / 8);
    return output;
  }

private:
  Stream &stream_;

}; // BlendingInfo::Reader

PSD_REGISTER_READER(BlendingInfo);

class BlendingInfo::Writer {
public:

  explicit Writer(Stream &stream, const BlendingInfo &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(std::uint32_t(
      input_.CalculateContentLength()
    ));
    stream_.Write(input_.data_);
  }

private:
  Stream &stream_;
  const BlendingInfo &input_;

}; // BlendingInfo::Writer 

PSD_REGISTER_WRITER(BlendingInfo);

class LayerDataIOError : public Error {
public:
  struct InvalidSignature;

protected:
  LayerDataIOError(const std::string &msg) : Error(msg) {}
};

struct LayerDataIOError::InvalidSignature : public LayerDataIOError {
  InvalidSignature() : LayerDataIOError("Invalid signature") {}
};

using ChannelInfo = std::map<std::int16_t, std::uint32_t>;

class LayerData {
public:

  class LengthCalculator;
  class Reader;
  class Writer;

  explicit LayerData() = default;

  bool operator==(const LayerData &other) const {
    return 
    std::tie(
      rectangle,
      channel_count,
      channel_info,
      blend,
      opacity,
      clipping,
      transparency_protected,
      visible,
      obsolete,
      useful_info,
      irrelievant_to_appearance,
      adjustment_data,
      blending_info,
      name,
      extra_info
    ) == 
    std::tie(
      other.rectangle,
      other.channel_count,
      other.channel_info,
      other.blend,
      other.opacity,
      other.clipping,
      other.transparency_protected,
      other.visible,
      other.obsolete,
      other.useful_info,
      other.irrelievant_to_appearance,
      other.adjustment_data,
      other.blending_info,
      other.name,
      other.extra_info
    );
  }
  bool operator!=(const LayerData &other) const {
    return !operator==(other);
  }

  Rectangle     rectangle;
  std::uint16_t channel_count;
  ChannelInfo   channel_info;
  Blend::Tp     blend;
  std::uint8_t  opacity;
  std::uint8_t  clipping;

  bool transparency_protected;
  bool visible;
  bool obsolete;
  bool useful_info;
  bool irrelievant_to_appearance;

  AdjustmentLayerData adjustment_data;
  BlendingInfo        blending_info;
  std::string         name;
  ExtraInfo           extra_info;

private:

  std::uint64_t CalculateNameLength() const {
    std::uint64_t output = name.size();
    while ((output++ + 1) % 4);
    return output;
  }
  std::uint64_t CalculateExtraLength() const {
    return PSD::LengthCalculator(adjustment_data).Calculate() + 
           PSD::LengthCalculator(blending_info).Calculate() + 
           CalculateNameLength() + 
           PSD::LengthCalculator(extra_info).Calculate();
  }

}; // LayerData

class LayerData::LengthCalculator {
public:
  explicit LengthCalculator(const LayerData &input) : input_(input) {}

  std::uint64_t Calculate() {
    return input_.CalculateExtraLength() + 34 + (input_.channel_count * 6);
  }

private:
  const LayerData &input_;

}; // LayerData::LengthCalculator

PSD_REGISTER_LENGTH_CALCULATOR(LayerData);

class LayerData::Reader {
public:
  explicit Reader(Stream &stream) : stream_(stream) {}

  LayerData Read() {
    LayerData output;

    output.rectangle     = stream_.Read<Rectangle>();
    output.channel_count = stream_.Read<std::uint16_t>();
    output.channel_info  = stream_.Read<ChannelInfo>(output.channel_count);

    ReadBlendSignature();

    output.blend    = stream_.Read<Blend::Tp>();
    output.opacity  = stream_.Read<std::uint8_t>();
    output.clipping = stream_.Read<std::uint8_t>();

    std::uint8_t flag_list = stream_.Read<std::uint8_t>();

    output.transparency_protected    = flag_list & 1;
    output.visible                   = flag_list & 2;
    output.obsolete                  = flag_list & 4;
    output.useful_info               = flag_list & 8;
    output.irrelievant_to_appearance = flag_list & 16;

    stream_.IncPos();

    std::uint64_t extra_length = stream_.Read<std::uint32_t>();

    output.adjustment_data = stream_.Read<AdjustmentLayerData>();
    output.blending_info   = stream_.Read<BlendingInfo>();
    output.name            = stream_.Read<char>(stream_.Read<std::uint8_t>());

    stream_.AdjustPos((output.CalculateNameLength() - 1) - output.name.size());

    output.extra_info      = stream_.Read<ExtraInfo>(CrTuple(
      extra_length - 
        PSD::LengthCalculator(output.adjustment_data) .Calculate() - 
        PSD::LengthCalculator(output.blending_info)   .Calculate() - 
        output.CalculateNameLength()
    ));
    return output;
  }

private:
  Stream &stream_;

  void ReadBlendSignature() {
    if (stream_.Read<std::uint8_t, 4>() != std::array<std::uint8_t, 4> { 0x38, 0x42, 0x49, 0x4D }) {
      throw LayerDataIOError::InvalidSignature();
    } 
  }

}; // LayerData::Reader

PSD_REGISTER_READER(LayerData);

class LayerData::Writer {
public:

  explicit Writer(Stream &stream, const LayerData &input) : stream_(stream), input_(input) {}

  void Write() {
    stream_.Write(input_.rectangle);
    stream_.Write(input_.channel_count);
    stream_.Write(input_.channel_info);

    WriteBlendSignature();

    stream_.Write(input_.blend);
    stream_.Write(input_.opacity);
    stream_.Write(input_.clipping);

    std::uint8_t flag_list = 0;

    if (input_.transparency_protected)    flag_list |= 1;
    if (input_.visible)                   flag_list |= 2;
    if (input_.obsolete)                  flag_list |= 4;
    if (input_.useful_info)               flag_list |= 8;
    if (input_.irrelievant_to_appearance) flag_list |= 16;

    stream_.Write(flag_list);
    stream_.Write<std::uint8_t>(0);
    stream_.Write<std::uint32_t>(input_.CalculateExtraLength());
    stream_.Write(input_.adjustment_data);
    stream_.Write(input_.blending_info);
    stream_.Write<std::uint8_t>(input_.name.size());
    stream_.Write<char>(input_.name);
    stream_.Write(std::vector<std::uint8_t>((input_.CalculateNameLength() - 1) - input_.name.size(), 0));
    stream_.Write(input_.extra_info);
  }

private:
  Stream &stream_;
  const LayerData &input_;

  void WriteBlendSignature() {
    stream_.Write<std::uint8_t, 4>({ 0x38, 0x42, 0x49, 0x4D });
  }

}; // LayerData::Writer 

PSD_REGISTER_WRITER(LayerData);

} // PSD
