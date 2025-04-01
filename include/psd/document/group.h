
#pragma once

#include <psd/document/element.h>
#include <image/processing/blend.h>
#include <psd/document/layer.h>
#include <psd/core/type/error.h>

namespace PSD {

class GroupError : public Error {
public:
  class NoSuchElement;
  class NoSuchGroup;
  class NoSuchLayer;
  class EmptyGroup;
  class BadCast;
  class CannotBeConverted;
  class UndefinedElement;

protected:
  using Error::Error;
};
class GroupError::NoSuchElement : public GroupError {
public:
  explicit NoSuchElement() : GroupError("Element not found") {}
};
class GroupError::NoSuchGroup : public GroupError {
public:
  explicit NoSuchGroup() : GroupError("Group not found") {}
};
class GroupError::NoSuchLayer : public GroupError {
public:
  explicit NoSuchLayer() : GroupError("Layer not found") {} 
};
class GroupError::EmptyGroup : public GroupError {
public:
  explicit EmptyGroup() : GroupError("Operation cannot be performed, the group is empty") {}
};
class GroupError::BadCast : public GroupError {
public:
  explicit BadCast() : GroupError("Bad group cast") {}
};
class GroupError::CannotBeConverted : public GroupError {
public:
  explicit CannotBeConverted() : GroupError("Cannot be converted") {}
};
class GroupError::UndefinedElement : public GroupError {
public:
  explicit UndefinedElement() : GroupError("Undefined element") {}
};

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
class Group : public Element {
public:

  class Processor;

  using BufferTp  = Image::Buffer<DepthV, ColorV>;
  using ChannelTp = Image::Buffer<DepthV, Color::Grayscale>;

  using Pointer      = std::shared_ptr<Group>;
  using ConstPointer = std::shared_ptr<Group const>;

  explicit Group() = default;

  explicit Group(const std::string &name) : name_(name) {}

  bool operator==(const Group<DepthV, ColorV> &other) const {
    auto compare = [](const auto &left, const auto &right) {
      return left->Compare(right);
    };
    return std::equal(
      element_list_.begin(), 
      element_list_.end(), 
      other.element_list_.begin(), 
      compare
    );
  }
  bool operator!=(const Group<DepthV, ColorV> &other) const {
    return !operator==(other);
  }

  Element::Pointer operator[](std::uint64_t index) {
    return element_list_[index];
  }
  Element::ConstPointer operator[](std::uint64_t index) const {
    return element_list_[index];
  }

  std::uint64_t FindRCount() const {
    return GetBottom() - GetTop();
  }
  std::uint64_t FindCCount() const {
    return GetRight() - GetLeft();
  }

  template <typename GroupT>
  class IteratorClass {
  public:

    using iterator_category = std::forward_iterator_tag;
    using value_type        = Element::Pointer; 
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type *;
    using reference         = value_type &;

    explicit IteratorClass(GroupT group, std::uint64_t current) 
      : group_   (group)
      , current_ (current) {}

    decltype(auto) operator*() { return (*group_)[current_]; }

    IteratorClass &operator++() { current_++; return *this; }
    IteratorClass &operator--() { current_--; return *this; }

    IteratorClass operator++(int) { auto tmp = *this; operator++(); return *this; }
    IteratorClass operator--(int) { auto tmp = *this; operator--(); return *this; }

    bool operator==(const IteratorClass &other) const { return current_ == other.current_; }
    bool operator!=(const IteratorClass &other) const { return !operator==(other); }

  private:
    GroupT        group_;
    std::uint64_t current_;
  };
  template <typename GroupT>
  class RevIteratorClass {
  public:

    using iterator_category = std::forward_iterator_tag;
    using value_type        = Element::Pointer; 
    using difference_type   = std::ptrdiff_t;
    using pointer           = value_type *;
    using reference         = value_type &;

    explicit RevIteratorClass(GroupT group, std::uint64_t current) 
      : group_   (group)
      , current_ (current) {}

    decltype(auto) operator*() { return (*group_)[current_ - 1]; }

    RevIteratorClass &operator++() { current_--; return *this; }
    RevIteratorClass &operator--() { current_++; return *this; }

    RevIteratorClass operator++(int) { auto tmp = *this; operator++(); return *this; }
    RevIteratorClass operator--(int) { auto tmp = *this; operator--(); return *this; }

    bool operator==(const RevIteratorClass &other) const { return current_ == other.current_; }
    bool operator!=(const RevIteratorClass &other) const { return !operator==(other); }

