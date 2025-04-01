
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

  struct Output {
    Image::Buffer <DepthV, ColorV> buffer;
    Image::Alpha  <DepthV>         alpha;
  };
  auto Process() {
    Output output;

    auto row_count    = input_.FindRCount();
    auto column_count = input_.FindCCount();

    output.buffer = decltype(output.buffer) (row_count, column_count);
    output.alpha  = decltype(output.alpha)  (row_count, column_count);

    for (auto element : input_) {
      if (element->IsLayer()) {
        ProcessLayer(output, LayerCast<DepthV, ColorV>(element));
        continue;
      }
      if (element->IsGroup()) {
        ProcessGroup(output, GroupCast<DepthV, ColorV>(element));
        continue;
      }
      throw GroupError::UndefinedElement();
    }
    return output;
  }

private:
  const Group<DepthV, ColorV> &input_;

  void ProcessLayer(Output &output, const Layer<DepthV, ColorV> &layer) {
    auto [input_buffer, input_alpha] = PSD::DocumentImpl::Processor(layer).Process();
    auto [output_buffer, output_alpha] = Image::Blend(
      output.buffer,
      output.alpha,
      input_buffer,
      input_alpha,
      layer.GetLeft() - input_.GetLeft(),
      layer.GetTop()  - input_.GetTop(),
      layer.GetBlending()
    );
    output.buffer = std::move(output_buffer);
    output.alpha  = std::move(output_alpha);
  }
  void ProcessGroup(Output &output, const Group<DepthV, ColorV> &group) {
    auto [input_buffer, input_alpha] = Processor(group).Process();
    auto [output_buffer, output_alpha] = Image::Blend(
      output.buffer,
      output.alpha,
      input_buffer,
      input_alpha,
      group.GetLeft() - input_.GetLeft(),
      group.GetTop()  - input_.GetTop(),
      // group.GetBlending()
      Blending::Normal
    );
    output.buffer = std::move(output_buffer);
    output.alpha  = std::move(output_alpha);
  }

};

}; 

