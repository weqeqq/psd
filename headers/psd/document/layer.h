
#pragma once

#include "psd/llapi/structure/info/layer_info/layer_data.h"
#include <cstddef>
#include <memory>
#include <psd/document/entry.h>
#include <string>
#include <vector>
#include <psd/llapi/structure.h>

#include <image/image.h>

namespace PSD {
//
class Layer : public EntryFor<Layer> {
  auto Comparable() const {
    return std::tie(name_, xoffset_, yoffset_, image_);
  }
public:
  Layer(const std::string &name) : name_(name) {}

  bool IsLayer() const override final { return true; }
  bool IsGroup() const override final { return false; }

  bool operator==(const Layer &other) const {
    return Comparable() == other.Comparable();
  }
  bool operator!=(const Layer &other) const {
    return !operator==(other);
  }

  llapi::Coordinates Coordinates() const {
    return llapi::Coordinates{
      yoffset_,
      xoffset_,
      xoffset_ + image_.RowCount(),
      yoffset_ + image_.ColumnCount()
    };
  }
  unsigned Top()    const override final { return Coordinates().top;    }
  unsigned Left()   const override final { return Coordinates().left;   }
  unsigned Bottom() const override final { return Coordinates().bottom; }
  unsigned Right()  const override final { return Coordinates().right;  }

  void SetOffset(unsigned xoffset, unsigned yoffset) {
    xoffset_ = xoffset;
    yoffset_ = yoffset;
  }
  void SetImage(::Image::Buffer<> image) {
    image_ = std::move(image);
  }
  ::Image::Buffer<> &Image() {
    return image_;
  }
  const ::Image::Buffer<> &Image() const {
    return image_;
  }
  void SetName(std::string name) {
    name_ = std::move(name);
  }
  std::string &Name() {
    return name_;
  }
  const std::string &Name() const {
    return name_;
  }
private:
  std::string name_;
  unsigned xoffset_ = 0;
  unsigned yoffset_ = 0;
  ::Image::Buffer<> image_;
}; // class Layer
inline Layer &LayerCast(std::shared_ptr<Entry> input) {
  return *std::static_pointer_cast<Layer>(input);
}
inline const Layer &LayerCast(std::shared_ptr<const Entry> input) {
  return *std::static_pointer_cast<const Layer>(input);
}
}; // namespace PSD::llapi
