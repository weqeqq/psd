
#pragma once

#include <cassert>
#include <map>
#include <psd/llapi/stream.h>

#include "layer_data/layer_data_extra.h"

#include <string>
#include <vector>

namespace PSD::llapi {
//
class Coordinates {
  struct FromStreamFn {
    void operator()(Stream &stream, Coordinates &output) {
      stream.ReadTo(output.top);
      stream.ReadTo(output.left);
      stream.ReadTo(output.bottom);
      stream.ReadTo(output.right);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Coordinates &input) {
      stream.Write(input.top);
      stream.Write(input.left);
      stream.Write(input.bottom);
      stream.Write(input.right);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  U32 top;
  U32 left;
  U32 bottom;
  U32 right;
}; // class Coordinates
class LayerFlags {
  struct FromStreamFn {
    void operator()(Stream &stream, LayerFlags &output) {
      auto flags = stream.Read<U8>();
      output.transparency_protected    = flags & 1;
      output.visible                   = flags & 2;
      output.obsolete                  = flags & 4;
      output.has_useful_information    = flags & 8;
      output.irrelievant_to_appearance = flags & 16;
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const LayerFlags &input) {
      U8 flags = 0;
      if (input.transparency_protected)    flags |= 1;
      if (input.visible)                   flags |= 2;
      if (input.obsolete)                  flags |= 4;
      if (input.has_useful_information)    flags |= 8;
      if (input.irrelievant_to_appearance) flags |= 16;
      stream.Write(flags);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  bool transparency_protected    = false;
  bool visible                   = false;
  bool obsolete                  = false;
  bool has_useful_information    = false;
  bool irrelievant_to_appearance = false;
}; // class LayerFlags
class AdjustmentInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, AdjustmentInfo &output) {
      auto length = stream.Read<U32>();
      if (length) {
        throw Error("PSD::Error: AdjustmentError");
      }
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const AdjustmentInfo &input) {
      stream.Write(input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  AdjustmentInfo() = default;

  constexpr bool operator==(const AdjustmentInfo &) const { return true; }
  constexpr bool operator!=(const AdjustmentInfo &) const { return false; }
  constexpr unsigned Length() const {
    return 4 + ContentLength();
  }
private:
  constexpr U32 ContentLength() const {
    return 0;
  }
}; // class AdjustmentInfo
class BlendingInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, BlendingInfo &output) {
      stream.ReadTo(output.data_, stream.Read<U32>());
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const BlendingInfo &input) {
      stream.Write(U32(input.data_.size()));
      stream.Write(input.data_);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  BlendingInfo() = default;
  unsigned Length() const {
    return data_.size() + 4;
  }
private:
  std::vector<U8> data_ = {
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff,
  };
}; // class BlendingInfo
class LayerData {
  struct FromStreamFn {
    void ReadCoordinates(Stream &stream, LayerData &output) {
      stream.ReadTo(output.coordinates);
    }
    void ReadChannelCount(Stream &stream, LayerData &output) {
      stream.ReadTo(output.channel_count);
    }
    void ReadChannelInfo(Stream &stream, LayerData &output) {
      for (auto index = 0u; index < output.channel_count; index++) {
        auto channel = stream.Read<I16>();
        auto length  = stream.Read<U32>();
        output.channel_info[channel] = length;
      }
    }
    void ReadBlendingSignature(Stream &stream) {
      if (stream.Read<U32>() != 0x3842494D) {
        throw Error("PSD::Error: BlendingSignatureError");
      }
    }
    void ReadBlending(Stream &stream, LayerData &output) {
      stream.ReadTo(output.blending);
    }
    void ReadOpacity(Stream &stream, LayerData &output) {
      stream.ReadTo(output.opacity);
    }
    void ReadClipping(Stream &stream, LayerData &output) {
      stream.ReadTo(output.clipping);
    }
    void ReadFlags(Stream &stream, LayerData &output) {
      stream.ReadTo(output.flags);
      stream++;
    }
    U32 ReadExtraLength(Stream &stream) {
      return stream.Read<U32>();
    }
    void ReadAdjustmentInfo(Stream &stream, LayerData &output) {
      stream.ReadTo(output.adjustment_info);
    }
    void ReadBlendingInfo(Stream &stream, LayerData &output) {
      stream.ReadTo(output.blending_info);
    }
    void ReadName(Stream &stream, LayerData &output) {
      stream.ReadTo(output.name, stream.Read<U8>());
      stream += output.NamePadding();
    }
    void ReadExtra(Stream &stream, LayerData &output, U32 length) {
      stream.ReadTo(
        output.extra_info,
        length -
        output.adjustment_info.Length() -
        output.blending_info.Length() -
        output.NameLength()
      );
    }
    void operator()(Stream &stream, LayerData &output) {
      ReadCoordinates       (stream, output);
      ReadChannelCount      (stream, output);
      ReadChannelInfo       (stream, output);
      ReadBlendingSignature (stream);
      ReadBlending          (stream, output);
      ReadOpacity           (stream, output);
      ReadClipping          (stream, output);
      ReadFlags             (stream, output);
      auto length = ReadExtraLength(stream);
      ReadAdjustmentInfo (stream, output);
      ReadBlendingInfo   (stream, output);
      ReadName           (stream, output);
      ReadExtra          (stream, output, length);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void WriteCoordinates(Stream &stream, const LayerData &input) {
      stream.Write(input.coordinates);
    }
    void WriteChannelCount(Stream &stream, const LayerData &input) {
      stream.Write(input.channel_count);
    }
    void WriteChannelInfo(Stream &stream, const LayerData &input) {
      stream.Write(input.channel_info);
    }
    void WriteBlendingSignature(Stream &stream) {
      stream.Write(U32(0x3842494D));
    }
    void WriteBlending(Stream &stream, const LayerData &input) {
      stream.Write(input.blending);
    }
    void WriteOpacity(Stream &stream, const LayerData &input) {
      stream.Write(input.opacity);
    }
    void WriteClipping(Stream &stream, const LayerData &input) {
      stream.Write(input.clipping);
    }
    void WriteFlags(Stream &stream, const LayerData &input) {
      stream.Write(input.flags);
      stream.Write(U8(0));
    }
    void WriteExtraLength(Stream &stream, const LayerData &input) {
      stream.Write(U32(input.ExtraLength()));
    }
    void WriteAdjustmentInfo(Stream &stream, const LayerData &input) {
      stream.Write(input.adjustment_info);
    }
    void WriteBlendingInfo(Stream &stream, const LayerData &input) {
      stream.Write(input.blending_info);
    }
    void WriteName(Stream &stream, const LayerData &input) {
      stream.Write(U8(input.name.size()));
      stream.Write(input.name);
      for (auto index = 0u;
                index < input.NamePadding();
                index++) {
        stream.Write(U8(0));
      }
    }
    void WriteExtra(Stream &stream, const LayerData &input) {
      stream.Write(input.extra_info);
    }
    void operator()(Stream &stream, const LayerData &input) {
      WriteCoordinates       (stream, input);
      WriteChannelCount      (stream, input);
      WriteChannelInfo       (stream, input);
      WriteBlendingSignature (stream);
      WriteBlending          (stream, input);
      WriteOpacity           (stream, input);
      WriteClipping          (stream, input);
      WriteFlags             (stream, input);
      WriteExtraLength       (stream, input);
      WriteAdjustmentInfo    (stream, input);
      WriteBlendingInfo      (stream, input);
      WriteName              (stream, input);
      WriteExtra             (stream, input);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  LayerData() = default;

  Coordinates        coordinates;
  U16                channel_count;
  std::map<I16, U32> channel_info;
  Blending           blending;
  U8                 opacity;
  U8                 clipping;
  LayerFlags         flags;
  AdjustmentInfo     adjustment_info;
  BlendingInfo       blending_info;
  std::string        name;
  LayerDataExtra     extra_info;

  unsigned Length() const {
    return ExtraLength() + 34 + (channel_count * 6);
  }
private:
  U32 NameLength() const {
    auto output = name.size(); while ((output++ + 1) % 4);
    return output;
  }
  U32 NamePadding() const {
    return (NameLength() - 1) - name.size();
  }
  U32 ExtraLength() const {
    return adjustment_info.Length() +
           blending_info.Length() +
           NameLength() +
           extra_info.Length();
  }
}; // class LaeyrData
}; // namespace PSD::llapi
