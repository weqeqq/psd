
#pragma once

#include "psd/llapi/structure/header.h"
#include <psd/llapi/structure/info/layer_info/layer_data.h>
#include <psd/llapi/structure/info/layer_info/channel_data.h>

namespace PSD::llapi {
//
struct LayerRecord {
  LayerData   layer_data;
  ChannelData channel_data;
}; // class Layer
class LayerInfo {
  struct FromStreamFn {
    void operator()(Stream &stream, LayerInfo &output, unsigned length) {
      if (!length) return;
      auto start       = stream.Pos();
      auto layer_count = stream.Read<U16>();
      output.record.resize(layer_count);
      for (auto &record : output.record) {
        stream.ReadTo(record.layer_data);
      }
      for (auto &record : output.record) {
        stream.ReadTo(record.channel_data, record.layer_data.channel_info);
      }
      auto readed = stream.Pos() - start;
      while (readed++ % 4) {
        stream++;
      }
      // if ((stream.Pos() - start) % 2) stream++;
    }
    void operator()(Stream &stream, LayerInfo &output) {
      operator()(stream, output, stream.Read<U32>());
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const LayerInfo &input, U32 length) {
      stream.Write(length);
      if (input.record.empty()) {
        return;
      }
      stream.Write(U16(input.record.size()));
      for (const auto &record : input.record) {
        stream.Write(record.layer_data);
      }
      for (const auto &record : input.record) {
        stream.Write(record.channel_data);
      }
    }
    void operator()(Stream &stream, const LayerInfo &input) {
      operator()(stream, input, input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  LayerInfo() = default;

  std::vector<LayerRecord> record;

  void Push(LayerRecord input) {
    record.push_back(std::move(input));
  }
  unsigned Length() const {
    return 4 + ContentLength();
  }
  void UpdateChannelInfo() {
    for (auto &record : record) {
      record.layer_data.channel_info.clear();
      for (const auto &[channel, data] : record.channel_data.data) {
        record.layer_data.channel_info[channel] = 2 + data.data.size();
      }
    }
  }
  void Decompress(const Header &header) {
    for (auto index = 0u;
              index < record.size();
              index++) {
      record[index].channel_data.Decompress(header, record[index].layer_data);
      UpdateChannelInfo();
    }
  }
  void Compress(Compression compression, unsigned level, const Header &header) {
    for (auto index = 0u;
              index < record.size();
              index++) {
      const auto &coordinates = record[index].layer_data.coordinates;
      record[index].channel_data.Compress(
        compression,
        level,
        coordinates.bottom - coordinates.top,
        coordinates.right  - coordinates.left,
        header.depth
      );
      UpdateChannelInfo();
    }
  }
private:
  U32 ContentLength() const {
    auto output = record.empty() ? 0u : 2u;
    for (const auto &record : record) {
      output += record.layer_data.Length();
      output += record.channel_data.Length();
    }
    return output;
  }
}; // class LayerInfo
inline void ConvertColorInPlace(LayerInfo &output, Color input_color, Color output_color) {
  for (auto &record : output.record) {
    ConvertColorInPlace(record.channel_data, input_color, output_color);
    switch (output_color) {
      case Color::Grayscale : record.layer_data.channel_count = 2; break;
      case Color::Rgb       : record.layer_data.channel_count = 4; break;
      default:
        throw Error("False");
    }
  }
}
inline LayerInfo ConvertColor(LayerInfo input, Color input_color, Color output_color) {
  LayerInfo output = std::move(input);
  ConvertColorInPlace(output, input_color, output_color);
  return output;
}
inline void ConvertDepthInPlace(LayerInfo &output, Depth input_depth, Depth output_depth) {
  for (auto &record : output.record) {
    ConvertDepthInPlace(record.channel_data, input_depth, output_depth);
  }
}
inline LayerInfo ConvertDepth(LayerInfo input, Depth input_depth, Depth output_depth) {
  LayerInfo output = std::move(input);
  ConvertDepthInPlace(output, input_depth, output_depth);
  return output;
}
}; // namespace PSD::llapi
