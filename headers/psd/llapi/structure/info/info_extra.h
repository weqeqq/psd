
#pragma once

#include "info_extra/layer16.h"
#include "info_extra/layer32.h"
#include "psd/llapi/structure/info/layer_info/layer_data/layer_data_extra/extra.h"

namespace PSD::llapi {
//
class InfoExtra {
  struct FromStreamFn {
    void operator()(Stream &stream, InfoExtra &output, unsigned length) {
      auto end_of_read = length + stream.Pos();
      while (stream.Pos() < end_of_read) {
        auto header = stream.Read<ExtraHeader>();
        switch (header.id) {
          case ExtraID::Layer16:
            output.Insert(ReadExtraFrom<Layer16>(stream, header)); break;
          case ExtraID::Layer32:
            output.Insert(ReadExtraFrom<Layer32>(stream, header)); break;
          default:
            ReadExtraFrom<DefaultExtra>(stream, header); break;
        }
      }
    }
    template <typename T>
    std::shared_ptr<Extra> ReadExtraFrom(Stream &stream, const ExtraHeader &header) {
      return stream.Read<std::shared_ptr<Extra>>(
        std::static_pointer_cast<Extra>(std::make_shared<T>(stream.Read<T>(header))),
        stream.Pos()
      );
    }
  }; // struct FromStreamFn
  struct ToStreamFn {
    void operator()(Stream &stream, const InfoExtra &input) {
      for (const auto &[id, extra] : input.data_) {
        stream.Write(extra);
      }
    }
  }; // struct ToStreamFn
  friend Stream;
public:
  InfoExtra() = default;
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
  void Erase(ExtraID id) {
    data_.erase(id);
  }
  template <typename T>
  void Erase() {
    Erase(ExtraToID<T>);
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
}; // class ExtraInfo
}; // namespace PSD::llapi
