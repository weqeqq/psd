
#pragma once

#include <psd/core/type/compression.h>

#include <vector>
#include <stdexcept>

namespace PSD {

template <typename T>
class Decompressor {
public:
  explicit Decompressor(const T &) {} 

}; // Decompressor

template <>
class Decompressor<std::vector<std::uint8_t>> {
public:

  Decompressor(const std::vector<std::uint8_t> &input) : input_(input) {}

  std::vector<std::uint8_t> Decompress(
    Compression::Tp compression, std::uint64_t row_count, std::uint64_t column_count
  ) const {
    switch (compression) {
      case Compression::RLE: return DecompressRLE(row_count, column_count);
      default: {
        throw std::runtime_error("Unsupported compression");
      }
    };
  }

private:
  const std::vector<std::uint8_t> &input_;

  std::vector<std::uint8_t> DecompressRLE(std::uint64_t row_count, std::uint64_t column_count) const;

}; // Decompressor<std::vector<std::uint8_t>>

#define PSD_REGISTER_DECOMPRESSOR_FOR_TYPE(TypeName, ClassName)             \
  class Decompressor<TypeName> : public ClassName::Decompressor {           \
  public:                                                                   \
    Decompressor(const TypeName &input) : ClassName::Decompressor(input) {} \
  }

#define PSD_REGISTER_DECOMPRESSOR(ClassName) \
  PSD_REGISTER_DECOMPRESSOR_FOR_TYPE(ClassName, ClassName)

#define PSD_REGISTER_DECOMPRESSOR_FOR_BUFFER(ClassName)                                                               \
  template <Depth::Tp DepthV, Color::Tp ColorV>                                                                       \
  class Decompressor<ClassName<DepthV, ColorV>> : public ClassName<DepthV, ColorV>::Decompressor {                    \
  public:                                                                                                             \
    explicit Decompressor(const ClassName<DepthV, ColorV> &input) : ClassName<DepthV, ColorV>::Decompressor(input) {} \
  }
  
}; // PSD
