
#pragma once

#include "psd/llapi/structure/info/layer_info.h"

namespace PSD::llapi {
//
class Layer16 : public ExtraFor<Layer16> {
  struct FromStreamFn {
    void operator()(Stream &stream, Layer16 &output, const ExtraHeader &header) {
      auto start = stream.Pos();
      stream.ReadTo(output.data, header.content_length);
      stream += header.content_length - (stream.Pos() - start);
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Layer16 &input) {
      stream.Write(input.data, input.ContentLength());
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Layer16() = default;
  Layer16(LayerInfo data) : data(std::move(data)) {}

  bool operator==(const Layer16 &other) const {
    return true; // TODO
  }
  bool operator!=(const Layer16 &other) const {
    return false; // TODO
  }
  LayerInfo data;
protected:
  U32 ContentLength() const override final {
    return data.Length();
  }
}; // class Layer16
namespace detail {
//
template <>
struct ExtraToID<Layer16> : UseExtraID<ExtraID::Layer16> {};
}; // namespace detail
}; // namespace PSD::llapi
