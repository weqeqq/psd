
#pragma once

#include "psd/llapi/stream.h"
#include "psd/llapi/structure/info/layer_info/channel_data.h"
#include <psd/llapi/structure/info/layer_info.h>
#include <psd/document/layer.h>

namespace PSD::detail {
//

template <typename T>
class LayerConverter;

template <>
class LayerConverter<Layer> {
public:
  llapi::LayerRecord operator()(const Layer &input) {
    llapi::LayerRecord output;
    CreateLayerData(input, output.layer_data);
    CreateChannelData(input, output.channel_data);
    return output;
  }
private:
  void CreateLayerData(const Layer &input, llapi::LayerData &output) {
    output.coordinates   = input.Coordinates();
    output.channel_count = input.Image().ChannelCount();
    output.blending      = llapi::Blending::Normal;
    output.opacity       = 0xff;
    output.clipping      = false;
    output.name          = input.Name();
  }
  void CreateChannelData(const Layer &input, llapi::ChannelData &output) {
    for (auto channel = 0u;
              channel < input.Image().ChannelCount();
              channel++)
    {
      std::vector<llapi::U8> channel_data(input.Image().Length());
      for (auto index = 0u;
                index < input.Image().Length();
                index++)
      {
        channel_data[index] = input.Image()[index][channel];
      }
      output.data[(channel == 3) ? -1 : channel] = llapi::Channel(std::move(channel_data));
    }
  }
}; // class LayerConverter<Layer>
template <>
class LayerConverter<llapi::LayerRecord> {
public:
  Layer operator()(const llapi::LayerRecord &input) {
    Layer output(input.layer_data.name);
    output.SetImage(ConvertData(input));
    output.SetOffset(
      input.layer_data.coordinates.left,
      input.layer_data.coordinates.top
    );
    return output;
  }
private:
  ::Image::Buffer<> ConvertData(const llapi::LayerRecord &input) {
    auto coordinates = input.layer_data.coordinates;
    ::Image::Buffer<> output(
      coordinates.bottom - coordinates.top,
      coordinates.right  - coordinates.left
    );
    for (auto channel = 0u;
              channel < input.layer_data.channel_count;
              channel++) {
      const auto &current_channel = input.channel_data.data.at((channel == 3) ? -1 : channel).data;
      for (auto index = 0u;
                index < output.Length();
                index++) {
        output[index][channel] = current_channel[index];
      }
    }
    if (input.layer_data.channel_count != 4) {
      for (auto index = 0u;
                index < output.Length();
                index++) {
        output[index][3] = 0xff;
      }
    }
    return output;
  }
}; // class LayerConverter<llapi::LayerRecord>
}; // namespace PSD
