
#pragma once

#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/main_info/layer_info/channel_data.h>
#include <psd/document/element.h>
#include <image/decode.h>

namespace PSD {

template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB>
class Layer : public Element {
public:

  using UsedChannelData = ChannelData<DepthV, ColorV>;

  using UsedBuffer  = typename UsedChannelData::UsedBuffer;

  using Pointer      = std::shared_ptr<Layer>;
  using ConstPointer = std::shared_ptr<ConstPointer>;

  explicit Layer(std::string name) : name_(name) {}

  bool operator==(const Layer<DepthV, ColorV> &other) const {
    return buffer_ == other.buffer_;
  }
  bool operator!=(const Layer<DepthV, ColorV> &other) const {
    return !operator==(other);
  }

  Layer &SetImage(UsedBuffer buffer) {
    buffer_ = buffer;

    UpdateRectangle();
    return *this;
  }
  Layer &SetImage(const std::filesystem::path &path) {
    return SetImage(
      Image::Decode<DepthV, ColorV>(path)
    );
  }
  Layer &SetImage(const std::string &path) {
    return SetImage(path);
  }
  Layer &SetImage(const char *path) {
    return SetImage(path);
  }

  Layer &SetOffset(std::uint64_t x_offset, std::uint64_t y_offset) {
    rectangle_.top    = y_offset;
    rectangle_.left   = x_offset;
    rectangle_.bottom = y_offset + buffer_.GetRowCount();
    rectangle_.right  = x_offset + buffer_.GetColumnCount();

    return *this;
  }
  Layer &SetBlending(Blending::Tp blending) {
    blending_ = blending;

    return *this;
  }

  std::uint64_t GetRowCount() const {
    return rectangle_.bottom - rectangle_.top;
  }
  std::uint64_t GetColumnCount() const {
    return rectangle_.right - rectangle_.left;
  }

  void UpdateRectangle() {
    rectangle_.top    = 0;
    rectangle_.left   = 0;
    rectangle_.bottom = buffer_.GetRowCount();
    rectangle_.right  = buffer_.GetColumnCount();
  }

  virtual bool IsGroup() const override {
    return false;
  }
  virtual bool IsLayer() const override {
    return true; 
  }

  virtual Element::Pointer Clone() const override {
    return Element::Clone(this);
  }
  virtual bool Compare(Element::ConstPointer other) const override {
    return Element::Compare(this, other);
  }

  virtual std::uint64_t GetTop() const override {
    return rectangle_.top;
  }
  virtual std::uint64_t GetLeft() const override {
    return rectangle_.left;
  }
  virtual std::uint64_t GetBottom() const override {
    return rectangle_.bottom;
  }
  virtual std::uint64_t GetRight() const override {
    return rectangle_.right;
  }

  UsedBuffer &GetBuffer() {
    return buffer_;
  }
  const UsedBuffer &GetBuffer() const {
    return buffer_;
  }

  Blending::Tp GetBlending() const {
    return blending_;
  }

  Rectangle &GetRectangle() {
    return rectangle_;
  }
  const Rectangle &GetRectangle() const {
    return rectangle_;
  }

  std::uint32_t GetChannelCount() const {
    return Color::ChannelCount<ColorV, Image::EnableAlpha>;
  }

  const std::string &GetName() const {
    return name_;
  }
  void SetName(std::string name) {
    name_ = std::move(name);
  }

private:

  UsedBuffer buffer_;
  bool visible_ = true;

  std::string name_;

  Blending::Tp blending_ = Blending::Normal;

  Rectangle rectangle_;
}; 
template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB>
decltype(auto) LayerCast(Element::Pointer input) {
  return *std::static_pointer_cast<Layer<DepthV, ColorV>>(input);
}
template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB>
decltype(auto) LayerCast(Element::ConstPointer input) {
  return *std::static_pointer_cast<const Layer<DepthV, ColorV>>(input);
}
};

