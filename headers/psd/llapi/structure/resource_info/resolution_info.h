#pragma once

#include <psd/llapi/structure/resource_info/resource.h>

namespace PSD::llapi {
//
enum class DisplayUnit : U16 {
  Inch   = 1,
  Cm     = 2,
  Point  = 3,
  Pica   = 4,
  Column = 5,
};
template <>
struct FromStreamFn<DisplayUnit> {
  void operator()(Stream &stream, DisplayUnit &display_unit) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<DisplayUnit> &>(display_unit));
  }
}; // struct FromStreamFn<DisplayUnit>
template <>
struct ToStreamFn<DisplayUnit> {
  void operator()(Stream &stream, DisplayUnit display_unit) {
    stream.Write(static_cast<std::underlying_type_t<DisplayUnit>>(display_unit));
  }
}; // struct ToStreamFn<DisplayUnit>
enum class ResolutionUnit : U16 {
  PerInch = 1,
  PerCm   = 2,
};
template <>
struct FromStreamFn<ResolutionUnit> {
  void operator()(Stream &stream, ResolutionUnit &resolution_unit) {
    stream.ReadTo(reinterpret_cast<std::underlying_type_t<ResolutionUnit> &>(resolution_unit));
  }
}; // struct FromStreamFn<ResolutionUnit>
template <>
struct ToStreamFn<ResolutionUnit> {
  void operator()(Stream &stream, ResolutionUnit resolution_unit) {
    stream.Write(static_cast<std::underlying_type_t<ResolutionUnit>>(resolution_unit));
  }
}; // struct ToStreamFn<ResolutionUnit>
class ResolutionInfo : public ResourceFor<ResolutionInfo> {
  friend ResourceFor<ResolutionInfo>;
  struct FromStreamFn {
    void operator()(Stream &stream, ResolutionInfo &output, const ResourceHeader &) {
      ReadResolution(stream, output.hr);
      stream.ReadTo(output.hrunit);
      stream.ReadTo(output.wunit);
      ReadResolution(stream, output.vr);
      stream.ReadTo(output.vrunit);
      stream.ReadTo(output.hunit);
    }
    void ReadResolution(Stream &stream, float &resolution) {
      resolution =
        static_cast<float>(stream.Read<U32>()) /
        static_cast<float>(1 << 16);
    }
  }; // struct ResolutionInfo
  struct ToStreamFn {
    void operator()(Stream &stream, const ResolutionInfo &input) {
      WriteResolution(stream, input.hr);
      stream.Write(input.hrunit);
      stream.Write(input.wunit);
      WriteResolution(stream, input.vr);
      stream.Write(input.vrunit);
      stream.Write(input.hunit);
    }
    void WriteResolution(Stream &stream, float resolution) {
      stream.Write(U32(resolution * (1 << 16)));
    }
  }; // struct ToStreamFn
  friend Stream;
  auto Comparable() const {
    return std::tie(
      hr, hrunit, wunit,
      vr, vrunit, hunit
    );
  }
public:
  ResolutionInfo() = default;
  bool operator==(const ResolutionInfo &other) const {
    return Comparable() == other.Comparable();
  }
  float           hr     = 72.0;
  ResolutionUnit  hrunit = ResolutionUnit::PerInch;
  DisplayUnit     wunit  = DisplayUnit::Inch;
  float           vr     = 72.0;
  ResolutionUnit  vrunit = ResolutionUnit::PerInch;
  DisplayUnit     hunit  = DisplayUnit::Inch;
private:
  ResourceHeader header_ = {ResourceID::ResolutionInfo, 16};
}; // class LayerState
}; // namespace PSD::llapi
