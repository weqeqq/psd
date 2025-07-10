
#pragma once

#include <iostream>
#include <psd/llapi/structure/info/extra_info.h>
#include <psd/llapi/structure/info/layer_info.h>
#include <psd/llapi/structure/info/global_info.h>

namespace PSD::llapi {
//
class Info {
  struct FromStreamFn {
    void operator()(Stream &stream, Info &output) {
      auto length = stream.Read<U32>();
      auto start  = stream.Pos();

      stream.ReadTo(output.layer_info);
      stream.ReadTo(output.global_info);
      stream.ReadTo(output.extra_info, length - (stream.Pos() - start));
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const Info &input) {
      stream.Write(input.ContentLength());
      stream.Write(input.layer_info);
      stream.Write(input.global_info);
      stream.Write(input.extra_info);
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  Info() = default;

  LayerInfo  layer_info;
  GlobalInfo global_info;
  ExtraInfo  extra_info;

  unsigned Length() const {
    return 4 + ContentLength();
  }
private:
  U32 ContentLength() const {
    return layer_info.Length() + global_info.Length() + extra_info.Length();
  }
};
}; // namespace PSD::llapi
