
#pragma once

#include <psd/core/type/compression.h>

namespace PSD {

template <typename T>
class Compressor {
public:
  Compressor(const T &);

}; // Compressor

template <>
class Compressor<std::vector<std::uint8_t>> {
public:

  Compressor(const std::vector<std::uint8_t> &input) : input_(input) {}

  std::vector<std::uint8_t> Compress(
    Compression::Tp compression, std::uint64_t row_count, std::uint64_t column_count
  ) {
    switch (compression) {
      case Compression::RAW: return input_;
      case Compression::RLE: return CompressRLE(row_count, column_count);
      default: throw std::runtime_error("unsupported");
    }
  }

private:
  const std::vector<std::uint8_t> &input_;

  std::vector<std::uint8_t> CompressRLE(std::uint64_t row_count, std::uint64_t column_count) const;

}; // Compressor<std::vector<std::uint8_t>>

#define PSD_REGISTER_COMPRESSOR_FOR_TYPE(TypeName, ClassName)           \
  class Compressor<TypeName> : public ClassName::Compressor {           \
  public:                                                               \
    Compressor(const TypeName &input) : ClassName::Compressor(input) {} \
  }

#define PSD_REGISTER_COMPRESSOR_FOR_BUFFER(ClassName)                                                             \
  template <Depth::Tp DepthV, Color::Tp ColorV>                                                                   \
  class Compressor<ClassName<DepthV, ColorV>> : public ClassName<DepthV, ColorV>::Compressor {                    \
  public:                                                                                                         \
    explicit Compressor(const ClassName<DepthV, ColorV> &input) : ClassName<DepthV, ColorV>::Compressor(input) {} \
  }

#define PSD_REGISTER_COMPRESSOR(ClassName) \
  PSD_REGISTER_COMPRESSOR_FOR_TYPE(ClassName, ClassName)

}; // PSD
