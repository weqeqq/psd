
#pragma once

#include <psd/document/group.h>
#include <psd/document/impl/convertor/layer.h>
#include <psd/structure/main_info/layer_info.h>

namespace PSD::DocumentImpl {

template <typename T>
class GroupConvertor {
public:

  static_assert(false);

  explicit GroupConvertor(T) {}
};

template <Depth::Tp DepthV, 
          Color::Tp ColorV> 
class GroupConvertor<Group<DepthV, ColorV>> {
public:

  explicit GroupConvertor(const Group<DepthV, ColorV> &input) 
    : input_(input) {}

  constexpr bool CanBeConverted() const {
    return true;
  }
  std::vector<LayerInfoElement<DepthV, ColorV>> Convert() const {
    std::vector<LayerInfoElement<DepthV, ColorV>> output;
    output.push_back(CreateStart());

    for (const auto &element : input_) {
      if (element->IsLayer()) {
        output.push_back(
          LayerConvertor(LayerCast<DepthV, ColorV>(element)).Convert()
        );
        continue;
      } 
      if (element->IsGroup()) {
        auto input = GroupConvertor(GroupCast<DepthV, ColorV>(element)).Convert();
        output.insert(
          output .end   (),
          input  .begin (),
          input  .end   ()
        );
        continue;
      }
      throw std::runtime_error("how did u get this err");
    }
    output.push_back(CreateEnd());
    return output;
  }
private:
  const Group<DepthV, ColorV> &input_;

  void AssignStart(LayerData &layer_data) const {
    layer_data.channel_count = Color::ChannelCount<ColorV, Image::EnableAlpha>;

    layer_data.blending = Blending::Normal;
    layer_data.opacity  = 0xff;
    layer_data.clipping = false;

    layer_data.useful_info               = true; 
    layer_data.irrelievant_to_appearance = true;

    layer_data.name = "</Layer group>";

    layer_data.extra_info.Emplace<UnicodeName>(layer_data.name);
    layer_data.extra_info.Emplace<SectionDivider>(DividerType::Hidden);
  }
  LayerInfoElement<DepthV, ColorV> CreateStart() const {
    decltype(CreateStart()) output;

    AssignStart(output.layer_data);

    output.channel_data.buffer = Image::AlphaBuffer<DepthV, ColorV>();
    // output.channel_data.alpha  = Image::Buffer<DepthV, Color::Grayscale>();
    return output;
  }
  void AssignEnd(LayerData &layer_data) const {
    layer_data.channel_count = Color::ChannelCount<ColorV, Image::EnableAlpha>;

    layer_data.blending = input_.GetBlending();
    layer_data.opacity  = input_.GetOpacity();
    layer_data.clipping = input_.GetClipping();

    layer_data.useful_info               = true;
    layer_data.irrelievant_to_appearance = true;

    layer_data.name = input_.GetName();

    layer_data.extra_info.Emplace<UnicodeName>(layer_data.name);
    layer_data.extra_info.Emplace<SectionDivider>(DividerType::ClosedFolder);
  }
  LayerInfoElement<DepthV, ColorV> CreateEnd() const {
    LayerInfoElement<DepthV, ColorV> output;

    AssignEnd(output.layer_data);

    output.channel_data.buffer = Image::AlphaBuffer<DepthV, ColorV>();
    // output.channel_data.alpha  = Image::Buffer<DepthV, Color::Grayscale>();
    return output;
  }
};

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
static bool IsLayer(const LayerInfoElement<DepthV, ColorV> &element) {
  return element.layer_data.extra_info
    .template Get<SectionDivider>().type == DividerType::Layer;
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
static bool IsGroupStart(const LayerInfoElement<DepthV, ColorV> &element) {
  return element.layer_data.extra_info
    .template Get<SectionDivider>().type == DividerType::Hidden;
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
static bool IsGroupEnd(const LayerInfoElement<DepthV, ColorV> &element) {
  auto type = element.layer_data.extra_info
    .template Get<SectionDivider>().type;

  return type == DividerType::OpenFolder ||
         type == DividerType::ClosedFolder;
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
static auto FindGroupEnd(
  typename std::vector<LayerInfoElement<DepthV, ColorV>>::const_iterator begin,
  typename std::vector<LayerInfoElement<DepthV, ColorV>>::const_iterator end
) {
  for (auto iterator = begin; iterator != end; iterator++) {
    if (IsGroupEnd(*iterator)) {
      return ++iterator;
    }
  }
  return end;
}
template <Depth::Tp DepthV, 
          Color::Tp ColorV>
static std::uint64_t GroupLayerCount(const Group<DepthV, ColorV> &group) {
  std::uint64_t output = 2;

  for (decltype(auto) element : group) {
    if (element->IsLayer()) {
      output++;
      continue;
    }
    if (element->IsGroup()) {
      output += GroupLayerCount(GroupCast<DepthV, ColorV>(element));
      continue;
    }
    throw GroupError::UndefinedElement();
  }
  return output;
}
template <typename StructureT>
using FromStructure = LayerInfoElement<
  StructureT::DepthValue, 
  StructureT::ColorValue
>;
template <Depth::Tp DepthV,
          Color::Tp ColorV>
class GroupConvertor<LayerInfoElement<DepthV, ColorV>> {
public:

  using ElementTp  = LayerInfoElement<DepthV, ColorV>;
  using IteratorTp = typename std::vector<ElementTp>::const_iterator;

  explicit GroupConvertor(const std::vector<LayerInfoElement<DepthV, ColorV>> &input)
    : begin_ (input.begin())
    , end_   (input.end()) {}

  explicit GroupConvertor(IteratorTp begin, IteratorTp end) 
    : begin_ (begin)
    , end_   (end) {}

  bool CanBeConverted() const {
    return IsGroupStart(*begin_) && IsGroupEnd(*(end_ - 1));
  }

  Group<DepthV, ColorV> Convert() const {
    if (!CanBeConverted()) {
      throw GroupError::CannotBeConverted();
    }
    Group<DepthV, ColorV> output;

    for (auto iterator = begin_ + 1; iterator != end_;) {
      if (IsLayer(*iterator)) {
        output.Push(LayerConvertor(*iterator++).Convert());
        continue;
      }
      if (IsGroupStart(*iterator)) {
        output.Push(GroupConvertor<LayerInfoElement<DepthV, ColorV>>(
          iterator, 
          FindGroupEnd<DepthV, ColorV>(iterator, end_)
        ).Convert());
        iterator += GroupLayerCount(output.BackGroup());
        continue;
      }
      if (IsGroupEnd(*iterator)) {
        output.SetBlending (iterator->layer_data.blending);
        output.SetName     (iterator->layer_data.name);
        output.SetOpacity  (iterator->layer_data.opacity);
        iterator++;
        break;
      }
      throw std::runtime_error("how did u get this err");
    }
    return output;
  }

private:
  IteratorTp begin_;
  IteratorTp end_;

};

};
