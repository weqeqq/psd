
#pragma once

#include <psd/document/layer.h>
#include <psd/structure/main_info/layer_info.h>

namespace PSD::DocumentImpl {
  
template <typename T>
class LayerConvertor {
public:

  static_assert(false);

  explicit LayerConvertor(T) {}

};

template <Depth::Tp DepthV, Color::Tp ColorV>
class LayerConvertor<Layer<DepthV, ColorV>> {
public:
  explicit LayerConvertor(const Layer<DepthV, ColorV> &input) : input_(input) {}

  using OutputTp = PSD::LayerInfoElement<DepthV, ColorV>;

  constexpr bool CanBeConverted() const {
    return true;
  }
  OutputTp Convert() const {
    return OutputTp {
      CreateLayerData   (),
      CreateChannelData ()
    };
  }

private:
  const Layer<DepthV, ColorV> &input_;

  LayerData CreateLayerData() const {
    LayerData output;

    AssignBase  (output);
    AssignExtra (output);

    return output;
  }

  ChannelData<DepthV, ColorV> CreateChannelData() const {
    ChannelData<DepthV, ColorV> output;

    output.buffer = input_.GetBuffer();
    output.alpha  = input_.GetAlpha();

    return output;
  }

  void AssignBase(LayerData &output) const {
    output.rectangle     = input_.GetRectangle();
    output.channel_count = input_.GetChannelCount(); 
    output.blending      = input_.GetBlending();
    output.opacity       = 0xff;
    output.clipping      = false;
    output.name          = input_.GetName();
  }
  void AssignExtra(LayerData &output) const {
    output.extra_info.Set(UnicodeName(output.name));
    output.extra_info.Set(SectionDivider(DividerType::Layer));
  }
}; 

template <Depth::Tp DepthV, Color::Tp ColorV>
class LayerConvertor<PSD::LayerInfoElement<DepthV, ColorV>> {
public:

  using LayerTp   = Layer<DepthV, ColorV>;
  using ElementTp = PSD::LayerInfoElement<DepthV, ColorV>;
  using ChannelTp = Image::Alpha<DepthV>;

  explicit LayerConvertor(const ElementTp &input) : input_(input) {}

  bool CanBeConverted() const {
    auto section_divider = input_.layer_data.extra_info.template Get<SectionDivider>();
    return section_divider.type == DividerType::Layer;
  }

  LayerTp Convert() const {
    LayerTp output(GetName());

    output.SetImage(input_.channel_data.buffer);
    output.SetAlpha(GetAlpha());
    output.SetBlending(input_.layer_data.blending);
    output.SetOffset(
      input_.layer_data.rectangle.left, 
      input_.layer_data.rectangle.top
    );
    return output;
  }

private:
  const ElementTp &input_;

  std::string GetName() const {
    return input_.layer_data.name; // TODO
  }
  ChannelTp GetAlpha() const {
    return input_.channel_data.alpha.GetLength()
      ? input_.channel_data.alpha
      : ChannelTp(
      input_.channel_data.buffer.GetRCount(),
      input_.channel_data.buffer.GetCCount()
    );
  }

};

};