  private:
    GroupT        group_;
    std::uint64_t current_;
  };
  using Iterator      = IteratorClass<      Group *>;
  using ConstIterator = IteratorClass<const Group *>;

  using RevIterator      = RevIteratorClass<      Group *>;
  using ConstRevIterator = RevIteratorClass<const Group *>;

  auto Begin() {
    return IteratorClass(this, 0);
  }
  auto Begin() const {
    return IteratorClass(this, 0);
  }
  auto ConstBegin() const {
    return ConstIterator(this, 0);
  }
  auto RevBegin() {
    return RevIteratorClass(this, GetLength());
  }
  auto RevBegin() const {
    return RevIteratorClass(this, GetLength());
  }
  auto ConstRevBegin() const {
    return RevBegin();
  }
  auto End() {
    return IteratorClass(this, GetLength());
  }
  auto End() const {
    return IteratorClass(this, GetLength());
  }
  auto ConstEnd() const {
    return End();
  }
  auto RevEnd() {
    return RevIteratorClass(this, 0);
  }
  auto RevEnd() const {
    return RevIteratorClass(this, 0);
  }
  auto ConstRevEnd() const {
    return RevEnd();
  }

  virtual bool IsGroup() const override {
    return true;
  }
  virtual bool IsLayer() const override {
    return false;
  }

  virtual Element::Pointer Clone() const override {
    return Element::Clone(this);
  }
  virtual bool Compare(Element::ConstPointer other) const override {
    return Element::Compare(this, other);
  }

  virtual std::uint64_t GetTop() const override {
    if (IsEmpty()) {
      return 0;
    }
    auto result = *std::min_element(Begin(), End(), [](auto element, auto smallest) {
      return element->GetTop() < 
             smallest->GetTop();
    });
    return result->GetTop();
  }
  virtual std::uint64_t GetLeft() const override {
    if (IsEmpty()) {
      return 0;
    }
    auto result = *std::min_element(Begin(), End(), [](auto element, auto smallest) {
      return element->GetLeft() <
             smallest->GetLeft();
    });
    return result->GetLeft();
  }
  virtual std::uint64_t GetBottom() const override {
    if (IsEmpty()) {
      return 0;
    }
    auto result = *std::max_element(Begin(), End(), [](auto largest, auto element) {
      return largest->GetBottom() <
             element->GetBottom();
    });
    return result->GetBottom();
  }
  virtual std::uint64_t GetRight() const override {
    if (IsEmpty()) {
      return 0;
    }
    auto result = *std::max_element(Begin(), End(), [](auto largest, auto element) {
      return largest->GetRight() <
             element->GetRight();
    });
    return result->GetRight();
  }

  std::uint64_t GetLength() const {
    return element_list_.size();
  }
  std::uint64_t GetElementCount() const {
    return GetLength();
  }
  std::uint64_t GetLayerCount() const {
    return std::count_if(Begin(), End(), [](const auto &element) {
      return element->IsLayer();
    });
  }
  std::uint64_t GetGroupCount() const {
    return std::count_if(Begin(), End(), [](const auto &element) {
      return element->IsGroup();
    });
  }

  template <typename ElementT>
  void Push(ElementT element) {
    element_list_.push_back(Element::Create<ElementT>(std::move(element)));
  }

  bool HasGroups() const {
    return std::any_of(Begin(), End(), [](auto element) {
      return element->IsGroup();
    });
  }
  bool HasLayers() const {
    if (std::any_of(Begin(), End(), [](auto element) {
      return element->IsLayer();
    })) {
      return true;
    }
    for (auto iterator = Begin(); iterator != End(); iterator++) {
      if ((*iterator)->HasLayers()) {
        return true;
      }
    }
    return false;
  }

  static decltype(auto) Cast(Element::Pointer element) {
    if (!element->IsGroup()) {
      throw GroupError::BadCast();
    }
    return *std::static_pointer_cast<Group>(element);
  }
  static decltype(auto) Cast(Element::ConstPointer element) {
    if (!element->IsGroup()) {
      throw GroupError::BadCast();
    }
    return *std::static_pointer_cast<const Group>(element);
  }

  bool IsEmpty() const {
    return !GetLength();
  }

