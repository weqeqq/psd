
#pragma once

namespace PSD {

template <typename Te>
class LengthCalculator {
public:

  static_assert(false);

  LengthCalculator(Te) {}

}; // LengthCalculator

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

}; // PSD

