
#pragma once

#include <psd/document/impl/convertor/layer.h>
#include <psd/document/impl/convertor/group.h>
#include <psd/document/group.h>

namespace PSD::DocumentImpl {

template <Depth::Tp DepthV = Depth::Eight, 
          Color::Tp ColorV = Color::RGB>
using Root = Group<DepthV, ColorV>;

template <typename T> 
class RootConvertor {
public:
  static_assert(false);
  explicit RootConvertor(T) {}
};
template <Depth::Tp DepthV,
          Color::Tp ColorV>
class RootConvertor<Root<DepthV, ColorV>> {
public:

  explicit RootConvertor(const Root<DepthV, ColorV> &input) : input_(input) {}

  LayerInfo<DepthV, ColorV> Convert() const {
    LayerInfo<DepthV, ColorV> output;

    for (decltype(auto) element : input_) {
      if (element->IsLayer()) {
        ConvertLayer(output, element);
        continue;
      } 
      if (element->IsGroup()) {
        ConvertGroup(output, element);
        continue;
      }
      throw GroupError::UndefinedElement();
    }
    return output;
  }
  
private:
  const Root<DepthV, ColorV> &input_;

  static void ConvertLayer(LayerInfo<DepthV, ColorV> &output, Element::ConstPointer element) {
    output.Push(
      LayerConvertor(LayerCast<DepthV, ColorV>(element)).Convert()
    );
  }
  static void ConvertGroup(LayerInfo<DepthV, ColorV> &output, Element::ConstPointer element) {
    auto input = GroupConvertor(GroupCast<DepthV, ColorV>(element)).Convert();
    output.Insert(
      output.End(),
      input.begin(),
      input.end()
    );
  }
};
template <Depth::Tp DepthV,
          Color::Tp ColorV>
class RootConvertor<LayerInfo<DepthV, ColorV>> {
public:

  explicit RootConvertor(const LayerInfo<DepthV, ColorV> &input) : input_(input) {}

  auto Convert() const {
    Root<DepthV, ColorV> output;

    for (auto iterator  = input_.Begin ();
              iterator != input_.End   ();) {
      if (IsLayer(*iterator)) {
        output.Push(LayerConvertor(*iterator++).Convert());
        continue;
      }
      if (IsGroupStart(*iterator)) {
        output.Push(GroupConvertor<LayerInfoElement<DepthV, ColorV>>(
          iterator,
          FindGroupEnd<DepthV, ColorV>(iterator, input_.End())
        ).Convert());
        iterator += GroupLayerCount(output.BackGroup());
        continue;
      }
      throw GroupError::UndefinedElement();
    }
    return output;
  }

private:
  const LayerInfo<DepthV, ColorV> &input_;
};
};


