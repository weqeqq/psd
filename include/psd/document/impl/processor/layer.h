
#pragma once

#include <psd/document/layer.h>
#include <psd/document/impl/processor.h>

namespace PSD::DocumentImpl {

template <Depth::Tp DepthV, 
          Color::Tp ColorV>
class Processor<Layer<DepthV, ColorV>> {
public:

  explicit Processor(const Layer<DepthV, ColorV> &input) : input_(input) {}

  auto Process() const {
    return input_.GetBuffer();
  }
  auto Process(std::uint64_t row_count, std::uint64_t column_count) const {
    // TODO
  }

private:
  const Layer<DepthV, ColorV> &input_;
};

};

