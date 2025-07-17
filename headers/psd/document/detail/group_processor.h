
#pragma once

#include "psd/document/detail/layer_processor.h"
#include "psd/document/layer.h"
#include <cassert>
#include <psd/document/group.h>
#include <image/image.h>
#include <image/processing/blend.h>

namespace PSD::detail {
//
class GroupProcessor {
public:
  ::Image::Buffer<> operator()(const Group &input) const {
    ::Image::Buffer<> output(
      input.Bottom() - input.Top(),
      input.Right()  - input.Left()
    );
    for (auto entry : input) {
      if (entry->IsLayer()) {
        ProcessLayer(input, output, LayerCast(entry));
        continue;
      }
      if (entry->IsGroup()) {
        ProcessGroup(input, output, GroupCast(entry));
        continue;
      }
      assert(false);
    }
    return output;
  }
private:
  void ProcessLayer(const Group &group, ::Image::Buffer<> &output, const Layer &input) const {
    Image::Blend(
      output,
      LayerProcessor()(input),
      input.Left() - group.Left(),
      input.Top()  - group.Top(),
      Image::Blending::Normal
    );
  }
  void ProcessGroup(const Group &group, ::Image::Buffer<> &output, const Group &input) const {
    Image::Blend(
      output,
      GroupProcessor()(input),
      input.Left() - group.Left(),
      input.Top()  - group.Top(),
      Image::Blending::Normal
    );
  }
};

inline constexpr auto ProcessGroup = GroupProcessor();

}; // PSD::detail
