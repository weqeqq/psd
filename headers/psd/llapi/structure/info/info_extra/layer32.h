
#pragma once

#include "psd/llapi/structure/info/layer_info.h"
#include "psd/llapi/structure/info/layer_info/layer_data/layer_data_extra/extra.h"

namespace PSD::llapi {
//
class Layer32 : public ExtraFor<Layer32> {
  struct FromStreamFn {
    void operator()(Stream &stream, Layer32 &output, const ExtraHeader &header) {
      auto start = stream.Pos();
      stream.ReadTo(output.data, header.content_length);
      stream += header.content_length - (stream.Pos() - start);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Layer32 &input) {
      stream.Write(input.data, input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Layer32() = default;
  Layer32(LayerInfo data) : data(std::move(data)) {}

  bool operator==(const Layer32 &other) const {
    return true; // TODO
  }
  bool operator!=(const Layer32 &other) const {
    return false; // TODO
  }
  LayerInfo data;
protected:
  U32 ContentLength() const override final {
    return data.Length();
  }
}; // class Layer32
namespace detail {
//
template <>
struct ExtraToID<Layer32> : UseExtraID<ExtraID::Layer32> {};
}; // namespace detail
}; // namespace PSD::llapi
