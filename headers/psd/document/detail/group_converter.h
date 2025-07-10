
#pragma once

#include "psd/document/detail/layer_converter.h"
#include "psd/document/layer.h"
#include "psd/llapi/structure/info/extra_info/section_divider.h"
#include "psd/llapi/structure/info/layer_info.h"
#include "psd/llapi/structure/info/layer_info/channel_data.h"
#include <cassert>
#include <psd/document/group.h>

namespace PSD::detail {
//

template <typename T>
class GroupConverter;

template <>
class GroupConverter<Group> {
public:
  std::vector<llapi::LayerRecord> operator()(const Group &input) {
    std::vector<llapi::LayerRecord> output;
    output.push_back(CreateStart());

    for (const auto &entry : input) {
      if (entry->IsLayer()) {
        output.push_back(LayerConverter<Layer>()(LayerCast(entry)));
        continue;
      }
      if (entry->IsGroup()) {
        auto input = GroupConverter<Group>()(GroupCast(entry));
        output.insert(output.end(), input.begin(), input.end());
        continue;
      }
      assert(false);
    }
    output.push_back(CreateEnd(input));
    return output;
  }
private:
  void CreateStartLayerData(llapi::LayerData &output) {
    output.channel_count = 4;
    output.blending = llapi::Blending::Normal;
    output.opacity = 0xff;
    output.clipping = false;
    output.flags.has_useful_information = true;
    output.flags.irrelievant_to_appearance = true;
    output.name = "</Layer group>";
    output.extra_info.Insert(llapi::SectionDivider(llapi::DividerType::Hidden));
  }
  void CreateEmptyChannelData(llapi::ChannelData &output) {
    for (auto channel = 0u;
              channel < 4;
              channel++) {
      output.data[(channel == 3) ? -1 : channel].second = {};
    }
  }
  llapi::LayerRecord CreateStart() {
    llapi::LayerRecord output;
    CreateStartLayerData(output.layer_data);
    CreateEmptyChannelData(output.channel_data);
    return output;
  }
  void CreateEndLayerData(const Group &input, llapi::LayerData &output) {
    output.channel_count = 4;
    output.blending = llapi::Blending::PassThrough;
    output.opacity = 0xff;
    output.clipping = false;
    output.flags.has_useful_information = true;
    output.flags.irrelievant_to_appearance = true;
    output.name = input.Name();
    output.extra_info.Insert(llapi::SectionDivider(llapi::DividerType::ClosedFolder));
  }
  llapi::LayerRecord CreateEnd(const Group &input) {
    llapi::LayerRecord output;
    CreateEndLayerData(input, output.layer_data);
    CreateEmptyChannelData(output.channel_data);
    return output;
  }
}; // class GroupConverter

inline bool IsLayer(const llapi::LayerRecord &input) {
  if (!input.layer_data.extra_info.Exists<llapi::SectionDivider>()) {
    return true;
  }
  return input.layer_data.extra_info
    .At<llapi::SectionDivider>().type == llapi::DividerType::Layer;
}
inline bool IsGroupStart(const llapi::LayerRecord &input) {
  if (!input.layer_data.extra_info.Exists<llapi::SectionDivider>()) {
    return false;
  }
  return input.layer_data.extra_info
    .At<llapi::SectionDivider>().type == llapi::DividerType::Hidden;
}
inline bool IsGroupEnd(const llapi::LayerRecord &input) {
  if (!input.layer_data.extra_info.Exists<llapi::SectionDivider>()) {
    return false;
  }
  auto type = input.layer_data.extra_info
    .At<llapi::SectionDivider>().type;
  return type == llapi::DividerType::OpenFolder ||
         type == llapi::DividerType::ClosedFolder;
}
template <typename I>
I FindGroupEnd(I input, I end) {
  for (; input != end; input++) {
    if (IsGroupEnd(*input)) return ++input;
  }
  return end;
}
inline unsigned GroupRecordCount(const Group &input) {
  unsigned output = 2;
  for (const auto &entry : input) {
    if (entry->IsLayer()) {
      output++;
      continue;
    }
    if (entry->IsGroup()) {
      output += GroupRecordCount(GroupCast(entry));
      continue;
    }
    assert(false);
  }
  return output;
}
template <>
class GroupConverter<llapi::LayerRecord> {
public:
  template <typename I>
  Group operator()(I input, I end) {
    if (!IsGroupStart(*input) && !IsGroupEnd(*input)) {
      throw Error("Cannot be converter");
    }
    input++;
    Group output("");
    while (input != end) {
      if (IsLayer(*input)) {
        output.Push(LayerConverter<llapi::LayerRecord>()(*input++));
        continue;
      }
      if (IsGroupStart(*input)) {
        auto group = GroupConverter<llapi::LayerRecord>()(
          input,
          FindGroupEnd(input, end)
        );
        input += GroupRecordCount(group);
        output.Push(std::move(group));
        continue;
      }
      if (IsGroupEnd(*input)) {
        output.SetName(input->layer_data.name);
        input++;
        break;
      }
      assert(false);
    }
    return output;
  }
};
}; // PSD::detail
