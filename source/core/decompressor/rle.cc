
#include <psd/core/decompressor.h>
#include <image/core/endianness.h>

namespace PSD {

class RLEDecompressor {
public:

  RLEDecompressor(const std::vector<std::uint8_t> &input) : input_(input) {}

  std::vector<std::uint8_t> Decompress(std::uint64_t row_count, std::uint64_t column_count) const {
    if (input_.size() < row_count * 2) {
      throw std::runtime_error("Input too small for count list");
    }
    std::vector<std::uint8_t> output(row_count * column_count); 

    auto output_offset  = 0ul;
    auto input_offset   = 0ul;
    auto input_iterator = input_.begin();

    const auto count_list_length = row_count * sizeof(std::uint16_t);

    for (auto index = 0ul;
              index < row_count;
              index++) {
      std::uint16_t count = 0;

      auto ccount = reinterpret_cast<std::uint8_t *>(&count);
      ccount[0] = *input_iterator++;
      ccount[1] = *input_iterator++;
      count = Image::ConvertEndianness<Image::Endianness::Big>(count);

      DecompressLine(
        input_.begin() + count_list_length + input_offset,
        input_.begin() + count_list_length + input_offset + count,
        output.begin() + output_offset,
        output.begin() + output_offset + column_count
      );
      input_offset  += count;
      output_offset += column_count;
    }
    return output;
  }

private:
  const std::vector<std::uint8_t> &input_;

  void DecompressLine(
    std::vector<std::uint8_t>::const_iterator input_iterator,
    std::vector<std::uint8_t>::const_iterator input_end,
    std::vector<std::uint8_t>::iterator       output_iterator,
    std::vector<std::uint8_t>::iterator       output_end
  ) const {

    while (input_iterator != input_end) {
      std::int16_t count = static_cast<std::int8_t>(*input_iterator++);  
      if (count < 0) {
        count = -count + 1;

        if (input_iterator == input_end) {
          throw std::runtime_error("Unexpected end of input");
        }
        std::int8_t value = *input_iterator++;
        std::fill(output_iterator, output_iterator + count, value);
        output_iterator += count;
      } else {
        count++;
        std::copy(input_iterator, input_iterator + count, output_iterator);
        input_iterator  += count;
        output_iterator += count;
      }
    }
  }
};
std::vector<std::uint8_t> Decompressor<std::vector<std::uint8_t>>::DecompressRLE(
  std::uint64_t row_count, std::uint64_t column_count
) const {
  return RLEDecompressor(input_).Decompress(row_count, column_count);
}
};
