
#pragma once

#include <vector>
#include <array>
#include <cstdint>

namespace PSD {

template <typename ValueT>
class LengthCalculator {
public:
  explicit LengthCalculator(const ValueT &input) : input_(input) {}

  std::uint64_t Calculate() { 
    return sizeof input_; 
  }
private:
  const ValueT &input_;
}; 
template <typename ValueT>
class LengthCalculator<std::vector<ValueT>> {
public:
  using UsedVector = std::vector<ValueT>;

  explicit LengthCalculator(const UsedVector &input) : input_(input) {}

  std::uint64_t Calculate() {
    return input_.size();
  }
private:
  const UsedVector &input_;
};

template <typename ValueT, std::uint64_t Length>
class LengthCalculator<std::array<ValueT, Length>> {
public:
  using UsedArray = std::array<ValueT, Length>;

  explicit LengthCalculator(const UsedArray &input) : input_(input) {}

  constexpr std::uint64_t Calculate() {
    return Length;
  }
private:
  const UsedArray &input_;
};

#define PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(TypeName, ClassName)                \
  template <>                                                                       \
  class LengthCalculator<TypeName> : public ClassName::LengthCalculator {           \
  public:                                                                           \
    explicit LengthCalculator(const TypeName &input) : ClassName::LengthCalculator(input) {} \
  }

#define PSD_REGISTER_LENGTH_CALCULATOR(ClassName) \
  PSD_REGISTER_LENGTH_CALCULATOR_FOR_TYPE(ClassName, ClassName)

#define PSD_REGISTER_LENGTH_CALCULATOR_FOR_BUFFER(ClassName)                                                                  \
  template <Depth::Tp DepthV, Color::Tp ColorV>                                                                               \
  class LengthCalculator<ClassName<DepthV, ColorV>> : public ClassName<DepthV, ColorV>::LengthCalculator {                    \
  public:                                                                                                                     \
    explicit LengthCalculator(const ClassName<DepthV, ColorV> &input) : ClassName<DepthV, ColorV>::LengthCalculator(input) {} \
  }

};

