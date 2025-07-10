
#include "psd/llapi/stream.h"
#include <psd/llapi/structure/info/layer_info/channel_data.h>

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
std::vector<U8>
Decompress(const std::vector<U8> &input, unsigned row_count, unsigned column_count) {
  std::vector<U8> output(row_count * column_count);

  auto ooffset = 0ul;
  auto ioffset = 0ul;
  auto citerator = input.begin();

  const auto clength = row_count * 2;
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
    ooffset += column_count;
  }
  return output;
}
}; // namespace PSD::llapi
