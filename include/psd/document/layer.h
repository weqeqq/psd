
#pragma once

#include <psd/structure/main_info/layer_info/layer_data.h>
#include <psd/structure/main_info/layer_info/channel_data.h>
#include <psd/document/element.h>
#include <image/decoder.h>

namespace PSD {

template <Depth::Tp DepthV = Depth::Eight,
          Color::Tp ColorV = Color::RGB>
class Layer : public Element {
public:

  using ChannelDataTp = ChannelData<DepthV, ColorV>;

  using BufferTp  = typename ChannelDataTp::BufferTp;
  using ChannelTp = typename ChannelDataTp::ChannelTp;

  using Pointer      = std::shared_ptr<Layer>;
  using ConstPointer = std::shared_ptr<ConstPointer>;

  explicit Layer(std::string name) : name_(name) {}

  bool operator==(const Layer<DepthV, ColorV> &other) const {
    return buffer_ == other.buffer_ &&
           alpha_  == other.alpha_;
  }
  bool operator!=(const Layer<DepthV, ColorV> &other) const {
    return !operator==(other);
  }

  Layer &SetImage(BufferTp buffer) {
    buffer_       = buffer;
    alpha_ = ChannelTp(buffer_.GetRCount(), buffer_.GetCCount(), Depth::Max<DepthV>);

    UpdateRectangle();
    return *this;
  }
  Layer &SetImage(std::string_view path) {
    return SetImage(
      Image::Decoder(std::string(path)).Decode<DepthV, ColorV>()
    );
  }
  Layer &SetAlpha(ChannelTp alpha) {
    alpha_ = std::move(alpha);

    return *this;
  }

  Layer &SetOffset(std::uint64_t x_offset, std::uint64_t y_offset) {
    rectangle_.top    = y_offset;
    rectangle_.left   = x_offset;
    rectangle_.bottom = y_offset + buffer_.GetRCount();
    rectangle_.right  = x_offset + buffer_.GetCCount();

    return *this;
  }
  Layer &SetBlending(Blending::Tp blending) {
    blending_ = blending;

    return *this;
  }

  std::uint64_t GetRCount() const {
    return rectangle_.bottom - rectangle_.top;
  }
  std::uint64_t GetCCount() const {
    return rectangle_.right - rectangle_.left;
  }

  void UpdateRectangle() {
    rectangle_.top    = 0;
    rectangle_.left   = 0;
    rectangle_.bottom = buffer_.GetRCount();
    rectangle_.right  = buffer_.GetCCount();
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

  BufferTp &GetBuffer() {
    return buffer_;
  }
  const BufferTp &GetBuffer() const {
    return buffer_;
  }

  ChannelTp &GetAlpha() {
    return alpha_;
  }
  const ChannelTp &GetAlpha() const {
    return alpha_;
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
    std::uint32_t output = Color::ChannelCount<ColorV> + 1;

    /*if (transparency_.GetLength()) {*/
    /*  output++;*/
    /*}*/
    if (user_mask_.GetLength()) {
      output++;
    }
    if (real_mask_.GetLength()) {
      output++;
    }
    return output;
  }

  const std::string &GetName() const {
    return name_;
  }
  void SetName(std::string name) {
    name_ = std::move(name);
  }

private:

  BufferTp buffer_;

  ChannelTp alpha_ = ChannelTp();
  ChannelTp user_mask_ = ChannelTp();
  ChannelTp real_mask_ = ChannelTp();

  bool visible_ = true;

  std::string name_;

  Blending::Tp blending_ = Blending::Normal;

  Rectangle rectangle_;

}; // Layer

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

