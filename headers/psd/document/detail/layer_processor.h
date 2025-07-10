
#pragma once

#include <psd/document/layer.h>

namespace PSD::detail {
//
class LayerProcessor {
public:
  ::Image::Buffer<> operator()(const Layer &input) {
    return input.Image();
  }
};
}; // namespace PSD::detail
