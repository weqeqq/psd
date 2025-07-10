
#pragma once

#include "psd/document/detail/group_converter.h"
#include "psd/document/detail/layer_converter.h"
#include "psd/document/group.h"
#include "psd/document/layer.h"
#include "psd/llapi/structure/info/layer_info.h"
#include <cassert>
namespace PSD::detail {
//
using Root = Group;

template <typename T>
class RootConverter;

template <>
class RootConverter<Root> {
public:
  llapi::LayerInfo operator()(const Root &input) {
    llapi::LayerInfo output;
    for (auto entry : input) {
      if (entry->IsLayer()) {
        ConvertLayer(output, LayerCast(entry));
        continue;
      }
      if (entry->IsGroup()) {
        ConvertGroup(output, GroupCast(entry));
        continue;
      }
      assert(false);
    }
    return output;
  }
private:
  void ConvertLayer(llapi::LayerInfo &output, const Layer &input) {
    output.record.push_back(LayerConverter<Layer>()(input));
  }
  void ConvertGroup(llapi::LayerInfo &output, const Group &input) {
    auto data = GroupConverter<Group>()(input);
    output.record.insert(output.record.end(), data.begin(), data.end());
  }
}; // class RootConverter<Root>

template <>
class RootConverter<llapi::LayerInfo> {
public:
  Root operator()(const llapi::LayerInfo &input) {
    Root output;
    for (auto iterator =  input.record.begin();
              iterator != input.record.end();) {
      if (IsLayer(*iterator)) {
        output.Push(LayerConverter<llapi::LayerRecord>()(*iterator++));
        continue;
      }
      if (IsGroupStart(*iterator)) {
        auto group = GroupConverter<llapi::LayerRecord>()(
          iterator,
          FindGroupEnd(iterator, input.record.end())
        );
        iterator += GroupRecordCount(group);
        output.Push(group);
        continue;
      }
      assert(false);
    }
    return output;
  }
}; // class RootConverter<llapi::LayerInfo>
}; // PSD::detail
