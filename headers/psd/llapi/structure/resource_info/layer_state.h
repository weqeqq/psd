
#pragma once

#include <psd/llapi/structure/resource_info/resource.h>

namespace PSD::llapi {
//
class LayerState : public ResourceFor<LayerState> {
  friend ResourceFor<LayerState>;
  struct FromStreamFn {
    void operator()(Stream &stream, LayerState &layer_state, const ResourceHeader &) {
      stream.ReadTo(layer_state.index);
    }
  };
  struct ToStreamFn {
    void operator()(Stream &stream, const LayerState &layer_state) {
      stream.Write(layer_state.index);
    }
  };
  friend Stream;
public:
  LayerState() = default;
  LayerState(U16 index) : index(index) {}
  bool operator==(const LayerState &other) const {
    return index == other.index;
  }
  U16 index = 0;
private:
  ResourceHeader header_ = {ResourceID::LayerState, 2};
}; // class LayerState
// template <>
// struct ResourceFromID_<ResourceID::LayerState> : UseResource<LayerState> {}; // struct ResourceFromID_
// template <>
// struct ResourceToID_<LayerState> : UseResourceID<ResourceID::LayerState> {}; // struct ResourceToID_
}; // namespace PSD::llapi
