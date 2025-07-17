
#pragma once

#include <unordered_map>

#include "layer_data_extra/default_extra.h"
#include "layer_data_extra/section_divider.h"

namespace PSD::llapi {
//

class LayerDataExtra {
  struct FromStreamFn {
    void operator()(Stream &stream, LayerDataExtra &output, unsigned length) {
      auto end_of_read = length + stream.Pos();
      while (stream.Pos() < end_of_read) {
        auto header = stream.Read<ExtraHeader>();
        switch (header.id) {
          case ExtraID::SectionDivider:
            output.Insert(ReadExtraFrom<SectionDivider>(stream, header)); break;
          default:
            ReadExtraFrom<DefaultExtra>(stream, header); break;
        }
      }
    }
    template <typename T>
    std::shared_ptr<Extra> ReadExtraFrom(Stream &stream, const ExtraHeader &header) {
      return stream.Read<std::shared_ptr<Extra>>(
        std::static_pointer_cast<Extra>(std::make_shared<T>(stream.Read<T>(header)))
      );
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const LayerDataExtra &input) {
      for (const auto &[id, extra] : input.data_) {
        stream.Write(extra);
      }
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  LayerDataExtra() = default;
  unsigned Length() const {
    return ContentLength();
  }
  void Insert(std::shared_ptr<Extra> extra) {
    data_[extra->ID()] = extra;
  }
  template <typename T>
  void Insert(T &&extra) {
    Insert(std::static_pointer_cast<Extra>(std::make_shared<T>(std::forward<T>(extra))));
  }
  bool Exists(ExtraID id) const {
    return data_.find(id) != data_.end();
  }
  template <typename T>
  bool Exists() const {
    return Exists(ExtraToID<T>);
  }
  std::shared_ptr<Extra> At(ExtraID id) {
    if (!Exists(id)) {
      throw Error("PSD::Error: ExtraError");
    }
    return data_.find(id)->second;
  }
  std::shared_ptr<const Extra> At(ExtraID id) const {
    if (!Exists(id)) {
      throw Error("PSD::Error: ExtraError");
    }
    return data_.find(id)->second;
  }
  template <typename T>
  T &At() {
    return *std::static_pointer_cast<T>(At(ExtraToID<T>));
  }
  template <typename T>
  const T &At() const {
    return *std::static_pointer_cast<const T>(At(ExtraToID<T>));
  }
private:
  std::unordered_map<ExtraID, std::shared_ptr<Extra>> data_;
  U32 ContentLength() const {
    U32 length = 0;
    for (const auto &[id, extra] : data_) {
      length += extra->Length();
    }
    return length;
  }
};
} // namespace PSD::llapi