  decltype(auto) FrontElement() {
    return FrontElement(*this);
  }
  decltype(auto) FrontElement() const {
    return FrontElement(*this);
  }
  decltype(auto) BackElement() {
    return BackElement(*this);
  }
  decltype(auto) BackElement() const {
    return BackElement(*this);
  }
  decltype(auto) FrontLayer() {
    return FrontLayer(*this);
  }
  decltype(auto) FrontLayer() const {
    return FrontLayer(*this);
  }
  decltype(auto) BackLayer() {
    return BackLayer(*this);
  }
  decltype(auto) BackLayer() const {
    return BackLayer(*this);
  }
  decltype(auto) FrontGroup() {
    return FrontGroup(*this);
  }
  decltype(auto) FrontGroup() const {
    return FrontGroup(*this);
  }
  decltype(auto) BackGroup() {
    return BackGroup(*this);
  }
  decltype(auto) BackGroup() const {
    return BackGroup(*this);
  }

  const std::string GetName() const {
    return name_;
  }
  Group SetName(const std::string &name) {
    name_ = name;
    return *this;
  }

  Group &SetBlending(Blending::Tp blending) {
    blending_ = blending;

    return *this;
  }
  Blending::Tp GetBlending() const {
    return blending_;
  }

  Group &SetClipping(bool clipping) {
    clipping_ = clipping;

    return *this;
  }
  Group &EnableClipping() {
    return SetClipping(true);
  }
  Group &DisableClipping() {
    return SetClipping(false);
  }
  Group &ToggleClipping() {
    return SetClipping(!clipping_);
  }
  bool GetClipping() const {
    return clipping_;
  }

  Group &SetOpacity(std::uint8_t opacity) {
    opacity_ = opacity;
    return *this;
  }
  std::uint8_t GetOpacity() const {
    return opacity_;
  }

private:
  std::vector<Element::Pointer> element_list_;

  std::string  name_;
  Blending::Tp blending_ = Blending::Normal;
  bool         clipping_ = false;
  std::uint8_t opacity_  = 0xff;

  template <typename Self>
  static decltype(auto) FrontElement(Self &&self) {
    if (self.IsEmpty()) {
      throw GroupError::EmptyGroup();
    }
    return *self.Begin();
  }
  template <typename Self>
  static decltype(auto) BackElement(Self &&self) {
    if (self.IsEmpty()) {
      throw GroupError::EmptyGroup();
    }
    return *self.RevBegin();
  }
  template <typename Self>
  static decltype(auto) FrontLayer(Self &&self) {
    auto iterator = std::find_if(
      self.Begin (),
      self.End   (),
      [](auto element) { return element->IsLayer(); }
    );
    if (iterator == self.End()) {
      throw GroupError::NoSuchLayer();
    }
    return LayerCast(*iterator);
  }
  template <typename Self>
  static decltype(auto) BackLayer(Self &&self) {
    auto iterator = std::find_if(
      self.RevBegin (),
      self.RevEnd   (),
      [](auto element) { return element->IsLayer(); }
    );
    if (iterator == self.RevEnd()) {
      throw GroupError::NoSuchLayer();
    }
    return LayerCast(*iterator);
  }
  template <typename Self>
  static decltype(auto) FrontGroup(Self &&self) {
    auto iterator = std::find_if(
      self.Begin (),
      self.End   (),
      [](auto element) { return element->IsGroup(); }
    );
    if (iterator == self.End()) {
      throw GroupError::NoSuchGroup();
    }
    return self.Cast(*iterator);
  }
  template <typename Self>
  static decltype(auto) BackGroup(Self &&self) {
    auto iterator = std::find_if(
      self.RevBegin (),
      self.RevEnd   (),
      [](auto element) { return element->IsGroup(); }
    );
    if (iterator == self.RevEnd()) {
      throw GroupError::NoSuchGroup();
    }
    return self.Cast(*iterator);
  }

};
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
auto begin(Group<DepthV, ColorV> &input) {
  return input.Begin();
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
auto begin(const Group<DepthV, ColorV> &input) {
  return input.Begin();
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
auto end(Group<DepthV, ColorV> &input) {
  return input.End();
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
auto end(const Group<DepthV, ColorV> &input) {
  return input.End();
}

template <Depth::Tp DepthV,
          Color::Tp ColorV>
decltype(auto) GroupCast(Element::Pointer input) {
  return Group<DepthV, ColorV>::Cast(input);
}
template <Depth::Tp DepthV,
          Color::Tp ColorV>
decltype(auto) GroupCast(Element::ConstPointer input) {
  return Group<DepthV, ColorV>::Cast(input);
}

}; 
