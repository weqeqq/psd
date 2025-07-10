
#pragma once

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
    void operator()(Stream &stream, LayerInfo &output) {
      if(!stream.Read<U32>()) return;
      auto start       = stream.Pos();
      auto layer_count = stream.Read<U16>();
      output.record.resize(layer_count);
      for (auto &record : output.record) {
        stream.ReadTo(record.layer_data);
      }
      for (auto &record : output.record) {
        stream.ReadTo(record.channel_data, record.layer_data.channel_info);
      }
      if ((stream.Pos() - start) % 2) stream++;
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const LayerInfo &input) {
      stream.Write(input.ContentLength());
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
      for (const auto &[channel, data] : record.channel_data.data) {
        record.layer_data.channel_info[channel] = 2 + data.second.size();
      }
    }
  }
  void Decompress() {
    for (auto index = 0u;
              index < record.size();
              index++) {
      const auto &coordinates = record[index].layer_data.coordinates;
      record[index].channel_data.Decompress(
        coordinates.bottom - coordinates.top,
        coordinates.right  - coordinates.left
      );
      UpdateChannelInfo();
    }
  }
  void Compress(Compression compression) {
    for (auto index = 0u;
              index < record.size();
              index++) {
      const auto &coordinates = record[index].layer_data.coordinates;
      record[index].channel_data.Compress(
        compression,
        coordinates.bottom - coordinates.top,
        coordinates.right  - coordinates.left
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
}; // namespace PSD::llapi
