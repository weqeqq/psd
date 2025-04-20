
#pragma once

#include <psd/document/group.h>
#include <psd/document/impl/processor.h>
#include <psd/document/impl/processor/layer.h>

namespace PSD::DocumentImpl {

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class Processor<Group<DepthV, ColorV>> {
public:

  explicit Processor(const Group<DepthV, ColorV> &input) : input_(input) {}


  using UsedBuffer = Image::AlphaBuffer<DepthV, ColorV>;

  auto Process() {
    auto row_count    = input_.FindRCount();
    auto column_count = input_.FindCCount();

    UsedBuffer buffer(row_count, column_count);

    for (auto element : input_) {
      if (element->IsLayer()) {
        ProcessLayer(buffer, LayerCast<DepthV, ColorV>(element));
        continue;
      }
      if (element->IsGroup()) {
        ProcessGroup(buffer, GroupCast<DepthV, ColorV>(element));
        continue;
      }
      throw GroupError::UndefinedElement();
    }
    return buffer;
  }

private:
  const Group<DepthV, ColorV> &input_;

  void ProcessLayer(UsedBuffer &output, const Layer<DepthV, ColorV> &layer) {
    Image::Blend(
      output,
      PSD::DocumentImpl::Processor(layer).Process(),
      layer.GetLeft() - input_.GetLeft(),
      layer.GetTop()  - input_.GetTop(),
      layer.GetBlending()
    );
  }
  void ProcessGroup(UsedBuffer &output, const Group<DepthV, ColorV> &group) {
    Image::Blend(
      output,
      Processor(group).Process(),
      group.GetLeft() - input_.GetLeft(),
      group.GetTop()  - input_.GetTop(),
      // group.GetBlending()
      Blending::Normal
    );
  }

};

}; 

