
#pragma once

namespace PSD::DocumentImpl {

template <typename ElementT>
class Processor {
public:
  static_assert(false);
  explicit Processor(ElementT) {}
};

#define PSD_REGISTER_PROCESSOR(ClassName)           \
  template <Depth::Tp DepthV,                       \
            Color::Tp ColorV>                       \
  class Processor<ClassName<DepthV, ColorV>>        \
    : public ClassName<DepthV, ColorV>::Processor { \
  public:                                           \
    using ClassName<DepthV, ColorV>::Processor;     \
  }; 

};
