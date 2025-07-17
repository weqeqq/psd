
#include "psd/llapi/stream.h"
#include "psd/llapi/structure/header.h"
#include <cstddef>
#include <iterator>
#include <psd/llapi/structure/info/layer_info/channel_data.h>
#include <libdeflate.h>
#include <xsimd/xsimd.hpp>

#if PSD_COMPILER_MSVC
#include <intrin.h>
#endif

namespace PSD::llapi {
//
template <typename I, typename O>
void DecompressLine(I input, I end, O output) {
  while (input != end) {
    I16 count = static_cast<I8>(*input++);
    if (count < 0) {
      count = -count + 1;
      std::fill(output, output + count, *input++);
      output += count;
    } else {
      count++;
      std::copy(input, input + count, output);
      input  += count;
      output += count;
    }
  }
}
std::vector<U8> DecompressDefault(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
) {
  std::vector<U8> output(row_count * column_count * ByteCount(depth));

  auto ooffset = 0ul;
  auto ioffset = 0ul;
  auto citerator = input.begin();

  const auto clength = row_count * sizeof(U16);
  for (auto index = 0ul;
            index < row_count;
            index++) {
    U16 count = 0;
    U8 *ccount = reinterpret_cast<U8 *>(&count);
    #if PSD_LITTLE_ENDIAN
    ccount[1] = *citerator++;
    ccount[0] = *citerator++;
    #else
    ccount[0] = *citerator++;
    ccount[1] = *citerator++;
    #endif
    DecompressLine(
      input.begin() + clength + ioffset,
      input.begin() + clength + ioffset + count,
      output.begin() + ooffset
    );
    ioffset += count;
    ooffset += column_count * ByteCount(depth);
  }
  return output;
}
std::vector<U8> DecompressDeflate(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
) {
  std::vector<U8> output(row_count * column_count * ByteCount(depth));
  auto decompressor = libdeflate_alloc_decompressor();
  if (!decompressor) {
    throw Error("PSD::Error: DecompressorError");
  }
  std::size_t decompressed = 0ul;
  if (libdeflate_zlib_decompress(
    decompressor,
    input.data(),
    input.size(),
    output.data(),
    output.size(),
    &decompressed
  ) != LIBDEFLATE_SUCCESS) {
    libdeflate_free_decompressor(decompressor);
    throw Error("PSD::Error: DecompressionError");
  };
  libdeflate_free_decompressor(decompressor);
  return output;
}
namespace {
//
void DecodeDelta8(
  std::vector<U8> &data,
  unsigned row_count,
  unsigned column_count
) {
  for (auto row = 0u;
            row < row_count;
            row++) {
    auto offset = row * column_count;
    for (auto column = 1u;
              column < (column_count);
              column++) {
      data[offset + column] += data[offset + column - 1];
    }
  }
}
void DecodeDelta16(
  std::vector<U8> &data,
  unsigned row_count,
  unsigned column_count
) {
  auto &data16 = reinterpret_cast<std::vector<U16> &>(data);
  #ifdef PSD_LITTLE_ENDIAN
  for (auto &value : data16)  value = detail::ByteSwap(value);
  #endif
  for (auto row = 0u;
            row < row_count;
            row++) {
    auto offset = row * column_count;
    for (auto column = 1u;
              column < (column_count);
              column++) {
      data16[offset + column] += data16[offset + column - 1];
    }
  }
}
std::vector<U8> Deinterleave(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count
) {
  std::vector<U8> output(input.size());
  for (auto row = 0u;
            row < row_count;
            row++) {
    for (auto column = 0u;
              column < (column_count);
              column++) {
      unsigned offset = row * column_count * sizeof(F32);
      output[offset + (column * sizeof(F32)) + 0] = input[offset + column];
      output[offset + (column * sizeof(F32)) + 1] = input[offset + (column_count) * 1 + column];
      output[offset + (column * sizeof(F32)) + 2] = input[offset + (column_count) * 2 + column];
      output[offset + (column * sizeof(F32)) + 3] = input[offset + (column_count) * 3 + column];
    }
  }
  return output;
}
void DecodeDelta32(
  std::vector<U8> &data,
  unsigned row_count,
  unsigned column_count
) {
  auto index = 0u;
  for (auto row = 0u;
            row < row_count;
            row++) {
    index++;
    for (auto column = 1u;
              column < (column_count * sizeof(F32));
              column++) {
      data[index] += data[index-1];
      index++;
    }
  }
  data = Deinterleave(data, row_count, column_count);
  #ifdef PSD_LITTLE_ENDIAN
  for (auto &value : reinterpret_cast<std::vector<U32> &>(data)) value = detail::ByteSwap(value);
  #endif
}
}; // namespace
std::vector<U8>
DecompressDeflateDelta(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth
) {
  auto decompressed = DecompressDeflate(
    input,
    row_count,
    column_count,
    depth
  );
  auto decode_delta = [&](auto function) {
    return function(
      decompressed,
      row_count,
      column_count
    );
  };
  switch (depth) {
    case Depth::One       : throw Error("Err");
    case Depth::Eight     : decode_delta(DecodeDelta8); break;
    case Depth::Sixteen   : decode_delta(DecodeDelta16); break;
    case Depth::ThirtyTwo : decode_delta(DecodeDelta32); break;
    default:
    throw Error("ErrD");
  }
  return decompressed;
}

inline int ZeroCount(std::uint32_t value) {
  #if PSD_COMPILER_MSVC
    unsigned long index;
    if (_BitScanForward(&index, value)) {
      return static_cast<int>(index);
    }
    return 32;
  #elif PSD_COMPILER_GCC || PSD_COMPILER_CLANG
    return __builtin_ctz(value);
  #endif
}
template <typename I>
void InsertCompressed(
  std::vector<U8> &output,
  I input,
  I end
) {
  while (input != end) {
    std::uint8_t value = *input++;
    std::int16_t count = 1;

    while (input != end && count < 128) {
      std::uint64_t remaining = std::distance(input, end);

      if (remaining >= xsimd::batch<U8>::size && count + xsimd::batch<U8>::size < 128) {
        auto compare = xsimd::load_unaligned(&*input) == xsimd::batch<U8>(value);

        if (xsimd::all(compare)) {
          count += xsimd::batch<U8>::size;
          input += xsimd::batch<U8>::size;
          continue;
        }
        int matching = ZeroCount(~compare.mask());
        count += matching;
        input += matching;
        break;

      } else {
        if (*input != value) {
          break;
        }
        count++;
        input++;
      }
    }
    if (count > 1 && count != 2) {
      output.push_back(-(count - 1));
      output.push_back(value);
    } else {

      while ((input + 0) != end &&
             (input + 1) != end && count < 128) {
        std::uint64_t remaining = std::distance(input, end);

        if (remaining >= (xsimd::batch<U8>::size * 2) && count + (xsimd::batch<U8>::size * 2) < 128) {
          auto compare = xsimd::load_unaligned(&*(input)) ==
                         xsimd::load_unaligned(&*(input + xsimd::batch<U8>::size));

          if (xsimd::none(compare)) {
            count += (xsimd::batch<U8>::size * 2);
            input += (xsimd::batch<U8>::size * 2);
            continue;
          }
          auto non_matching = ZeroCount(compare.mask());
          count += non_matching;
          input += non_matching;
          break;
        } else {
          if (*(input + 0) ==
              *(input + 1)) {
            break;
          }
          count++;
          input++;
        }
      }
      output.push_back(count - 1);
      output.insert(output.end(), input - count, input);
    }
  }
}
std::vector<U8> CompressDefault(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  unsigned
) {
  std::vector<U8> output(row_count * sizeof(U16));
  auto iterator = input.begin();
  output.reserve(output.size() + row_count * column_count);
  for (auto index = 0;
            index < row_count;
            index++) {
      union {
          U16 value; U8 array[2];
      } count;
      auto before_insert = output.size();
      InsertCompressed(
        output,
        iterator,
        iterator + column_count
      );
      iterator += column_count;
      count.value = output.size() - before_insert;
#if PSD_LITTLE_ENDIAN
      output[(index * 2) + 0] = count.array[1];
      output[(index * 2) + 1] = count.array[0];
#else
      output[(index * 2) + 0] = count.array[0];
      output[(index * 2) + 1] = count.array[1];
#endif
  }
  return output;
}
std::vector<U8>
CompressDeflate(
  const std::vector<U8> &input,
  unsigned,
  unsigned,
  Depth,
  unsigned level
) {
  auto compressor = libdeflate_alloc_compressor(level);
  if (!compressor) {
    throw Error("PSD::Error: CompressionError");
  }
  std::vector<U8> output(libdeflate_zlib_compress_bound(compressor, input.size()));
  auto length = libdeflate_deflate_compress(
    compressor,
    input.data(),
    input.size(),
    output.data() + 2,
    output.size() - 6
  );
  if (!length) {
    libdeflate_free_compressor(compressor);
    throw Error("PSD::Error: CompressionError");
  }
  output.resize(length + 6);
  output[0] = 0x78;
  output[1] = 0xDA;
  union {
    U32 value;
    U8  array[sizeof value];
  } adler;
  adler.value = libdeflate_adler32(1, output.data() + 2, output.size() - 6);
  #if PSD_LITTLE_ENDIAN
    output[output.size()-1] = adler.array[0];
    output[output.size()-2] = adler.array[1];
    output[output.size()-3] = adler.array[2];
    output[output.size()-4] = adler.array[3];
  #else
    output[output.size()-1] = adler.array[3];
    output[output.size()-2] = adler.array[2];
    output[output.size()-3] = adler.array[1];
    output[output.size()-4] = adler.array[0];
  #endif
  libdeflate_free_compressor(compressor);
  return output;
}
namespace {
void EncodeDelta8(
  std::vector<U8> &data,
  unsigned row_count,
  unsigned column_count
) {
  for (auto row = 0u;
            row < row_count;
            row++) {
    auto offset   = row * column_count;
    auto previous = data[offset];
    for (auto column = 1u;
              column < (column_count);
              column++)
    {
      auto current = data[offset + column];
      data[offset + column] -= previous;
      previous = current;
    }
  }
}
} // namespace
std::vector<U8> CompressDeflateDelta(
  const std::vector<U8> &input,
  unsigned row_count,
  unsigned column_count,
  Depth depth,
  unsigned level
) {
  auto encoded = input;
  auto encode_delta = [&](auto function) {
    return function(
      encoded,
      row_count,
      column_count
    );
  };
  switch (depth) {
    case Depth::Eight: encode_delta(EncodeDelta8); break;
    default: throw Error("EncodeDeltaErr");
  }
  return CompressDeflate(
    encoded,
    row_count,
    column_count,
    depth,
    level
  );
}
}; // namespace PSD::llapi
